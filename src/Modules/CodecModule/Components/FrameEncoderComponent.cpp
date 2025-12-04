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

#include "FrameEncoderComponent.h"

#include "Core/Error.h"

namespace nv3dvc {
namespace modules {
namespace codecmodule {
namespace components {

core::Error FrameEncoderComponent::Initialize(CUcontext cu_context, int width, int height,
                                              const NvEncoderInitParam& encode_options, NV_ENC_BUFFER_FORMAT format) {
  core::Error err = core::Error::SUCCESS;

  CHECK_FALSE(m_isInitialized, core::Error::ERR_INITIALIZATION);
  CHECK_SUCCESS(m_videoEncoder.Initialize(cu_context, width, height, encode_options, format, avg_bitrate, gop_length,
                                          frame_interval_p, extra_output_delay));
  m_isInitialized = true;

bail:
  return err;
}

bool FrameEncoderComponent::IsInitialized() const { return m_isInitialized; }

const NvEncInputFrame* FrameEncoderComponent::GetNextInputFrame() { return m_videoEncoder.GetNextInputFrame(); }

core::Error FrameEncoderComponent::EncodeLowLatency(std::vector<std::vector<uint8_t>>* packets,
                                                    std::vector<uint8_t>* sei_payload, bool encode_last_frame) {
  core::Error err = core::Error::SUCCESS;
  CHECK_NONNULL(packets, core::Error::ERR_NULL_POINTER);
  CHECK_NONNULL(sei_payload, core::Error::ERR_NULL_POINTER);

  bool encode_sps_header = false;
  bool force_idr_frame = false;
  if (frame_interval_idr > 0 && m_idrCountdown <= 0) {
    encode_sps_header = true;
    force_idr_frame = true;
    m_idrCountdown = frame_interval_idr;
  }

  CHECK_SUCCESS(
      m_videoEncoder.EncodeLowLatency(packets, sei_payload, encode_last_frame, encode_sps_header, force_idr_frame));
  m_idrCountdown -= packets->size();
  if (!debug_output_stream_file.get()->empty()) {
    err = m_videoEncoder.WriteEncodedPackages(debug_output_stream_file.get()->c_str(), packets, true);
  }
bail:
  return err;
}

}  // namespace components
}  // namespace codecmodule
}  // namespace modules
}  // namespace nv3dvc
