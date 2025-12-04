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

#include "AudioDecoderComponent.h"

#include <opus.h>

#include "Core/Error.h"
#include "Core/Util/Logger.h"

namespace nv3dvc {
namespace modules {
namespace codecmodule {
namespace components {

// Implement deleter for the OpusDecoder.
void AudioDecoderComponent::OpusDecoderDeleter::operator()(OpusDecoder* st) { opus_decoder_destroy(st); }

core::Error AudioDecoderComponent::Initialize(double sample_rate, int channels) {
  m_sampleRate = static_cast<int>(sample_rate);
  m_numChannels = channels;

  // Create Opus decoder.
  int error;
  m_opusDecoder.reset(opus_decoder_create(static_cast<opus_int32>(sample_rate), channels, &error));
  if (error != OPUS_OK) {
    return core::Error::ERR_GENERAL;
  }

  return core::Error::SUCCESS;
}

void AudioDecoderComponent::SetDecodeCallback(std::function<void(const float* src_data, int src_samples)> callback) {
  m_decodeCallback = callback;
}

core::Error AudioDecoderComponent::DecodeSamples(const unsigned char* const data, const int data_size) {
  core::Error err = core::Error::SUCCESS;
  CHECK_NONNULL(data, core::Error::ERR_NULL_POINTER);

  LOG_VERBOSE("Decoding %d bytes of audio", data_size);

  // Decode buffer should be able to hold 120ms.
  const int out_size = (120 * m_sampleRate) / 1000;
  m_tmpDecodeBuffer.resize(out_size * m_numChannels);
  constexpr int kDecodeFEC = 0;
  const int result =
      opus_decode_float(m_opusDecoder.get(), data, data_size, m_tmpDecodeBuffer.data(), out_size, kDecodeFEC);

  if (result < 0) {
    LOG_ERROR("Opus decode failed: %s", opus_strerror(result));
    return core::Error::ERR_GENERAL;
  }

  if (m_decodeCallback) {
    const int num_samples = result;
    m_decodeCallback(m_tmpDecodeBuffer.data(), num_samples);
  }
bail:
  return err;
}

}  // namespace components
}  // namespace codecmodule
}  // namespace modules
}  // namespace nv3dvc
