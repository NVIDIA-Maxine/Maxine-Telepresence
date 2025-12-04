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

#ifndef SRC_MODULES_CAPTUREMODULE_COMPONENTS_RECORDINGBEHAVIOR_H_
#define SRC_MODULES_CAPTUREMODULE_COMPONENTS_RECORDINGBEHAVIOR_H_

#include <string>

#include "Core/Error.h"
#include "Core/Properties/Property.h"
#include "Modules/BehaviorModule/Components/BehaviorComponent.h"

// Forward declarations
typedef struct _GstElement GstElement;

namespace nv3dvc {
namespace modules {
namespace capturemodule {
namespace components {

/// @defgroup RecordingBehaviorProperties RecordingBehavior
/// @ingroup  ComponentProperties
/// @brief    This component enables recording of the rendered frames to be saved as a video file
///
/// Assumes the same entity has a commonmodule::components::RecordingCallbackComponent attached

/// See @ref RecordingBehaviorProperties
class RecordingBehavior : public behaviormodule::components::BehaviorComponent {
 public:
  constexpr static const char* NAME = "RecordingBehavior";
  std::string Name() const override { return NAME; };

  RecordingBehavior() = default;
  ~RecordingBehavior() override = default;

  core::Error OnInitialize() override;
  core::Error OnUnloadScene() override;

  /// @brief Opens the stream for writing video and audio
  /// @return core::Error::SUCCESS if successful
  core::Error Start();

  /// @brief Closes the stream for writing video and audio
  /// @return core::Error::SUCCESS if successful
  core::Error Stop();

  /// @brief Set the video size for the output stream
  /// @param[in] width  The width
  /// @param[in] height The height
  /// @return    core::Error::SUCCESS if successful
  core::Error SetVideoSize(int width, int height);

  /// @brief Set the audio format for the output stream
  /// @param sample_rate  The desired sample rate
  /// @param num_channels The number of audio channels
  /// @return    core::Error::SUCCESS if successful
  core::Error SetAudioFormat(int sample_rate, int num_channels);

  /// @brief Push video data to the writer
  /// @param[in] src_rgba_pixels The video pixel data. Assumed to be RGBA U8
  /// @return    core::Error::SUCCESS if successful or if video size has not yet been configured
  core::Error PushVideoData(const uint8_t* src_rgba_pixels);

  /// @brief Push audio data to the writer
  /// @param[in] src_data    An array of audio samples
  /// @param[in] src_samples The number of audio samples in the array
  /// @return    core::Error::SUCCESS if successful or if audio has not yet been configured
  core::Error PushAudioData(const float* src_data, int src_samples);

 public:
  /// @ingroup RecordingBehaviorProperties
  /// @{
  core::properties::Property<std::string> file_path = {
      this,
      "file_path",
      "The file path of where to write the output video. Can be .mkv or .mp4",
      std::string("recording.mkv"),
  };
  core::properties::Property<bool> enable = {
      this,
      "enable",
      "Whether to write frames to video file. Opens output stream when set to true. Closes it when set to false",
      false,
  };
  /// @}

 private:
  static core::Error PushData(GstElement* appsrc, const void* src_data, size_t src_bytes);

  std::atomic<int> m_audioSampleRate = 0;
  std::atomic<int> m_audioNumChannels = 0;
  std::atomic<int> m_videoWidth = 0;
  std::atomic<int> m_videoHeight = 0;

  GstElement* m_pipeline = nullptr;
  GstElement* m_audiosrc = nullptr;
  GstElement* m_videosrc = nullptr;
};

}  // namespace components
}  // namespace capturemodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_CAPTUREMODULE_COMPONENTS_RECORDINGBEHAVIOR_H_
