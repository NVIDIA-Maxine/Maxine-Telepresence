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

#ifndef SRC_MODULES_CODECMODULE_COMPONENTS_AUDIOENCODERCOMPONENT_H_
#define SRC_MODULES_CODECMODULE_COMPONENTS_AUDIOENCODERCOMPONENT_H_

#include <memory>
#include <string>
#include <vector>

#include "Core/EntityComponentSystem/Component.h"
#include "Core/Error.h"

struct OpusEncoder;

namespace nv3dvc {
namespace modules {
namespace codecmodule {

/// @defgroup OpusMode OpusMode
/// @ingroup  EnumProperties
/// @brief    Opus mode
/// @{
enum class OpusMode {
  VOIP,                 ///< Best for most VoIP/videoconference applications
  AUDIO,                ///< Best for broadcast/high-fidelity applications
  RESTRICTED_LOWDELAY,  ///< Only use when lowest-achievable latency is what matters most
};
/// @}

namespace components {

/// @defgroup AudioEncoderComponentProperties AudioEncoderComponent
/// @ingroup  ComponentProperties
/// @brief    Component for encoding audio samples

/// See @ref AudioEncoderComponentProperties
class AudioEncoderComponent : public core::ecs::Component {
 public:
  constexpr static const char* NAME = "AudioEncoderComponent";
  std::string Name() const override { return NAME; };

  AudioEncoderComponent() = default;
  ~AudioEncoderComponent() = default;

  core::Error Initialize(double sample_rate, int channels);

  void SetEncodeCallback(std::function<int(float* dst_data, int dst_samples, int frame_samples)> callback);

  core::Error EncodeSamples(std::vector<std::vector<unsigned char>>* packets);

 public:
  /// @ingroup AudioEncoderComponentProperties
  /// @{
  core::properties::Property<OpusMode> opus_mode = {
      this,
      "opus_mode",
      "The Opus encoder coding mode, based on the intended application",
      OpusMode::AUDIO,
  };
  core::properties::Property<int> target_bitrate = {
      this,
      "target_bitrate",
      "Target average bitrate [bits/second]",
      128'000,
  };
  /// @}

 private:
  int m_sampleRate = 0;
  int m_numChannels = 0;

  /// @brief OpusEncoder pointer must be deleted with a library function.
  struct OpusEncoderDeleter {
    void operator()(OpusEncoder* st);
  };
  std::unique_ptr<OpusEncoder, OpusEncoderDeleter> m_opusEncoder = nullptr;
  std::vector<float> m_tmpEncodeBuffer;
  std::function<int(float* dst_data, int dst_samples, int frame_samples)> m_encodeCallback;
};

}  // namespace components
}  // namespace codecmodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_CODECMODULE_COMPONENTS_AUDIOENCODERCOMPONENT_H_
