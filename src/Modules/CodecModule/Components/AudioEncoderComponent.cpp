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

#include "AudioEncoderComponent.h"

#include <vector>

#include "Core/Util/Logger.h"
#include "opus.h"

namespace nv3dvc {
namespace modules {
namespace codecmodule {
namespace components {

static constexpr int kFrameDurationMilliSecs = 10;
static constexpr int kMinBitrate = 500;
static constexpr int kMaxBitrate = 512'000;

// Implement deleter for the OpusEncoder.
void AudioEncoderComponent::OpusEncoderDeleter::operator()(OpusEncoder* st) { opus_encoder_destroy(st); }

core::Error AudioEncoderComponent::Initialize(double sample_rate, int channels) {
  m_sampleRate = sample_rate;
  m_numChannels = channels;
  // Translate OpusMode to opus internal value.
  int opus_application;
  switch (*opus_mode.get()) {
    case OpusMode::VOIP:
      opus_application = OPUS_APPLICATION_VOIP;
      break;
    case OpusMode::AUDIO:
      opus_application = OPUS_APPLICATION_AUDIO;
      break;
    case OpusMode::RESTRICTED_LOWDELAY:
      opus_application = OPUS_APPLICATION_RESTRICTED_LOWDELAY;
      break;
    default:
      opus_application = OPUS_APPLICATION_AUDIO;
  }

  // Create Opus Encoder.
  int error;
  m_opusEncoder.reset(opus_encoder_create(m_sampleRate, m_numChannels, opus_application, &error));
  if (error != OPUS_OK) {
    return core::Error::ERR_GENERAL;
  }

  error = opus_encoder_ctl(m_opusEncoder.get(), OPUS_SET_BITRATE(int{target_bitrate}));
  if (error < 0) {
    LOG_ERROR("Opus encoder failed to set bitrate: %s", opus_strerror(error));
    return core::Error::ERR_GENERAL;
  }

  return core::Error::SUCCESS;
}

void AudioEncoderComponent::SetEncodeCallback(
    std::function<int(float* dst_data, int dst_samples, int frame_samples)> callback) {
  m_encodeCallback = callback;
}

core::Error AudioEncoderComponent::EncodeSamples(std::vector<std::vector<unsigned char>>* packets) {
  core::Error err = core::Error::SUCCESS;
  CHECK_NONNULL(packets, core::Error::ERR_NULL_POINTER);
  CHECK_NONNULL(m_encodeCallback, core::Error::ERR_INITIALIZATION);
  // As per Opus documentation, frame_samples is the duration of the frame in samples (per channel).
  const int frame_samples = (kFrameDurationMilliSecs * m_sampleRate) / 1000;

  packets->clear();

  m_tmpEncodeBuffer.resize(10.0f * m_sampleRate * m_numChannels);
  const int src_samples = m_encodeCallback(m_tmpEncodeBuffer.data(), m_tmpEncodeBuffer.size(), frame_samples);
  const int num_frames = src_samples / frame_samples;

  packets->resize(num_frames);
  const float* src_ptr = m_tmpEncodeBuffer.data();
  for (int i = 0; i < num_frames; ++i) {
    auto& packet = (*packets)[i];
    // As per Opus documentation, max_packet is the maximum number of bytes that can be written in the packet
    // (1276 bytes is recommended).
    const int max_packet = 1276;
    packet.resize(max_packet);
    const int result = opus_encode_float(m_opusEncoder.get(), src_ptr, frame_samples, packet.data(), max_packet);
    if (result > 0) {
      const int packet_bytes = result;
      packet.resize(packet_bytes);  // Shrink to fit.
    } else {
      LOG_ERROR("Opus encode failed: %s", opus_strerror(result));
    }
    src_ptr += frame_samples * m_numChannels;
  }
bail:
  return err;
}

}  // namespace components
}  // namespace codecmodule
}  // namespace modules
}  // namespace nv3dvc
