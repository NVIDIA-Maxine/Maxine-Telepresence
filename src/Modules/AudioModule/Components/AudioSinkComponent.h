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

#ifndef SRC_MODULES_AUDIOMODULE_COMPONENTS_AUDIOSINKCOMPONENT_H_
#define SRC_MODULES_AUDIOMODULE_COMPONENTS_AUDIOSINKCOMPONENT_H_

#include <memory>
#include <string>

#include "Core/EntityComponentSystem/Component.h"
#include "Modules/AudioModule/AudioRingBuffer.h"

namespace nv3dvc {
namespace modules {
namespace audiomodule {
namespace components {

/// @defgroup AudioSinkComponentProperties AudioSinkComponent
/// @ingroup  ComponentProperties
/// @brief    Represents an audio stream that should be captured from each AudioInputComponent and mixed together.
///
/// Acted on by the systems::AudioInputSystem system

/// See @ref  AudioSinkComponentProperties
class AudioSinkComponent : public core::ecs::Component {
 public:
  constexpr static const char* NAME = "AudioSinkComponent";
  std::string Name() const override { return NAME; }

  AudioSinkComponent();
  ~AudioSinkComponent() override {}

  bool Initialize();
  bool IsInitialized() const;
  int GetSampleRate() const;
  int GetNumChannels() const;

  /// @brief Push audio data into the component's internal ring buffer
  /// @param[in] src_data    Pointer to a buffer containing interleaved audio data
  /// @param[in] src_samples Duration of the buffer, measured in samples
  /// @return    Duration of audio that was successfully pushed, measured in samples
  int PushSinkAudio(const float* src_data, int src_samples);

  /// @brief Pull audio data out of the component's internal ring buffer
  /// @param[out] dst_data      Pointer to a buffer where interleaved audio data will be written
  /// @param[in]  dst_samples   Duration of the buffer, measured in samples
  /// @param[in]  frame_samples Minimum divisor for number of samples; the number pulled will be a multiple of this
  /// @return     Duration of audio that was successfully pulled, measured in samples
  int PopSinkAudio(float* dst_data, int dst_samples, int frame_samples);

 public:
  /// @ingroup AudioSinkComponentProperties
  /// @{
  core::properties::Property<int> sample_rate = {
      this,
      "sample_rate",
      "The desired sample rate in Hz",
      48'000,
  };
  core::properties::Property<int> num_channels = {
      this,
      "num_channels",
      "The desired number of channels",
      1,
  };
  /// @}
 private:
  bool m_isInitialized = false;
  std::unique_ptr<AudioRingBuffer> m_ringBuffer = nullptr;
};

}  // namespace components
}  // namespace audiomodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_AUDIOMODULE_COMPONENTS_AUDIOSINKCOMPONENT_H_
