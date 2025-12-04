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

#ifndef SRC_MODULES_CAPTUREMODULE_CAPTUREDEVICE_MULTIMEDIAFILE_H_
#define SRC_MODULES_CAPTUREMODULE_CAPTUREDEVICE_MULTIMEDIAFILE_H_

#include <atomic>
#include <string>

#include "CaptureDevice.h"
#include "nvCVImage.h"

// Forward declarations
typedef struct _GstElement GstElement;

namespace nv3dvc {
namespace modules {
namespace capturemodule {
namespace capturedevice {

/// @brief Gstreamer multimedia file capture device
class MultimediaFile : public ICaptureDevice {
 public:
  /// @brief Constructor
  /// @param[in] add_silent_audio_if_missing If the file does not contain audio, add a silent audio stream
  explicit MultimediaFile(bool add_silent_audio_if_missing);

  /// @brief Destructor
  ~MultimediaFile() override;

  /// @brief Initialize the capture device
  core::Error Initialize(const CameraDescriptor& camera_descriptor, uint32_t width, uint32_t height) override;

  /// @brief Whether the multimedia file contains a real audio stream
  ///
  /// If `add_silent_audio_if_missing` was set to true in the constructor, `HasAudio()` will return true even if the
  /// file does not contain a real audio stream.
  /// @return Whether the file contains a real audio stream
  bool FileContainsAudio() const;

  CaptureApi GetCaptureApi() const override;
  core::Error GetFrame(NvCVImage* frame, cudaStream_t stream) override;
  core::Error GetFramerate(float* framerate) const override;
  core::Error GetResolution(int* width, int* height) const override;
  bool HasAudio() const override;
  core::Error GetAudioFormat(int* sample_rate, int* num_channels) const override;
  core::Error PullAudioData(float* dst_data, int dst_samples, int* pulled_samples) override;

  core::Error StopCapture() override;

 private:
  static core::Error PullData(GstElement* appsink, void* dst_data, size_t dst_bytes, size_t* pulled_bytes);
  friend struct MultimediaFileCallbacks;

  bool m_isInitialized = false;
  CameraDescriptor m_cameraDescriptor;
  NvCVImage m_latestFrameRgbaPinned;

  std::string m_filePath;
  int m_audioSampleRate = 0;
  int m_audioNumChannels = 0;
  int m_videoWidth = 0;
  int m_videoHeight = 0;
  float m_videoFrameRate = 0.0f;
  bool m_addSilentAudioIfMissing = true;
  std::atomic<bool> m_fileContainsAudio = false;
  std::atomic<bool> m_waitForDecoderPads = false;
  std::atomic<bool> m_hasAudio = false;
  std::atomic<bool> m_hasVideo = false;

  GstElement* m_pipeline = nullptr;
  GstElement* m_decoder = nullptr;
  GstElement* m_audioSink = nullptr;
  GstElement* m_videoSink = nullptr;
};

}  // namespace capturedevice
}  // namespace capturemodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_CAPTUREMODULE_CAPTUREDEVICE_MULTIMEDIAFILE_H_
