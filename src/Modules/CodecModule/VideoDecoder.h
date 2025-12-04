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

#ifndef SRC_MODULES_CODECMODULE_VIDEODECODER_H_
#define SRC_MODULES_CODECMODULE_VIDEODECODER_H_

#include <memory>

#include "Core/Error.h"
#include "NvDecoder/NvDecoder.h"
#include "cuda.h"
#include "driver_types.h"
#include "nvCVImage.h"

namespace nv3dvc {
namespace modules {
namespace codecmodule {

/// @brief Video decoder
///
/// Wrapper class for NvDecoder
class VideoDecoder {
 public:
  enum FrameOutputFormat { RGB8, RGBA8, Y8Planar, RGBFNorm, RGB8Planar, RGBFPlanarNorm, NV12 };

  /// @brief Initialize the video decoder
  /// @param[in] cu_context               The CUDA context
  /// @param[in] codec                    The CUDA video codec
  /// @param[in] output_format            The frame output format
  /// @param[in] force_zero_latency       Force zero latency
  /// @param[in] decode_into_device_frame Whether to decode into a CUDA device frame
  /// @param[in] low_latency_encoding     Low latency encoding
  /// @param[in] device_frame_pitched     Whether a pitched device pointer is used for the decoded frame
  /// @return    core::Error::SUCCESS     If successful
  core::Error Initialize(CUcontext cu_context, cudaVideoCodec codec, FrameOutputFormat output_format,
                         bool force_zero_latency, bool decode_into_device_frame, bool low_latency_encoding,
                         bool device_frame_pitched);

  /// @brief Decode a frame
  ///
  /// Expects there to be no extra frames available. If there is, call GetDecodedFrame first.
  /// @param[in] video_chunk The video package
  /// @param[in] video_bytes The number of bytes constituting the video package
  /// @param[in] stream      The CUDA stream on which to perform decoding
  /// @param[in] flags       CUVID flags
  /// @return    The number of decoded frames
  int DecodeFrame(uint8_t* video_chunk, const size_t& video_bytes, cudaStream_t stream,
                  int flags = CUVID_PKT_ENDOFPICTURE);

  /// @brief Get a decoded frame
  ///
  /// DecodeFrame needs to be called first.
  /// @param[out] output_frame A frame wrapped in an NvCVImage
  /// @return     core::Error::ERR_DATA_UNAVAILABLE If no frames are available
  /// @return     NVCV_ERR_PIXELFORMAT              If the format of the frame is not supported
  /// @return     core::Error::SUCCESS              If successful
  core::Error GetDecodedFrame(NvCVImage* output_frame);

  /// @brief Get the current number of available frames
  /// @return The current number of available frames
  int NumDecodedFramesAvailable() const { return m_numDecodedFramesAvailable; }

 private:
  FrameOutputFormat m_outputFormat = RGB8;
  CUcontext m_cuContext = nullptr;
  bool m_forceZeroLatency = false;

  int m_width = 0;
  int m_height = 0;

  int m_nVideoBytes = 0;
  int m_nFrameReturned = 0;
  int m_nFrame = 0;
  int m_matrix = 0;
  int m_presentationTimeframe = 0;
  int64_t m_timestamp = 0;

  std::shared_ptr<NvDecoder> m_dec = nullptr;
  int m_numDecodedFramesAvailable = 0;

  uint8_t* m_frame = nullptr;  // device pointer of decoded frame
};

}  // namespace codecmodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_CODECMODULE_VIDEODECODER_H_
