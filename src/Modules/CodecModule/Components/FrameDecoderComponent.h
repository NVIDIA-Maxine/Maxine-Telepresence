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

#ifndef SRC_MODULES_CODECMODULE_COMPONENTS_FRAMEDECODERCOMPONENT_H_
#define SRC_MODULES_CODECMODULE_COMPONENTS_FRAMEDECODERCOMPONENT_H_

#include <string>

#include "Core/EntityComponentSystem/Component.h"
#include "Core/Error.h"
#include "Modules/CodecModule/VideoDecoder.h"
#include "Modules/TriplaneModule/TriplanePackage.h"

namespace nv3dvc {
namespace modules {
namespace codecmodule {
namespace components {

/// @defgroup FrameDecoderComponentProperties FrameDecoderComponent
/// @ingroup  ComponentProperties
/// @brief    Component for decoding video frames

/// See @ref FrameDecoderComponentProperties
class FrameDecoderComponent : public core::ecs::Component {
 public:
  constexpr static const char* NAME = "FrameDecoderComponent";
  std::string Name() const override { return NAME; };

  FrameDecoderComponent() = default;

  /// @brief See VideoDecoder::Initialize
  core::Error Initialize(CUcontext cu_context, cudaVideoCodec codec, VideoDecoder::FrameOutputFormat output_format,
                         bool force_zero_latency = false, bool decode_into_device_frame = true,
                         bool low_latency_encoding = true, bool device_frame_pitched = false);

  /// @brief See VideoDecoder::DecodeFrame
  int DecodeFrame(uint8_t* video_chunk, size_t video_bytes, cudaStream_t stream, int flags = CUVID_PKT_ENDOFPICTURE);

  /// @brief See VideoDecoder::NumDecodedFramesAvailable
  int NumDecodedFramesAvailable() const;

  /// @brief See VideoDecoder::GetDecodedFrame
  core::Error GetDecodedFrame(NvCVImage* output_frame);

  /// @brief Get the last decoded triplane package
  /// @return The last decoded triplane package
  triplanemodule::TriplanePackage GetLastDecodedTriplanePackage();

  /// @brief Set the last decoded triplane package
  /// @param triplane_package The last decoded triplane package to set
  void SetLastDecodedTriplanePackage(const triplanemodule::TriplanePackage& triplane_package);

 private:
  VideoDecoder m_videoDecoder;
  triplanemodule::TriplanePackage m_lastDecodedTriplanePackage;
};

}  // namespace components
}  // namespace codecmodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_CODECMODULE_COMPONENTS_FRAMEDECODERCOMPONENT_H_
