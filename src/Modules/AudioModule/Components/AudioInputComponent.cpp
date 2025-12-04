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

#include "AudioInputComponent.h"

#include <portaudio.h>

#include <algorithm>
#include <string>

#include "Core/Util/Logger.h"
#include "Modules/AudioModule/AudioUtils.h"
#include "MultiChannelResampler.h"

namespace nv3dvc {
namespace modules {
namespace audiomodule {
namespace components {

struct InputCallbackWrapper {
  static int Call(const void* input, void* output,
                  unsigned long num_samples,  // NOLINT(runtime/int) The PortAudio API uses `unsigned long`.
                  const PaStreamCallbackTimeInfo* time_info, PaStreamCallbackFlags status_flags, void* user_data) {
    AudioInputComponent* component = static_cast<AudioInputComponent*>(user_data);
    const bool success = component->InputCallback(static_cast<const float*>(input), num_samples);
    return success ? paContinue : paAbort;
  }
};

AudioInputComponent::AudioInputComponent() = default;

AudioInputComponent::~AudioInputComponent() { Uninitialize(); }

core::Error AudioInputComponent::Initialize() {
  // Don't store more than one channel in the capture buffer.
  m_captureChannels = 1;
  m_captureSampleRate = 48'000;

  if (capture_api == AudioInputApi::PORT_AUDIO) {
    const PaDeviceInfo* device_info = nullptr;
    PaError pa_err;

    const PaHostApiIndex host_api = Pa_HostApiTypeIdToHostApiIndex(kDefaultAudioAPI);
    const PaHostApiInfo* host_api_info = Pa_GetHostApiInfo(host_api);

    if (use_default_device) {
      const PaDeviceIndex device_index = host_api_info->defaultInputDevice;
      if (device_index == paNoDevice) {
        LOG_ERROR("No default audio input device is available, or an error was encountered.");
        return core::Error::ERR_INITIALIZATION;
      }
      device_info = Pa_GetDeviceInfo(device_index);
      m_deviceIndex = device_index;
      device_name = std::string(device_info->name);
    } else {
      for (int host_api_device_index = 0; host_api_device_index < host_api_info->deviceCount; host_api_device_index++) {
        const PaDeviceIndex device_index = Pa_HostApiDeviceIndexToDeviceIndex(host_api, host_api_device_index);
        const PaDeviceInfo* tmp_device_info = Pa_GetDeviceInfo(device_index);
        if (device_name.get()->compare(tmp_device_info->name) == 0) {
          if (tmp_device_info->maxInputChannels > 0) {
            LOG_INFO("Found requested audio input device '%s'.", device_name.get()->c_str());
            device_info = tmp_device_info;
            m_deviceIndex = device_index;
            break;
          } else {
            LOG_WARNING("Found requested audio input device '%s', but it does not support input.",
                        device_name.get()->c_str());
          }
        }
      }
    }

    if (device_info != nullptr) {
      LOG_INFO("Using audio input device #%d - %s", m_deviceIndex, device_name.get()->c_str());
    } else {
      LOG_ERROR("Could not find audio input device '%s' in '%s' api.", device_name.get()->c_str(), host_api_info->name);
      return core::Error::ERR_INITIALIZATION;
    }

    // Capture up to two channels from the input device.
    const int device_channels = std::min(device_info->maxInputChannels, 2);

    // Test supported sample rates.
    PaStreamParameters input_parameters;
    input_parameters.device = m_deviceIndex;
    input_parameters.channelCount = device_channels;
    input_parameters.sampleFormat = paFloat32;
    input_parameters.suggestedLatency = 0; /* ignored by Pa_IsFormatSupported() */
    input_parameters.hostApiSpecificStreamInfo = NULL;
    // Try to open the device with the output sample rate. If that fails, use the default sample rate.
    for (const int device_sample_rate : {m_captureSampleRate, static_cast<int>(device_info->defaultSampleRate)}) {
      pa_err = Pa_IsFormatSupported(&input_parameters, nullptr, device_sample_rate);
      if (pa_err == paFormatIsSupported) {
        LOG_INFO("Audio input device supports sample rate: %d", device_sample_rate);
        SetDeviceFormat(device_sample_rate, device_channels);
        break;
      } else {
        LOG_INFO("Audio input device does not support sample rate: %d", device_sample_rate);
      }
    }
    if (pa_err != paFormatIsSupported) {
      LOG_ERROR("Could not find supported sample rate for audio input device");
      return core::Error::ERR_INITIALIZATION;
    }

    LOG_INFO("Latency : %.1f ms", device_info->defaultLowInputLatency);

    input_parameters.device = m_deviceIndex;
    input_parameters.channelCount = m_deviceChannels;
    input_parameters.sampleFormat = paFloat32;
    input_parameters.suggestedLatency = device_info->defaultLowInputLatency;
    input_parameters.hostApiSpecificStreamInfo = NULL;
    LOG_INFO("Input Channels : %d", input_parameters.channelCount);

    pa_err = Pa_OpenStream(&m_audioStream,                // stream
                           &input_parameters,             // input
                           nullptr,                       // no output
                           m_deviceSampleRate,            //
                           paFramesPerBufferUnspecified,  //
                           paClipOff,                     // streamFlags
                           InputCallbackWrapper::Call,    // streamCallback
                           this);                         // userData for callback

    if (pa_err != paNoError) {
      LOG_ERROR("Failed to open audio stream");
      return core::Error::ERR_INITIALIZATION;
    }
  } else if (capture_api == AudioInputApi::WEB_CAMERA) {
    m_deviceIndex = 0;
    device_name = std::string();
    m_deviceSampleRate = 0;
    m_deviceChannels = 0;
  } else {
    LOG_ERROR("Unknown audio capture API requested");
    return core::Error::ERR_INITIALIZATION;
  }

  m_isInitialized = true;
  return core::Error::SUCCESS;
}

void AudioInputComponent::Uninitialize() {
  StopInputThread();
  m_isInitialized = false;
}

bool AudioInputComponent::IsInitialized() const { return m_isInitialized; }

int AudioInputComponent::GetSampleRate() const { return m_captureSampleRate; }

int AudioInputComponent::GetNumChannels() const { return m_captureChannels; }

void AudioInputComponent::SetSinkCallback(std::function<void(const float* src_data, int src_samples)> callback) {
  m_sinkCallback = callback;
}

void AudioInputComponent::StartInputThread() {
  if (!m_isInitialized) {
    LOG_ERROR("AudioInputComponent is not initialized");
    return;
  }

  LOG_VERBOSE("Starting audio input stream");
  const PaError stopped = Pa_IsStreamStopped(m_audioStream);
  if (stopped < 0) {
    LOG_WARNING("Pa_IsStreamStopped returned %#x", stopped);
    return;
  } else if (stopped == 0) {
    LOG_WARNING("audio stream is already started");
    return;
  }

  const PaError err = Pa_StartStream(m_audioStream);
  if (err != paNoError) {
    LOG_WARNING("Failed to start audio stream");
    return;
  }

  LOG_VERBOSE("Audio input stream started");
}

void AudioInputComponent::StopInputThread() {
  LOG_VERBOSE("Stopping audio input stream");
  const PaError stopped = Pa_IsStreamStopped(m_audioStream);
  if (stopped < 0) {
    LOG_WARNING("Pa_IsStreamStopped returned %#x", stopped);
    return;
  } else if (stopped > 0) {
    LOG_WARNING("audio stream is already stopped");
    return;
  }

  const PaError err = Pa_StopStream(m_audioStream);
  if (err != paNoError) {
    LOG_WARNING("Failed to stop audio processing stream");
  }
  LOG_VERBOSE("Audio input stream stopped");
}

bool AudioInputComponent::IsInputThreadStarted() const { return !Pa_IsStreamStopped(m_audioStream); }

void AudioInputComponent::SetDeviceFormat(int sample_rate, int num_channels) {
  if (m_deviceSampleRate == sample_rate && m_deviceChannels == num_channels) return;

  m_deviceSampleRate = sample_rate;
  m_deviceChannels = num_channels;

  if (m_deviceSampleRate != m_captureSampleRate) {
    // We need to resample.
    LOG_INFO("Audio input will be resampled from %d Hz to %d Hz", m_deviceSampleRate, m_captureSampleRate);
    m_resampler.reset(
        oboe::resampler::MultiChannelResampler::make(m_captureChannels, m_deviceSampleRate, m_captureSampleRate,
                                                     oboe::resampler::MultiChannelResampler::Quality::Medium));
  }
}

bool AudioInputComponent::InputCallback(const float* input_buffer, const size_t num_input_samples) {
  // Convert channels.
  m_tmpMixedBuffer.resize(num_input_samples * m_captureChannels);
  utils::TransferAudio(input_buffer, m_deviceChannels, num_input_samples, m_tmpMixedBuffer.data(), m_captureChannels);

  float* output_src_ptr = nullptr;
  int num_output_samples = 0;

  // Resample if necessary.
  if (m_resampler) {
    // Temporarily enlarge m_tmpResampledBuffer to accommodate up to 1 sec of new samples.
    m_tmpResampledBuffer.resize(m_captureSampleRate * m_captureChannels);

    // Resample input audio.
    const float* src_ptr = m_tmpMixedBuffer.data();
    float* dst_ptr = m_tmpResampledBuffer.data();
    int input_samples_left = num_input_samples;
    while (input_samples_left > 0) {
      if (m_resampler->isWriteNeeded()) {
        m_resampler->writeNextFrame(src_ptr);
        src_ptr += m_captureChannels;
        input_samples_left--;
      } else {
        m_resampler->readNextFrame(dst_ptr);
        dst_ptr += m_captureChannels;
        num_output_samples++;
      }
    }
    // Shrink to fit.
    m_tmpResampledBuffer.resize(num_output_samples * m_captureChannels);
    output_src_ptr = m_tmpResampledBuffer.data();
  } else {
    num_output_samples = num_input_samples;
    output_src_ptr = m_tmpMixedBuffer.data();
  }

  // get gain: this is combination of static gain and dynamic gain
  float gain = recording_gain.get()->load();

  // Apply gain.
  std::for_each_n(output_src_ptr, num_output_samples * m_captureChannels, [gain](float& val) { val *= gain; });

  // Send the processed audio to the capture callback.
  if (m_sinkCallback) {
    m_sinkCallback(output_src_ptr, num_output_samples);
  }

  m_tmpMixedBuffer.clear();
  m_tmpResampledBuffer.clear();

  return true;
}

}  // namespace components
}  // namespace audiomodule
}  // namespace modules
}  // namespace nv3dvc
