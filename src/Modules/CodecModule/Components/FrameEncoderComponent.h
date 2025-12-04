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

#ifndef SRC_MODULES_CODECMODULE_COMPONENTS_FRAMEENCODERCOMPONENT_H_
#define SRC_MODULES_CODECMODULE_COMPONENTS_FRAMEENCODERCOMPONENT_H_

#include <string>
#include <vector>

#include "Core/EntityComponentSystem/Component.h"
#include "Core/Error.h"
#include "Core/Properties/Property.h"
#include "Modules/CodecModule/VideoEncoder.h"

namespace nv3dvc {
namespace modules {
namespace codecmodule {
namespace components {

/// @defgroup FrameEncoderComponentProperties FrameEncoderComponent
/// @ingroup  ComponentProperties
/// @brief    Component for encoding video frames

/// See @ref FrameEncoderComponentProperties
class FrameEncoderComponent : public core::ecs::Component {
 public:
  constexpr static const char* NAME = "FrameEncoderComponent";
  std::string Name() const override { return NAME; };

  FrameEncoderComponent() = default;

  /// @brief Initialize the video decoder component
  /// @param[in] cu_context               The CUDA context
  /// @param[in] width                    The width [pixels] of the frames to encode
  /// @param[in] height                   The height [pixels] of the frames to encode
  /// @param[in] encode_options           Encoder options
  /// @param[in] format                   The frame input format
  /// @return    core::Error::SUCCESS     If successful
  core::Error Initialize(CUcontext cu_context, int width, int height, const NvEncoderInitParam& encode_options,
                         NV_ENC_BUFFER_FORMAT format);

  /// @brief Whether the component has been properly initialized
  /// @return Whether the component has been properly initialized
  bool IsInitialized() const;

  /// @brief Get a pointer to a location of where the next input frame can be written
  /// @return The next input frame location
  const NvEncInputFrame* GetNextInputFrame();

  /// @brief Encode the input frame into packages
  /// When calling this funciton, it is assumed that the data has been copied to the location returned to by
  /// GetNextInputFrame()
  /// @param[out] packages         The encoded packages
  /// @param[in] sei_payload       The SEI message payload, if any. Use empty vector for no SEI data
  /// @param[in] encode_last_frame Whether this is the last frame
  /// @return    core::Error::ERR_NULL_POINTER If nullptr was provided
  /// @return    core::Error::SUCCESS          If successful
  core::Error EncodeLowLatency(std::vector<std::vector<uint8_t>>* packets, std::vector<uint8_t>* sei_payload,
                               bool encode_last_frame = false);

 public:
  /// @ingroup FrameEncoderComponentProperties
  /// @{
  core::properties::Property<uint32_t> avg_bitrate = {
      this,
      "avg_bitrate",
      "Target average bitrate [bits/second]",
      5'000'000,
  };
  core::properties::Property<uint32_t> gop_length = {
      this,
      "gop_length",
      "The encoder GOP length (interval between I-frames)",
      NVENC_INFINITE_GOPLENGTH,
  };
  core::properties::Property<uint32_t> frame_interval_p = {
      this,
      "frame_interval_p",
      "The encoder GOP pattern, specified by the interval between P-frames. 1: IPPPPP, 2: IBPBPBP, 3: IBBPBBP etc",
      1,
  };
  core::properties::Property<uint32_t> frame_interval_idr = {
      this,
      "frame_interval_idr",
      "The interval at which to force the encoder to send an IDR-frame. Set to zero to disable.",
      15,
  };
  core::properties::Property<uint32_t> extra_output_delay = {
      this,
      "extra_output_delay",
      "Extra output frame delay",
      1,
  };
  core::properties::Property<std::string> debug_output_stream_file = {
      this,
      "debug_output_stream_file",
      "File path to a raw stream data file",
      "",
  };
  /// @}

 private:
  int m_idrCountdown = 0;
  bool m_isInitialized = false;

  VideoEncoder m_videoEncoder;
};

}  // namespace components
}  // namespace codecmodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_CODECMODULE_COMPONENTS_FRAMEENCODERCOMPONENT_H_
