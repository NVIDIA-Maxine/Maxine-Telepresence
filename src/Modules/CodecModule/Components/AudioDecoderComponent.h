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

#ifndef SRC_MODULES_CODECMODULE_COMPONENTS_AUDIODECODERCOMPONENT_H_
#define SRC_MODULES_CODECMODULE_COMPONENTS_AUDIODECODERCOMPONENT_H_

#include <memory>
#include <string>
#include <vector>

#include "Core/EntityComponentSystem/Component.h"
#include "Core/Error.h"

// Forward declaration
struct OpusDecoder;

namespace nv3dvc {
namespace modules {
namespace codecmodule {
namespace components {

/// @defgroup AudioDecoderComponentProperties AudioDecoderComponent
/// @ingroup  ComponentProperties
/// @brief    Component for decoding audio samples

/// See @ref AudioDecoderComponentProperties
class AudioDecoderComponent : public core::ecs::Component {
 public:
  constexpr static const char* NAME = "AudioDecoderComponent";
  std::string Name() const override { return NAME; };

  AudioDecoderComponent() = default;
  ~AudioDecoderComponent() = default;

  core::Error Initialize(double sample_rate, int channels);

  void SetDecodeCallback(std::function<void(const float* src_data, int src_samples)> callback);

  core::Error DecodeSamples(const unsigned char* data, int data_size);

 private:
  int m_sampleRate = 0;
  int m_numChannels = 0;
  // OpusDecoder pointer must be deleted with a library function.
  struct OpusDecoderDeleter {
    void operator()(OpusDecoder* st);
  };
  std::unique_ptr<OpusDecoder, OpusDecoderDeleter> m_opusDecoder;
  std::vector<float> m_tmpDecodeBuffer;
  std::function<void(const float* src_data, int src_samples)> m_decodeCallback;
};

}  // namespace components
}  // namespace codecmodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_CODECMODULE_COMPONENTS_AUDIODECODERCOMPONENT_H_
