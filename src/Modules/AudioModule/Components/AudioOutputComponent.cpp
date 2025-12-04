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

#include "AudioOutputComponent.h"

#include <portaudio.h>

#include <algorithm>

#include "Core/Util/Logger.h"
#include "Modules/AudioModule/AudioUtils.h"
#include "MultiChannelResampler.h"

namespace nv3dvc {
namespace modules {
namespace audiomodule {
namespace components {

struct OutputCallbackWrapper {
  static int Call(const void* input_buffer, void* output_buffer,
                  unsigned long num_samples,  // NOLINT(runtime/int) The PortAudio API uses `unsigned long`.
                  const PaStreamCallbackTimeInfo* time_info, PaStreamCallbackFlags status_flags, void* user_data) {
    AudioOutputComponent* component = static_cast<AudioOutputComponent*>(user_data);
    const bool success = component->OutputCallback(static_cast<float*>(output_buffer), num_samples);
    return success ? paContinue : paAbort;
  }
};

AudioOutputComponent::AudioOutputComponent() = default;

AudioOutputComponent::~AudioOutputComponent() { Uninitialize(); }

core::Error AudioOutputComponent::Initialize() {
  const PaDeviceInfo* device_info = nullptr;
  PaError pa_err;

  const PaHostApiIndex host_api = Pa_HostApiTypeIdToHostApiIndex(kDefaultAudioAPI);
  const PaHostApiInfo* host_api_info = Pa_GetHostApiInfo(host_api);

  if (use_default_device) {
    const PaDeviceIndex device_index = host_api_info->defaultOutputDevice;
    if (device_index == paNoDevice) {
      LOG_ERROR("No default audio output device is available or an error was encountered.");
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
        if (tmp_device_info->maxOutputChannels > 0) {
          LOG_INFO("Found requested audio output device '%s'.", device_name.get()->c_str());
          device_info = tmp_device_info;
          m_deviceIndex = device_index;
          break;
        } else {
          LOG_WARNING("Found requested audio output device '%s', but it does not support output.",
                      device_name.get()->c_str());
        }
      }
    }
  }

  if (device_info != nullptr) {
    LOG_INFO("Using audio output device #%d - %s", m_deviceIndex, device_name.get()->c_str());
  } else {
    LOG_ERROR("Could not find audio output device '%s' in '%s' api.", device_name.get()->c_str(), host_api_info->name);
    return core::Error::ERR_INITIALIZATION;
  }

  // Don't send more than two channels to the output device.
  m_deviceChannels = std::min(2, device_info->maxOutputChannels);

  // We assume 48 kHz.
  m_sourceSampleRate = 48'000;

  // Test supported sample rates.
  PaStreamParameters output_parameters;
  output_parameters.device = m_deviceIndex;
  output_parameters.channelCount = m_deviceChannels;
  output_parameters.sampleFormat = paFloat32;
  output_parameters.suggestedLatency = 0; /* ignored by Pa_IsFormatSupported() */
  output_parameters.hostApiSpecificStreamInfo = NULL;
  // Try to open the device with the source sample rate. If that fails, use the default sample rate.
  for (const int device_sample_rate : {m_sourceSampleRate, static_cast<int>(device_info->defaultSampleRate)}) {
    pa_err = Pa_IsFormatSupported(nullptr, &output_parameters, device_sample_rate);
    if (pa_err == paFormatIsSupported) {
      LOG_INFO("Audio output device supports sample rate: %d", device_sample_rate);
      m_deviceSampleRate = device_sample_rate;
      break;
    } else {
      LOG_INFO("Audio output device does not support sample rate: %d", device_sample_rate);
    }
  }
  if (pa_err != paFormatIsSupported) {
    LOG_ERROR("Could not find supported sample rate for audio output device");
    return core::Error::ERR_INITIALIZATION;
  }

  if (m_deviceSampleRate != m_sourceSampleRate) {
    // We need to resample.
    LOG_INFO("Audio output will be resampled from %d Hz to %d Hz", m_sourceSampleRate, m_deviceSampleRate);
    m_resampler.reset(
        oboe::resampler::MultiChannelResampler::make(m_deviceChannels, m_sourceSampleRate, m_deviceSampleRate,
                                                     oboe::resampler::MultiChannelResampler::Quality::Medium));
  }

  // PortAudio calls this framesPerBuffer. They define a frame as one sample from each channel.
  const int playback_samples_per_frame = 1024;  // From AudioHandler.h.

