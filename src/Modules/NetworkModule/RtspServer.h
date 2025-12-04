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

#ifndef SRC_MODULES_NETWORKMODULE_RTSPSERVER_H_
#define SRC_MODULES_NETWORKMODULE_RTSPSERVER_H_

#include <atomic>
#include <string>
#include <thread>  // NOLINT(build/c++11) (cpplint outside Chromium: https://github.com/google/styleguide/issues/483)

#include "Core/Error.h"

// Forward declarations
typedef struct _GstElement GstElement;
typedef struct _GMainContext GMainContext;
typedef struct _GMainLoop GMainLoop;
typedef struct _GstRTSPServer GstRTSPServer;

namespace nv3dvc {
namespace modules {
namespace networkmodule {

class RtspServer {
 public:
  RtspServer() = default;
  ~RtspServer();

  core::Error Initialize(int out_port, const std::string& mount_point, bool send_audio);

  core::Error SendAudioData(const uint8_t* data, size_t num_bytes);
  core::Error SendVideoData(const uint8_t* data, size_t num_bytes);

 private:
  friend struct RtspServerCallbacks;

  core::Error SendData(const uint8_t* data, size_t num_bytes, GstElement* appsrc);

  GstElement* m_audioSrc = nullptr;
  GstElement* m_videoSrc = nullptr;
  bool m_sendAudio = false;
  bool m_gotFirstAudioFrame = false, m_gotFirstVideoFrame = false;
  std::atomic<bool> m_connected = false;

  GMainContext* m_context = nullptr;
  GMainLoop* m_loop = nullptr;
  GstRTSPServer* m_server = nullptr;

  std::thread m_thread;
};

}  // namespace networkmodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_NETWORKMODULE_RTSPSERVER_H_
