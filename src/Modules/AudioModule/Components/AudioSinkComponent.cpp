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

#include "AudioSinkComponent.h"

#include <algorithm>

namespace nv3dvc {
namespace modules {
namespace audiomodule {
namespace components {

AudioSinkComponent::AudioSinkComponent() = default;

bool AudioSinkComponent::Initialize() {
  m_ringBuffer = AudioRingBuffer::Create(10.f, sample_rate, num_channels);
  m_isInitialized = true;
  return true;
}

bool AudioSinkComponent::IsInitialized() const { return m_isInitialized; }

int AudioSinkComponent::GetSampleRate() const { return sample_rate; }

int AudioSinkComponent::GetNumChannels() const { return num_channels; }

int AudioSinkComponent::PushSinkAudio(const float* const src_data, const int src_samples) {
  if (!m_ringBuffer) {
    return 0;
  }

  const int pushed_values = m_ringBuffer->Push(src_data, src_samples * num_channels);
  return pushed_values / num_channels;
}

int AudioSinkComponent::PopSinkAudio(float* const dst_data, const int dst_samples, const int frame_samples) {
  if (!m_ringBuffer) {
    return 0;
  }

  // Count the number of whole frames we can pop.
  const int src_values = m_ringBuffer->GetNumAvailableValues();
  // Duration of the available audio, in samples (per channel).
  const int src_samples = src_values / num_channels;

  const int num_frames = std::min(src_samples, dst_samples) / frame_samples;
  const int popped_values = m_ringBuffer->Pop(dst_data, num_frames * frame_samples * num_channels);
  return popped_values / num_channels;
}

}  // namespace components
}  // namespace audiomodule
}  // namespace modules
}  // namespace nv3dvc