  LOG_INFO("Latency : %.1f ms", device_info->defaultLowOutputLatency);

  output_parameters.device = m_deviceIndex;
  output_parameters.channelCount = m_deviceChannels;
  output_parameters.sampleFormat = paFloat32;
  output_parameters.suggestedLatency = device_info->defaultLowOutputLatency;
  output_parameters.hostApiSpecificStreamInfo = NULL;
  LOG_INFO("Output Channels : %d", output_parameters.channelCount);

  pa_err = Pa_OpenStream(&m_audioStream,               // stream
                         NULL,                         // no input
                         &output_parameters,           // output
                         m_deviceSampleRate,           //
                         playback_samples_per_frame,   //
                         paClipOff,                    // streamFlags
                         OutputCallbackWrapper::Call,  // streamCallback
                         this);                        // userData for callback

  if (pa_err != paNoError) {
    LOG_ERROR("Failed to open audio stream");
    return core::Error::ERR_INITIALIZATION;
  }

  m_isInitialized = true;
  return core::Error::SUCCESS;
}

void AudioOutputComponent::Uninitialize() {
  StopOutputThread();
  m_isInitialized = false;
}

bool AudioOutputComponent::IsInitialized() const { return m_isInitialized; }

int AudioOutputComponent::GetDeviceOutputSampleRate() const { return m_deviceSampleRate; }

int AudioOutputComponent::GetDeviceOutputChannels() const { return m_deviceChannels; }

void AudioOutputComponent::SetSourceCallback(std::function<void(float* dst_data, int dst_samples)> callback) {
  m_sourceCallback = callback;
}

void AudioOutputComponent::AddOutputCaptureCallback(
    std::function<void(const float* src_data, int src_samples)> callback) {
  m_outputCaptureCallbacks.push_back(callback);
}

void AudioOutputComponent::StartOutputThread() {
  LOG_VERBOSE("Starting audio output stream");
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

  LOG_VERBOSE("Audio output stream started");
}

void AudioOutputComponent::StopOutputThread() {
  LOG_VERBOSE("Stopping audio output stream");
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
  LOG_VERBOSE("Audio output stream stopped");
}

bool AudioOutputComponent::IsOutputThreadStarted() const { return !Pa_IsStreamStopped(m_audioStream); }

bool AudioOutputComponent::OutputCallback(float* output_buffer, const size_t num_output_samples) {
  // Initialize to zero.
  memset(output_buffer, 0.0f, sizeof(float) * num_output_samples * m_deviceChannels);

  // Get source audio.
  const double output_duration = static_cast<double>(num_output_samples) / m_deviceSampleRate;
  const int total_input_samples = static_cast<int>(std::ceil(output_duration * m_sourceSampleRate));
  const int current_input_samples = m_tmpSourceBuffer.size() / m_deviceChannels;
  const int additional_input_samples = total_input_samples - current_input_samples;
  m_tmpSourceBuffer.resize(total_input_samples * m_deviceChannels);
  if (m_sourceCallback) {
    // Add new data behind the old data.
    m_sourceCallback(m_tmpSourceBuffer.data() + current_input_samples * m_deviceChannels, additional_input_samples);
  }

  // Resample if necessary.
  if (m_resampler) {
    // Resample into the output buffer.
    const float* src_ptr = m_tmpSourceBuffer.data();
    float* dst_ptr = output_buffer;
    int num_input_samples = 0;
    int output_samples_left = num_output_samples;
    while (output_samples_left > 0) {
      if (m_resampler->isWriteNeeded()) {
        m_resampler->writeNextFrame(src_ptr);
        src_ptr += m_deviceChannels;
        num_input_samples++;
      } else {
        m_resampler->readNextFrame(dst_ptr);
        dst_ptr += m_deviceChannels;
        output_samples_left--;
      }
    }
    // Erase consumed input samples.
    m_tmpSourceBuffer.erase(m_tmpSourceBuffer.begin(),
                            m_tmpSourceBuffer.begin() + num_input_samples * m_deviceChannels);
  } else {
    std::copy(m_tmpSourceBuffer.begin(), m_tmpSourceBuffer.end(), output_buffer);
    m_tmpSourceBuffer.clear();
  }

  for (auto& callback : m_outputCaptureCallbacks) {
    callback(output_buffer, num_output_samples);
  }

  return true;
}

}  // namespace components
}  // namespace audiomodule
}  // namespace modules
}  // namespace nv3dvc
