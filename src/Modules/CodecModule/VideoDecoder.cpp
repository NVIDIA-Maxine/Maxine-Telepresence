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

#include "VideoDecoder.h"

#include <cassert>
#include <memory>

#include "Core/Error.h"
#include "Core/Util/Logger.h"
#include "nvCVImage.h"
#include "nvCVStatus.h"

namespace nv3dvc {
namespace modules {
namespace codecmodule {

core::Error VideoDecoder::Initialize(CUcontext cu_context, cudaVideoCodec codec, FrameOutputFormat output_format,
                                     bool force_zero_latency, bool decode_into_device_frame, bool low_latency_encoding,
                                     bool device_frame_pitched) {
  m_cuContext = cu_context;
  m_forceZeroLatency = force_zero_latency;

  m_outputFormat = output_format;

  const Rect* crop_rect = nullptr;
  const Dim* resize_dim = nullptr;

  m_dec = std::make_shared<NvDecoder>(m_cuContext, decode_into_device_frame, codec, low_latency_encoding,
                                      device_frame_pitched, crop_rect, resize_dim,
                                      false,  // extract SEI messages
                                      0, 0,   // max video width and height
                                      1000,   // clock rate
                                      m_forceZeroLatency);
  return core::Error::SUCCESS;
}

int VideoDecoder::DecodeFrame(uint8_t* video_chunk, const size_t& video_bytes, cudaStream_t stream, int flags) {
  // The function NvDecoder::Decode requires that "All frames that are available for display should be read before
  // making a subsequent decode call".
  assert(m_numDecodedFramesAvailable == 0);
  m_nFrameReturned = m_dec->Decode(video_chunk, video_bytes, flags, m_presentationTimeframe++);
  if (m_nFrameReturned > 0) {
    if (m_nFrame == 0) {
      m_matrix = m_dec->GetVideoFormatInfo().video_signal_description.matrix_coefficients;
      m_width = m_dec->GetWidth();
      m_height = m_dec->GetHeight();
      LOG_DEBUG("VideoDecoder first frame %d x %d", m_width, m_height);
    }
    m_nFrame += m_nFrameReturned;
    m_numDecodedFramesAvailable += m_nFrameReturned;
  }
  return m_nFrameReturned;
}

core::Error VideoDecoder::GetDecodedFrame(NvCVImage* output_frame) {
  core::Error err = core::Error::SUCCESS;
  CHECK_TRUE(m_numDecodedFramesAvailable > 0, core::Error::ERR_DATA_UNAVAILABLE);
  m_numDecodedFramesAvailable--;
  m_frame = m_dec->GetFrame(&m_timestamp);
  CHECK_NONNULL(m_frame, core::Error::ERR_NULL_POINTER);
  CHECK_TRUE(m_dec->GetOutputFormat() == cudaVideoSurfaceFormat_NV12, static_cast<core::Error>(NVCV_ERR_PIXELFORMAT));
  if (m_outputFormat == FrameOutputFormat::Y8Planar) {
    int pitch = m_width;
    CHECK_NVCV_SUCCESS(NvCVImage_Init(output_frame, m_width, m_height, pitch, m_frame, NvCVImage_PixelFormat::NVCV_Y,
                                      NvCVImage_ComponentType::NVCV_U8, NVCV_PLANAR, NVCV_CUDA));
  } else if (m_outputFormat == FrameOutputFormat::NV12) {
    int pitch = m_width;
    CHECK_NVCV_SUCCESS(NvCVImage_Init(output_frame, m_width, m_height, pitch, m_frame,
                                      NvCVImage_PixelFormat::NVCV_YUV420, NvCVImage_ComponentType::NVCV_U8, NVCV_NV12,
                                      NVCV_CUDA));
  } else {
    BAIL(err, static_cast<core::Error>(NVCV_ERR_PIXELFORMAT));
  }
bail:
  return err;
}

}  // namespace codecmodule
}  // namespace modules
}  // namespace nv3dvc
