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

#ifndef SRC_MODULES_CODECMODULE_VIDEOENCODER_H_
#define SRC_MODULES_CODECMODULE_VIDEOENCODER_H_

#include <memory>
#include <vector>

#include "../Interface/nvEncodeAPI.h"
#include "../Utils/NvEncoderCLIOptions.h"
#include "Core/Error.h"
#include "NvEncoder/NvEncoder.h"
#include "NvEncoder/NvEncoderCuda.h"
#include "cuda.h"

namespace nv3dvc {
namespace modules {
namespace codecmodule {

/// @brief Video encoder
///
/// Wrapper class for NvEncoder
class VideoEncoder {
 public:
  VideoEncoder() = default;
  ~VideoEncoder();

  /// @brief Initialize the video decoder
  /// @param[in] cu_context               The CUDA context
  /// @param[in] width                    The width [pixels] of the frames to encode
  /// @param[in] height                   The height [pixels] of the frames to encode
  /// @param[in] encode_options           Encoder options
  /// @param[in] format                   The frame input format
  /// @param[in] avg_bitrate              Target average bitrate [bits/second]
  /// @param[in] gop_length               The GOP frame length
  /// @param[in] frame_interval_p         The frame interval between p-frames
  /// @param[in] extra_output_delay       Extra output frame delay
  /// @return    core::Error::SUCCESS     If successful
  core::Error Initialize(CUcontext cu_context, int width, int height, NvEncoderInitParam encode_options,
                         NV_ENC_BUFFER_FORMAT format = NV_ENC_BUFFER_FORMAT_IYUV, uint32_t avg_bitrate = 0,
                         uint32_t gop_length = NVENC_INFINITE_GOPLENGTH, uint32_t frame_interval_p = 1,
                         uint32_t extra_output_delay = 1);

  /// @brief Get a pointer to a location of where the next input frame can be written
  /// @return The next input frame location
  const NvEncInputFrame* GetNextInputFrame();

  /// @brief Encode all remaining packages
  /// @param[out] packages The encoded packages
  /// @return     core::Error::SUCCESS If successful
  core::Error FlushEncoder(std::vector<std::vector<uint8_t>>* packages);

  /// @brief Encode the input frame into packages
  ///
  /// When calling this function, it is assumed that the data has been copied to the location returned to by
  /// GetNextInputFrame().
  /// @param[out] packages         The encoded packages
  /// @param[in] sei_payload       The SEI message payload, if any. Use empty vector for no SEI data
  /// @param[in] encode_last_frame Whether this is the last frame
  /// @param[in] encode_sps_header Whether to include the SPS header (automatically attached to IDR frames)
  /// @param[in] force_idr_frame   Whether to force an IDR frame
  /// @return    core::Error::ERR_NULL_POINTER If nullptr was provided
  /// @return    core::Error::SUCCESS          If successful
  core::Error EncodeLowLatency(std::vector<std::vector<uint8_t>>* packages, std::vector<uint8_t>* sei_payload,
                               bool encode_last_frame = false, bool encode_sps_header = false,
                               bool force_idr_frame = false);

  /// @brief Utility function for outputting encoded packages to file
  /// @param[in] out_file_path    Output file path
  /// @param[in] encoded_packages The byte packages to write
  /// @param[in] append           Wheter to write all the packages to a clean file or to append an existing file.
  ///                             Append also works for new files.
  /// @return    core::Error::ERR_NULL_POINTER If nullptr was provided
  /// @return    core::Error::ERR_WRITE        If writing the file failed
  /// @return    core::Error::SUCCESS          If successful
  core::Error WriteEncodedPackages(const char* out_file_path, std::vector<std::vector<uint8_t>>* encoded_packages,
                                   bool append);

 private:
  CUcontext m_cuContext = nullptr;
  size_t m_numFrames = 0;
  int m_frameSize = 0;

  std::shared_ptr<NvEncoderCuda> m_enc = nullptr;

  NV_ENC_INITIALIZE_PARAMS m_initializeParams = {};
  NV_ENC_BUFFER_FORMAT m_format = NV_ENC_BUFFER_FORMAT_UNDEFINED;
  NV_ENC_CONFIG m_encodeConfig = {};
};

}  // namespace codecmodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_CODECMODULE_VIDEOENCODER_H_
