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

#ifndef SRC_MODULES_NETWORKMODULE_RTPCLIENTRECEIVER_H_
#define SRC_MODULES_NETWORKMODULE_RTPCLIENTRECEIVER_H_

#include <atomic>
#include <functional>
#include <string>
#include <thread>  // NOLINT(build/c++11) (cpplint outside Chromium: https://github.com/google/styleguide/issues/483)

#include "Core/Error.h"
#include "RtpClient.h"

// Forward declarations
typedef struct _GstElement GstElement;
typedef struct _GstCaps GstCaps;
typedef struct _GstMessage GstMessage;
typedef struct _GMainLoop GMainLoop;
typedef struct _GstBus GstBus;

namespace nv3dvc {
namespace modules {
namespace networkmodule {

class RtpClientReceiver : public RtpClient {
  struct PipelineControl : RtpClient::PipelineControl {
    std::atomic<bool> busThreadIsRunning = {false};
  };

 public:
  using AppSinkCallback =
      std::function<void(uint8_t* metadata_payload, const size_t metadata_size, uint8_t* buffer, const size_t size)>;

  RtpClientReceiver() = default;

  /// @brief Call before Initialize
  /// @param on_connect_callback The callback to call on the bus thread upon connect
  core::Error SetOnConnectCallback(const std::function<void(void)>& on_connect_callback);

  core::Error Initialize(const std::string& host_address = std::string("127.0.0.1"), int host_port = 5000,
                         const std::string& location = "3dvc", bool receive_audio = false,
                         bool decode_video_frames = false);
  core::Error SetAudioCallback(AppSinkCallback callback);
  core::Error SetVideoCallback(AppSinkCallback callback);

  bool HasVideoCallback() const;
  bool HasAudioCallback() const;
  bool NeedsReinitialization() const;

  core::Error PollVideoAppSink();
  core::Error PollAudioAppSink();

  bool ShouldRun();
  bool IsConnected();
  void Disconnect();
  bool IsInitialized() const;
  void Shutdown();

 private:
  constexpr static const char* NAME = "RtpClientReceiver";  // For named logging

  core::Error PollAppSink(const PipelineControl& control, MediaType media_type);

  /// @brief Spawns a thread for the bus message processing
  /// @param pipeline The gstreamer pipeline for which the bus thread will run
  /// @param control  The pipeline control struct to be accessed by the bus thread
  /// @param prefix   Name for identification
  void SpawnBusThread(GstElement* pipeline, PipelineControl* control, const std::string& prefix);

  /// @brief Joins the bus thread
  void JoinBusThread();

  /// @brief Process function for bus message control
  /// @param pipeline The gstreamer pipeline for which the bus thread is running
  /// @param control  The pipeline control struct accessed by the bus thread
  /// @param msg      The incoming message
  /// @param prefix   Name for identification
  /// @return         A network status code
  static core::Error BusProcessMsg(GstElement* pipeline, PipelineControl* control, GstMessage* msg,
                                   const std::string& prefix);

  /// @brief The thread execution function for the bus thread
  /// @param pipeline The gstreamer pipeline for which the bus thread is running
  /// @param control  The pipeline control struct accessed by the bus thread
  /// @param prefix   Name for identification
  static void CodeThreadBus(GstElement* pipeline, PipelineControl* control, const std::string& prefix);

  friend struct RtpClientReceiverCallbacks;

  // Pipeline elements
  GstElement* m_pipeline = nullptr;
  GstElement* m_src = nullptr;
  GstElement* m_vappsink = nullptr;
  GstElement* m_aappsink = nullptr;
  GstElement* m_vqueue = nullptr;
  GstElement* m_aqueue = nullptr;
  GstElement* m_vparser = nullptr;
  GstElement* m_aparser = nullptr;
  GstElement* m_vrtpDepay = nullptr;
  GstElement* m_artpDepay = nullptr;
  GstElement* m_capsfilter = nullptr;

  GstElement* m_vdecoder = nullptr;
  GstElement* m_vconverter = nullptr;
  GstElement* m_vcapsfilter = nullptr;

  GstBus* m_bus = nullptr;
  GstMessage* m_msg = nullptr;

  std::thread m_busThread;

  bool m_shouldReceiveAudio = false;
  bool m_decodeVideoFrames = false;
  int m_videoWidth = 0, m_videoHeight = 0;
  bool m_isInitialized = false;

  PipelineControl m_pipelineControl;

  AppSinkCallback m_videoSinkCallback;
  AppSinkCallback m_audioSinkCallback;
  std::chrono::steady_clock::time_point m_lastDataTime = std::chrono::steady_clock::time_point::min();
};

}  // namespace networkmodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_NETWORKMODULE_RTPCLIENTRECEIVER_H_
