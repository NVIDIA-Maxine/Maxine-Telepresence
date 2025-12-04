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

#include "AudioSourceComponent.h"

#include "Core/Util/Logger.h"
#include "Modules/AudioModule/AudioUtils.h"

namespace nv3dvc {
namespace modules {
namespace audiomodule {
namespace components {

AudioSourceComponent::AudioSourceComponent() : m_isInitialized(false), m_sampleRate(0), m_numChannels(0) {
  m_startTime = std::chrono::steady_clock::now();
}

bool AudioSourceComponent::Initialize(int sample_rate, int num_channels) {
  m_sampleRate = sample_rate;
  m_numChannels = num_channels;
  m_ringBuffer = AudioRingBuffer::Create(0.1f, m_sampleRate, m_numChannels);
  m_isInitialized = true;
  return true;
}

bool AudioSourceComponent::IsInitialized() const { return m_isInitialized; }

int AudioSourceComponent::GetSampleRate() const { return m_sampleRate; }

int AudioSourceComponent::GetNumChannels() const { return m_numChannels; }

int AudioSourceComponent::PushSourceAudio(const float* const src_data, const int src_samples) {
  if (!m_ringBuffer) {
    return 0;
  }

  // If anyone wants a copy of the output (for AEC) make a mono copy of it.
  if (!m_destinationRingBuffers.empty()) {
    const int dst_channels = 1;
    m_tmpSourceBuffer.resize(src_samples * dst_channels);
    // Copy from `frame` to `m_tmpSourceBuffer`.
    // Convert the frame audio to have `dst_channels` channels.
    utils::TransferAudio(src_data, m_numChannels, src_samples, m_tmpSourceBuffer.data(), dst_channels);
  }

  for (auto& ring_buffer : m_destinationRingBuffers) {
    if (ring_buffer->GetNumChannels() != 1) {
      LOG_ERROR("Destination ring buffer must have one channel");
      continue;
    }
    ring_buffer->Push(m_tmpSourceBuffer.data(), m_tmpSourceBuffer.size());
  }

  const int pushed_values = m_ringBuffer->Push(src_data, src_samples * m_numChannels);

  m_lastReceivedSamplesTimestamp = std::chrono::steady_clock::now();

  return pushed_values / m_numChannels;
}

int AudioSourceComponent::PopSourceAudio(float* dst_data, int dst_samples) {
  if (!m_ringBuffer) {
    return 0;
  }

  const int popped_values = m_ringBuffer->Pop(dst_data, dst_samples * m_numChannels);

  // Clear samples that are outdated.
  ClearOutdatedSamplesInAudioBuffers();

  return popped_values / m_numChannels;
}

void AudioSourceComponent::SetDestinationRingBuffers(
    const std::vector<std::shared_ptr<AudioRingBuffer>>& ring_buffers) {
  m_destinationRingBuffers = ring_buffers;
}

void AudioSourceComponent::ClearOutdatedSamplesInAudioBuffers() {
  const auto now = std::chrono::steady_clock::now();

  if (now - m_lastReceivedSamplesTimestamp > m_maxLifeTimeOldSamplesMs) {
    std::vector<float> clear_buffer;

    // clear streaming buffer
    int num_available_values = m_ringBuffer->GetNumAvailableValues();
    if (num_available_values > 0) {
      clear_buffer.resize(num_available_values);
      m_ringBuffer->Pop(clear_buffer.data(), num_available_values);
    }

    // clear far end reference buffers used for AEC
    for (auto& ring_buffer : m_destinationRingBuffers) {
      num_available_values = ring_buffer->GetNumAvailableValues();
      if (num_available_values > 0) {
        clear_buffer.resize(num_available_values);
        ring_buffer->Pop(clear_buffer.data(), num_available_values);
      }
    }
  }
}

}  // namespace components
}  // namespace audiomodule
}  // namespace modules
}  // namespace nv3dvc
