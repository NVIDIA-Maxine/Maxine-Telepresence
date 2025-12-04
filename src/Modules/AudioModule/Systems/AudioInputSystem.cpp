/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: LicenseRef-NvidiaProprietary
 *
 * NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
 * property and proprietary rights in and to this material, related
 * documentation and any modifications thereto. Any use, reproduction,
 * disclosure or distribution of this material and related documentation
 * without an express license agreement from NVIDIA CORPORATION or
 * its affiliates is strictly prohibited.
 */

#include "AudioInputSystem.h"

#include <algorithm>
#include <utility>

#include "Core/Engine/Engine.h"
#include "Core/EntityComponentSystem/Entity.h"
#include "Core/Error.h"
#include "Core/Util/Logger.h"
#include "Modules/AudioModule/AudioUtils.h"
#include "Modules/AudioModule/Components/AudioInputComponent.h"
#include "Modules/AudioModule/Components/AudioSinkComponent.h"
#include "Modules/AudioModule/Components/AudioSourceComponent.h"
#include "Modules/CommonModule/Components/CallbackComponent.h"

namespace nv3dvc {
namespace modules {
namespace audiomodule {
namespace systems {

AudioInputSystem::AudioInputSystem() = default;

AudioInputSystem::~AudioInputSystem() {}

core::Error AudioInputSystem::OnLoadScene(core::ecs::registry::EntityRegistry* reg) {
  core::Error err = core::SUCCESS;

  auto sink_entities = reg->view<audiomodule::components::AudioSinkComponent>();
  auto input_entities = reg->view<audiomodule::components::AudioInputComponent>();
  auto source_entities = reg->view<audiomodule::components::AudioSourceComponent>();

  for (const core::ecs::Entity& sink_entity : sink_entities) {
    auto& sink_component = sink_entity.GetComponent<audiomodule::components::AudioSinkComponent>();
    if (!sink_component.IsInitialized()) sink_component.Initialize();
  }
  LOG_DEBUG("Found %d entities with AudioInputComponent", (int)input_entities.size());
  if (input_entities.size() > 1) {
    // Currently, the `AudioSinkComponent`s can only accept audio from one `AudioInputComponent`.
    LOG_ERROR("Too many entities with AudioInputComponent in the scene");
    return core::ERR_UNIMPLEMENTED;
  }
  for (core::ecs::Entity& input_entity : input_entities) {
    auto& input_component = input_entity.GetComponent<audiomodule::components::AudioInputComponent>();
    CHECK_SUCCESS(input_component.Initialize());
    const int input_channels = input_component.GetNumChannels();

    // Set up input audio processor.
    InputProcessor& input_processor = m_inputsProcessor[input_entity];
    input_processor.input_ring_buffer = AudioRingBuffer::Create(0.1f, input_component.GetSampleRate(), input_channels);

    // Create echo cancellers for all sources.
    input_processor.aec_frame_size = 1;
    for (core::ecs::Entity& source_entity : source_entities) {
      auto& source_component = source_entity.GetComponent<audiomodule::components::AudioSourceComponent>();
      InputProcessor::AECProvider& source_aec_provider = input_processor.sources_aec_provider[source_entity];

      // Create acoustic echo canceller.
      if (enable_aec) {
        if (source_component.GetNumChannels() == 1) {
          auto echo_canceller = CreateEchoCanceller();
          if (echo_canceller && echo_canceller->Initialize(afx_sdk_model_dir)) {
            // create audio ring buffer for echo cancellation
            source_aec_provider.echo_canceller = std::move(echo_canceller);
            source_aec_provider.far_end_audio_data_ring_buffer =
                AudioRingBuffer::Create(10.f, source_component.GetSampleRate(), 1);

            const int source_aec_frame_size = source_aec_provider.echo_canceller->GetNumSamplesPerFrame();
            // Get the AEC frame size from the first source.
            if (input_processor.aec_frame_size == 1) {
              input_processor.aec_frame_size = source_aec_frame_size;
            }
            // Verify AEC frame sizes are the same for all sources.
            if (input_processor.aec_frame_size != source_aec_frame_size) {
              LOG_ERROR("AEC frame sizes do not match");
            }
          } else {
            LOG_ERROR("Failed to initialize acoustic echo canceller");
          }
        } else {
          LOG_ERROR("AEC requires far end audio to have 1 channel, received %d channels",
                    source_component.GetNumChannels());
        }
      }
    }

    // Set the input component's callback that will be called by PortAudio when new audio is available.
    input_component.SetSinkCallback(
        [&input_processor, sink_entities, input_channels](const float* src_data, int src_samples) {
          // Put the new samples into the input ring buffer for processing.
          input_processor.input_ring_buffer->Push(src_data, src_samples * input_channels);

          const int num_samples_available = input_processor.input_ring_buffer->GetNumAvailableValues() / input_channels;

          // Compute number of samples that will be processed this time.
          // Other frames will be stored and processed next time.
          const int frame_size = input_processor.aec_frame_size;
          const int num_aec_frames = num_samples_available / frame_size;
          const int num_samples_to_process = num_aec_frames * frame_size;

          // Get the audio that we want to process, and put it into tmp_buffer.
          input_processor.tmp_buffer.resize(num_samples_to_process * input_channels);
          input_processor.input_ring_buffer->Pop(input_processor.tmp_buffer.data(), input_processor.tmp_buffer.size());

          // Apply noise removal here.

          // Acoustic echo cancellation (AEC).
          input_processor.tmp_buffer2.resize(input_processor.tmp_buffer.size());
          for (auto& [source_entity, source_aec_provider] : input_processor.sources_aec_provider) {
            // Skip inputs that aren't contributing to AEC.
            if (!source_aec_provider.echo_canceller) continue;

            const int far_end_channels = source_aec_provider.far_end_audio_data_ring_buffer->GetNumChannels();
            if (far_end_channels != 1) {
              LOG_ERROR("AEC requires far end audio to have 1 channel, received %d channels", far_end_channels);
            }

            // run AEC iteratively
            input_processor.tmp_far_end_frame.resize(frame_size);
            for (int offset = 0; offset < num_samples_to_process; offset += frame_size) {
              // Grab far end samples from ring buffer.
              const int available_samples = source_aec_provider.far_end_audio_data_ring_buffer->Pop(
                  input_processor.tmp_far_end_frame.data(), frame_size);
              if (available_samples < frame_size) {
                // If we didn't have enough to fill the frame, fill the rest with zeros.
                std::fill(input_processor.tmp_far_end_frame.begin() + available_samples,
                          input_processor.tmp_far_end_frame.end(), 0.0f);
              }

              // perform AEC at AEC framesize
              source_aec_provider.echo_canceller->Filter(input_processor.tmp_buffer.data() + offset,  //
                                                         input_processor.tmp_far_end_frame.data(),    //
                                                         input_processor.tmp_buffer2.data() + offset);
            }

            // Copy result back to tmp_buffer.
            std::copy(input_processor.tmp_buffer2.cbegin(), input_processor.tmp_buffer2.cend(),
                      input_processor.tmp_buffer.begin());
          }

          // Apply VAD here.

          // Send the processed audio to any AudioSinkComponents.
          for (const core::ecs::Entity& sink_entity : sink_entities) {
            auto& sink_component = sink_entity.GetComponent<audiomodule::components::AudioSinkComponent>();
            const int sink_channels = sink_component.GetNumChannels();
            const float* output_src_ptr = input_processor.tmp_buffer.data();
            int num_output_samples = input_processor.tmp_buffer.size() / input_channels;
            if (input_channels != sink_channels) {
              // Convert from input_channels to sink_channels.
              input_processor.tmp_buffer2.resize(num_output_samples * sink_channels);
              utils::TransferAudio(output_src_ptr, input_channels, num_output_samples,  //
                                   input_processor.tmp_buffer2.data(), sink_channels);
              // Update src_data to point to the converted audio.
              output_src_ptr = input_processor.tmp_buffer2.data();
            }
            sink_component.PushSinkAudio(output_src_ptr, num_output_samples);
          }

          input_processor.tmp_buffer.clear();
          input_processor.tmp_buffer2.clear();
        });
  }

  // For each source, get a list of input devices that need a copy of their audio for AEC.
  for (core::ecs::Entity& source_entity : source_entities) {
    auto& source_component = source_entity.GetComponent<audiomodule::components::AudioSourceComponent>();
    std::vector<std::shared_ptr<AudioRingBuffer>> aec_ring_buffers_for_source;
    for (core::ecs::Entity& input_entity : input_entities) {
      auto ring_buffer =
          m_inputsProcessor[input_entity].sources_aec_provider[source_entity].far_end_audio_data_ring_buffer;
      if (ring_buffer) {
        aec_ring_buffers_for_source.push_back(ring_buffer);
      }
    }
    source_component.SetDestinationRingBuffers(aec_ring_buffers_for_source);
  }

bail:
  return err;
}

core::Error AudioInputSystem::OnUnloadScene(core::ecs::registry::EntityRegistry* reg) {
  for (core::ecs::Entity& input_entity : reg->view<audiomodule::components::AudioInputComponent>()) {
    auto& input_component = input_entity.GetComponent<audiomodule::components::AudioInputComponent>();
    input_component.StopInputThread();
  }
  return core::SUCCESS;
}

core::Error AudioInputSystem::Run(core::ecs::registry::EntityRegistry* reg, float dt) {
  core::Error err = core::SUCCESS;

  for (core::ecs::Entity& input_entity : reg->view<audiomodule::components::AudioInputComponent>()) {
    auto& input_component = input_entity.GetComponent<audiomodule::components::AudioInputComponent>();
    if (input_component.capture_api == AudioInputApi::WEB_CAMERA &&
        input_entity.HasComponent<commonmodule::components::WebCameraAudioCallbackComponent>()) {
      auto& webcam_audio_cb_component =
          input_entity.GetComponent<commonmodule::components::WebCameraAudioCallbackComponent>();

      if (webcam_audio_cb_component.HasOnFiredCallback()) {
        // Get audio format.
        const int sample_rate = webcam_audio_cb_component.SampleRate();
        const int num_channels = webcam_audio_cb_component.NumChannels();
        input_component.SetDeviceFormat(sample_rate, num_channels);

        // Allocate temporary buffer with enough space for 1 second of data.
        const int max_samples = sample_rate;
        m_tmpBuffer.resize(max_samples * num_channels);
        // Get audio data.
        size_t pulled_samples = 0;
        err = webcam_audio_cb_component.Fire(m_tmpBuffer.data(), max_samples, &pulled_samples);
        if (pulled_samples > 0) {
          input_component.InputCallback(m_tmpBuffer.data(), pulled_samples);
        } else if (err == core::Error::ERR_DATA_UNAVAILABLE) {
          err = core::Error::SUCCESS;
        } else if (err == core::Error::ERR_EOF) {
          return err;  // Silently return for EOF
        }
        CHECK_SUCCESS(err);
      }
    } else if (input_component.capture_api == AudioInputApi::PORT_AUDIO) {
      // Start threads if not yet started.
      if (!input_component.IsInputThreadStarted()) input_component.StartInputThread();
    }
  }

bail:
  return err;
}

}  // namespace systems
}  // namespace audiomodule
}  // namespace modules
}  // namespace nv3dvc
