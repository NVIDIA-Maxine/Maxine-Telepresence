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

#include "AudioOutputSystem.h"

#include <algorithm>
#include <vector>

#include "Core/EntityComponentSystem/Entity.h"
#include "Core/Util/Logger.h"
#include "Modules/AudioModule/AudioUtils.h"
#include "Modules/AudioModule/Components/AudioOutputComponent.h"
#include "Modules/AudioModule/Components/AudioSourceComponent.h"
#include "Modules/CommonModule/Components/CallbackComponent.h"

namespace nv3dvc {
namespace modules {
namespace audiomodule {
namespace systems {

AudioOutputSystem::AudioOutputSystem() {}

AudioOutputSystem::~AudioOutputSystem() {}

core::Error AudioOutputSystem::OnLoadScene(core::ecs::registry::EntityRegistry* reg) {
  core::Error err = core::Error::SUCCESS;

  // Collected the audio ring buffers for all audio sources.
  auto source_entities = reg->view<audiomodule::components::AudioSourceComponent>();
  auto output_entities = reg->view<audiomodule::components::AudioOutputComponent>();
  if (output_entities.size() > 1) {
    // Currently, each `AudioOutputComponent` will pop samples off all the `AudioSourceComponent`s.
    // So you cannot have multiple `AudioOutputComponent`s, because they will compete for the source audio.
    LOG_ERROR("Too many output components in the scene");
    return core::ERR_UNIMPLEMENTED;
  }
  for (core::ecs::Entity& entity : output_entities) {
    auto& output_component = entity.GetComponent<audiomodule::components::AudioOutputComponent>();
    if (!output_component.IsInitialized()) output_component.Initialize();
    const int output_channels = output_component.GetDeviceOutputChannels();
    // Give the output component the set of audio sources for rendering.
    output_component.SetSourceCallback([source_entities, output_channels](float* dst_data, int dst_samples) {
      std::vector<float> tmp_source_buffer;
      std::vector<float> tmp_output_buffer;
      // Loop over audio sources.
      for (const core::ecs::Entity& source_entity : source_entities) {
        auto& source_component = source_entity.GetComponent<audiomodule::components::AudioSourceComponent>();
        const int source_channels = source_component.GetNumChannels();
        tmp_source_buffer.resize(dst_samples * source_channels);
        // Copy from ring buffers.
        const int src_values = source_component.PopSourceAudio(tmp_source_buffer.data(), dst_samples);
        // Fill remainder with zeros.
        std::fill(tmp_source_buffer.begin() + src_values, tmp_source_buffer.end(), 0.0f);

        auto src_ptr = tmp_source_buffer.data();
        if (source_channels != output_channels) {
          // Convert from source_channels to output_channels.
          tmp_output_buffer.resize(dst_samples * output_channels);
          utils::TransferAudio(tmp_source_buffer.data(), source_channels, dst_samples, tmp_output_buffer.data(),
                               output_channels);
          // Update src_ptr to point to the converted audio.
          src_ptr = tmp_output_buffer.data();
        }

        // Add the source audio to the output audio.
        const int num_values = dst_samples * output_channels;
        float* dst_ptr = dst_data;
        for (int i = 0; i < num_values; ++i) {
          *dst_ptr++ += *src_ptr++;
        }
      }
    });

    // Set up any recording components who are listening.
    if (entity.HasComponent<commonmodule::components::RecordingCallbackComponent>()) {
      auto& recording_cb_component = entity.GetComponent<commonmodule::components::RecordingCallbackComponent>();
      // Set up the AudioOutputComponent to pass audio to the RecordingCallbackComponent just before playing it.
      output_component.AddOutputCaptureCallback([&recording_cb_component](const float* data, int num_samples) {
        // Bail early if no-one is listening.
        if (!recording_cb_component.HasOnFiredCallback()) return;
        size_t pushed_samples = 0;
        core::Error err = recording_cb_component.Fire(const_cast<float*>(data), num_samples, &pushed_samples,
                                                      commonmodule::components::RecordingCallbackComponent::AUDIO_DATA);
        if (pushed_samples != num_samples) {
          LOG_WARNING("Failed to capture output audio frame");
        }
      });
    }
  }

bail:
  return err;
}

core::Error AudioOutputSystem::OnUnloadScene(core::ecs::registry::EntityRegistry* reg) {
  for (core::ecs::Entity& entity : reg->view<audiomodule::components::AudioOutputComponent>()) {
    auto& output_component = entity.GetComponent<audiomodule::components::AudioOutputComponent>();
    output_component.StopOutputThread();
  }

  return core::SUCCESS;
}

core::Error AudioOutputSystem::Run(core::ecs::registry::EntityRegistry* reg, float dt) {
  core::Error err = core::Error::SUCCESS;

  // Start threads if not yet started.
  for (core::ecs::Entity& entity : reg->view<audiomodule::components::AudioOutputComponent>()) {
    auto& output_component = entity.GetComponent<audiomodule::components::AudioOutputComponent>();

    if (entity.HasComponent<commonmodule::components::RecordingCallbackComponent>()) {
      auto& recording_cb_component = entity.GetComponent<commonmodule::components::RecordingCallbackComponent>();
      if (recording_cb_component.HasOnFiredCallback()) {
        // Pass the audio format.
        commonmodule::components::RecordingCallbackComponent::AudioFormat format = {
            output_component.GetDeviceOutputSampleRate(), output_component.GetDeviceOutputChannels()};
        CHECK_SUCCESS(recording_cb_component.Fire(&format, sizeof(format), nullptr,
                                                  commonmodule::components::RecordingCallbackComponent::AUDIO_FORMAT));
      }
    }

    if (!output_component.IsOutputThreadStarted()) output_component.StartOutputThread();
  }

bail:
  return err;
}

}  // namespace systems
}  // namespace audiomodule
}  // namespace modules
}  // namespace nv3dvc
