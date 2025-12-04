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

#ifndef SRC_MODULES_AUDIOMODULE_AUDIORINGBUFFER_H_
#define SRC_MODULES_AUDIOMODULE_AUDIORINGBUFFER_H_

#include <cstdint>
#include <memory>
#include <vector>

struct PaUtilRingBuffer;

namespace nv3dvc {
namespace modules {
namespace audiomodule {

/// @brief A single-producer, single-consumer ring buffer, using PaUtilRingBuffer from PortAudio.
/// This is only suitable for interleaved audio samples, not non-interleaved (planar) samples.
class AudioRingBuffer {
 public:
  /// @brief Try to create an AudioRingBuffer with the provided parameters.
  /// @param  duration_seconds Duration of the ring buffer, in seconds.
  /// @param  sample_rate      Sample-rate of the audio that will be stored.
  /// @param  channel_count    Number of channels in the audio that will be stored.
  /// @return The created AudioRingBuffer if successful, otherwise nullptr.
  static std::unique_ptr<AudioRingBuffer> Create(float duration_seconds, int sample_rate, int channel_count);

  /// @brief Destructor
  ~AudioRingBuffer();

  /// @brief Push new audio data at the "back" of the ring buffer.
  /// @param  src_data Pointer to the buffer containing the source audio data that should be added.
  /// @param  src_size Number of values requested to push from @p src_data.
  /// @return The actual number of values that were pushed to the ring buffer.
  int Push(const float* const src_data, const int src_size);

  /// @brief Pop audio data from the "front" of the ring buffer.
  /// @param  dst_data Pointer to a buffer that the audio data should be copied into.
  /// @param  dst_size Number of values requested to pop to @p dst_data.
  /// @return The actual number of values that were popped from the ring buffer.
  int Pop(float* const dst_data, const int dst_size);

  int GetSampleRate() const;
  int GetNumChannels() const;
  int GetNumAvailableValues() const;
  int64_t GetTotalWrittenValues() const;

 private:
  AudioRingBuffer(float duration_seconds, int sample_rate, int channel_count);

  const float m_lengthSeconds;
  const int m_sampleRate;
  const int m_channelCount;
  std::unique_ptr<PaUtilRingBuffer> m_rbuf;
  std::vector<float> m_storage;
  int64_t m_totalWriteCount = 0, m_totalDropCount = 0;
};

}  // namespace audiomodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_AUDIOMODULE_AUDIORINGBUFFER_H_
