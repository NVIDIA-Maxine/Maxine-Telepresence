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

#include "FrameDecoderComponent.h"

namespace nv3dvc {
namespace modules {
namespace codecmodule {
namespace components {

core::Error FrameDecoderComponent::Initialize(CUcontext cu_context, cudaVideoCodec codec,
                                              VideoDecoder::FrameOutputFormat output_format, bool force_zero_latency,
                                              bool decode_into_device_frame, bool low_latency_encoding,
                                              bool device_frame_pitched) {
  return m_videoDecoder.Initialize(cu_context, codec, output_format, force_zero_latency, decode_into_device_frame,
                                   low_latency_encoding, device_frame_pitched);
}

int FrameDecoderComponent::DecodeFrame(uint8_t* video_chunk, const size_t video_bytes, cudaStream_t stream, int flags) {
  return m_videoDecoder.DecodeFrame(video_chunk, video_bytes, stream, flags);
}

int FrameDecoderComponent::NumDecodedFramesAvailable() const { return m_videoDecoder.NumDecodedFramesAvailable(); }

core::Error FrameDecoderComponent::GetDecodedFrame(NvCVImage* output_frame) {
  return m_videoDecoder.GetDecodedFrame(output_frame);
}

triplanemodule::TriplanePackage FrameDecoderComponent::GetLastDecodedTriplanePackage() {
  return m_lastDecodedTriplanePackage;
}

void FrameDecoderComponent::SetLastDecodedTriplanePackage(const triplanemodule::TriplanePackage& triplane_package) {
  m_lastDecodedTriplanePackage = triplane_package;
}

}  // namespace components
}  // namespace codecmodule
}  // namespace modules
}  // namespace nv3dvc
