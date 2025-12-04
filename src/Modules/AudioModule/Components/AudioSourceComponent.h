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

#ifndef SRC_MODULES_AUDIOMODULE_COMPONENTS_AUDIOSOURCECOMPONENT_H_
#define SRC_MODULES_AUDIOMODULE_COMPONENTS_AUDIOSOURCECOMPONENT_H_

#include <chrono>  // NOLINT(build/c++11) (cpplint outside Chromium: https://github.com/google/styleguide/issues/483)
#include <memory>
#include <string>
#include <vector>

#include "Core/EntityComponentSystem/Component.h"
#include "Modules/AudioModule/AudioRingBuffer.h"

namespace nv3dvc {
namespace modules {
namespace audiomodule {
namespace components {

/// @defgroup AudioSourceComponentProperties AudioSourceComponent
/// @ingroup  ComponentProperties
/// @brief    Represents an audio stream that should be played through each AudioOutputComponent.
///
/// Acted on by the systems::AudioOutputSystem system

/// See @ref  AudioSourceComponentProperties
class AudioSourceComponent : public core::ecs::Component {
 public:
  constexpr static const char* NAME = "AudioSourceComponent";
  std::string Name() const override { return NAME; }

  AudioSourceComponent();
  ~AudioSourceComponent() override {}

  bool Initialize(int sample_rate, int num_channels);
  bool IsInitialized() const;
  int GetSampleRate() const;
  int GetNumChannels() const;

  /// @brief Push audio data into the component's internal ring buffer
  /// @param[in] src_data    Pointer to a buffer containing interleaved audio data
  /// @param[in] src_samples Duration of the buffer, measured in samples
  /// @return    Duration of audio that was successfully pushed, measured in samples
  int PushSourceAudio(const float* src_data, int src_samples);

  /// @brief Pull audio data out of the component's internal ring buffer
  /// @param[out] dst_data    Pointer to a buffer where interleaved audio data will be written
  /// @param[in]  dst_samples Duration of the buffer, measured in samples
  /// @return     Duration of audio that was successfully pulled, measured in samples
  int PopSourceAudio(float* dst_data, int dst_samples);

  /// @brief Set the list of additional ring buffers where the audio should be pushed when PushSourceAudio is called
  /// @param[in] ring_buffers The list of additional ring buffers
  void SetDestinationRingBuffers(const std::vector<std::shared_ptr<AudioRingBuffer>>& ring_buffers);

 private:
  void ClearOutdatedSamplesInAudioBuffers();

  bool m_isInitialized;

  std::chrono::steady_clock::time_point m_startTime;

  int m_sampleRate;
  int m_numChannels;

  std::unique_ptr<AudioRingBuffer> m_ringBuffer = nullptr;
  std::chrono::milliseconds m_maxLifeTimeOldSamplesMs = std::chrono::milliseconds{500};

  std::chrono::steady_clock::time_point m_lastReceivedSamplesTimestamp = std::chrono::steady_clock::time_point::min();

  std::vector<std::shared_ptr<AudioRingBuffer>> m_destinationRingBuffers;
  std::vector<float> m_tmpSourceBuffer;
};

}  // namespace components
}  // namespace audiomodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_AUDIOMODULE_COMPONENTS_AUDIOSOURCECOMPONENT_H_
