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

#include "VideoEncoder.h"

#include <vector>

#include "Core/Error.h"
#include "Core/Util/Logger.h"
#include "nvCVStatus.h"

namespace nv3dvc {
namespace modules {
namespace codecmodule {

VideoEncoder::~VideoEncoder() {
  if (m_enc) m_enc->DestroyEncoder();
}

core::Error VideoEncoder::Initialize(CUcontext cu_context, int width, int height, NvEncoderInitParam encode_options,
                                     NV_ENC_BUFFER_FORMAT format, uint32_t avg_bitrate, uint32_t gop_length,
                                     uint32_t frame_interval_p, uint32_t extra_output_delay) {
  core::Error err = core::Error::SUCCESS;
  BAIL_IF_TRUE(width == 0, err, static_cast<core::Error>(NVCV_ERR_RESOLUTION));
  BAIL_IF_TRUE(height == 0, err, static_cast<core::Error>(NVCV_ERR_RESOLUTION));
  m_cuContext = cu_context;
  m_format = format;

  try {
    // needs to be false for stream if not written to a container. fails for AV1 otherwise
    m_enc = std::make_shared<NvEncoderCuda>(cu_context, width, height,
                                            format,              // buffer format
                                            extra_output_delay,  // output delay (1 and higher enable better parallism)
                                            false,               // motion estimation only
                                            false,               // output in video memory
                                            false                // use iv container (important to be false for av1)
    );
  } catch (const std::exception& ex) {
    LOG_ERROR("Failed to create NvEncoderCuda: %s", ex.what());
    return core::Error::ERR_INITIALIZATION;
  }
  if (!m_enc) {
    LOG_ERROR("Failed to create NvEncoderCuda");
    return core::Error::ERR_INITIALIZATION;
  }

  m_initializeParams = {NV_ENC_INITIALIZE_PARAMS_VER};
  m_encodeConfig = {NV_ENC_CONFIG_VER};
  m_initializeParams.encodeConfig = &m_encodeConfig;
  m_enc->CreateDefaultEncoderParams(&m_initializeParams, encode_options.GetEncodeGUID(), encode_options.GetPresetGUID(),
                                    encode_options.GetTuningInfo());

  m_encodeConfig.gopLength = gop_length;
  m_encodeConfig.frameIntervalP = frame_interval_p;

  if (encode_options.IsCodecH264()) {
    m_encodeConfig.encodeCodecConfig.h264Config.idrPeriod = gop_length;
  } else if (encode_options.IsCodecHEVC()) {
    m_encodeConfig.encodeCodecConfig.hevcConfig.idrPeriod = gop_length;
  } else {
    m_encodeConfig.encodeCodecConfig.av1Config.idrPeriod = gop_length;
  }

  m_encodeConfig.rcParams.rateControlMode = NV_ENC_PARAMS_RC_CBR;
  m_encodeConfig.rcParams.multiPass = NV_ENC_TWO_PASS_FULL_RESOLUTION;
  m_encodeConfig.rcParams.averageBitRate = avg_bitrate;
  m_encodeConfig.rcParams.vbvBufferSize =
      (m_encodeConfig.rcParams.averageBitRate * m_initializeParams.frameRateDen / m_initializeParams.frameRateNum) * 5;
  m_encodeConfig.rcParams.maxBitRate = m_encodeConfig.rcParams.averageBitRate;
  m_encodeConfig.rcParams.vbvInitialDelay = m_encodeConfig.rcParams.vbvBufferSize;

  encode_options.SetInitParams(&m_initializeParams, format);

  m_enc->CreateEncoder(&m_initializeParams);

  m_frameSize = m_enc->GetFrameSize();

bail:
  return err;
}

const NvEncInputFrame* VideoEncoder::GetNextInputFrame() { return m_enc->GetNextInputFrame(); }

core::Error VideoEncoder::FlushEncoder(std::vector<std::vector<uint8_t>>* packages) {
  core::Error err = core::Error::SUCCESS;
  CHECK_NONNULL(packages, core::Error::ERR_NULL_POINTER);
  m_enc->FlushEncoder(*packages);
bail:
  return err;
}

core::Error VideoEncoder::EncodeLowLatency(std::vector<std::vector<uint8_t>>* packages,
                                           std::vector<uint8_t>* sei_payload, bool encode_last_frame,
                                           bool encode_sps_header, bool force_idr_frame) {
  core::Error err = core::Error::SUCCESS;

  // SEI data
  NV_ENC_SEI_PAYLOAD sei_data;
  // Params for one frame
  NV_ENC_PIC_PARAMS pic_params = {NV_ENC_PIC_PARAMS_VER};
  pic_params.encodePicFlags = 0;

  CHECK_NONNULL(packages, core::Error::ERR_NULL_POINTER);
  CHECK_NONNULL(sei_payload, core::Error::ERR_NULL_POINTER);

  // add SPS PPS header to allow decoder to pick up stream at any time
  // https://docs.nvidia.com/video-technologies/video-codec-sdk/11.1/nvenc-video-encoder-api-prog-guide/index.html
  if (encode_sps_header) {
    pic_params.encodePicFlags |= NV_ENC_PIC_FLAG_OUTPUT_SPSPPS;
  }
  if (force_idr_frame) {
    pic_params.encodePicFlags |= NV_ENC_PIC_FLAG_FORCEIDR;
  }
  if (!sei_payload->empty()) {
    sei_data.payloadType = 5;  // User Data Unregistered SEI Message
    sei_data.payloadSize = sei_payload->size();
    sei_data.payload = sei_payload->data();
    pic_params.codecPicParams.h264PicParams.seiPayloadArray = &sei_data;
    pic_params.codecPicParams.h264PicParams.seiPayloadArrayCnt = 1;
  }

  if (!encode_last_frame) {
    m_enc->EncodeFrame(*packages, &pic_params);
  } else {
    m_enc->EndEncode(*packages);
  }

  m_numFrames += packages->size();
bail:
  return err;
}

core::Error VideoEncoder::WriteEncodedPackages(const char* out_file_path,
                                               std::vector<std::vector<uint8_t>>* encoded_packages, bool append) {
  core::Error err = core::Error::SUCCESS;
  std::ofstream out_file;
  CHECK_NONNULL(out_file_path, core::Error::ERR_NULL_POINTER);
  CHECK_NONNULL(encoded_packages, core::Error::ERR_NULL_POINTER);

  std::ios_base::openmode options = std::ios::out | std::ios::binary | std::ios_base::app;
  if (append) {
    options |= std::ios_base::app;
  }
  out_file = std::ofstream(out_file_path, options);
  CHECK_TRUE(out_file.is_open(), core::Error::ERR_WRITE);

  for (std::vector<uint8_t>& packet : *encoded_packages) {
    out_file.write(reinterpret_cast<char*>(packet.data()), packet.size());
  }

  out_file.close();
bail:
  return err;
}

}  // namespace codecmodule
}  // namespace modules
}  // namespace nv3dvc
