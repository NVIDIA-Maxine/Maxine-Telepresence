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

#include "AudioRingBuffer.h"

#include <pa_ringbuffer.h>

#include "Core/Util/Logger.h"

namespace nv3dvc {
namespace modules {
namespace audiomodule {

inline unsigned int NextPowerOf2(unsigned int val) {
  val--;
  val = (val >> 1) | val;
  val = (val >> 2) | val;
  val = (val >> 4) | val;
  val = (val >> 8) | val;
  val = (val >> 16) | val;
  return ++val;
}

AudioRingBuffer::AudioRingBuffer(float duration_seconds, int sample_rate, int channel_count)
    : m_lengthSeconds(duration_seconds),
      m_sampleRate(sample_rate),
      m_channelCount(channel_count),
      m_rbuf(std::make_unique<PaUtilRingBuffer>()) {}

AudioRingBuffer::~AudioRingBuffer() {}

std::unique_ptr<AudioRingBuffer> AudioRingBuffer::Create(float duration_seconds, int sample_rate, int channel_count) {
  if (duration_seconds < 0.0f || sample_rate <= 0 || channel_count <= 0) return nullptr;

  std::unique_ptr<AudioRingBuffer> ring_buffer(new AudioRingBuffer(duration_seconds, sample_rate, channel_count));

  const int min_element_count = sample_rate * duration_seconds * channel_count;
  unsigned int element_count = NextPowerOf2(static_cast<unsigned int>(min_element_count));
  ring_buffer->m_storage.resize(element_count);

  // create ring buffer
  if (PaUtil_InitializeRingBuffer(ring_buffer->m_rbuf.get(), sizeof(float), element_count,
                                  ring_buffer->m_storage.data()) < 0) {
    LOG_ERROR("Failed to initialize ring buffer");
    return nullptr;
  }

  return ring_buffer;
}

int AudioRingBuffer::Push(const float* const src_data, const int src_size) {
  if (src_size <= 0) return 0;

  const ring_buffer_size_t write_count = PaUtil_WriteRingBuffer(m_rbuf.get(), src_data, src_size);
  if (write_count < src_size) {
    m_totalDropCount += src_size - static_cast<int>(write_count);
  }
  // The `m_totalWriteCount` counts all values, even if they were dropped.
  // This means it can be used as a clock.
  m_totalWriteCount += src_size;
  return static_cast<int>(write_count);
}

int AudioRingBuffer::Pop(float* const dst_data, const int dst_size) {
  if (dst_size <= 0) return 0;

  memset(dst_data, 0.0f, sizeof(float) * dst_size);
  const ring_buffer_size_t read_count = PaUtil_ReadRingBuffer(m_rbuf.get(), dst_data, dst_size);
  if (read_count < dst_size) {
    // LOG_VERBOSE("Ring buffer underflowing");
  }
  return static_cast<int>(read_count);
}

int AudioRingBuffer::GetSampleRate() const { return m_sampleRate; }

int AudioRingBuffer::GetNumChannels() const { return m_channelCount; }

int AudioRingBuffer::GetNumAvailableValues() const {
  return static_cast<int>(PaUtil_GetRingBufferReadAvailable(m_rbuf.get()));
}

int64_t AudioRingBuffer::GetTotalWrittenValues() const { return m_totalWriteCount; }

}  // namespace audiomodule
}  // namespace modules
}  // namespace nv3dvc
