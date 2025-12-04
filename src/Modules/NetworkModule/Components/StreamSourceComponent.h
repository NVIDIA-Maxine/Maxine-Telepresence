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

#ifndef SRC_MODULES_NETWORKMODULE_COMPONENTS_STREAMSOURCECOMPONENT_H_
#define SRC_MODULES_NETWORKMODULE_COMPONENTS_STREAMSOURCECOMPONENT_H_

#include <functional>
#include <string>

#include "Core/EntityComponentSystem/Component.h"
#include "Core/Error.h"
#include "Modules/NetworkModule/RtpClientReceiver.h"

namespace nv3dvc {
namespace modules {
namespace networkmodule {
namespace components {

/// @defgroup StreamSourceComponentProperties StreamSourceComponent
/// @ingroup  ComponentProperties
/// @brief    This component represents a multimedia stream that is received from a remote participant
///

/// See @ref StreamSourceComponentProperties
class StreamSourceComponent : public core::ecs::Component {
 public:
  constexpr static const char* NAME = "StreamSourceComponent";
  std::string Name() const override { return NAME; }

  ~StreamSourceComponent() override;

  core::Error Initialize();
  bool IsInitialized() const { return m_receiver.IsInitialized(); }

  core::Error SetVideoCallback(RtpClientReceiver::AppSinkCallback callback);
  core::Error SetAudioCallback(RtpClientReceiver::AppSinkCallback callback);
  core::Error SetOnConnectCallback(const std::function<void(void)>& callback);

  bool HasVideoCallback() const;
  bool HasAudioCallback() const;

  core::Error PollVideoAppSink();
  core::Error PollAudioAppSink();

  bool IsConnected() { return m_receiver.IsConnected(); }

 public:
  /// @ingroup StreamSourceComponentProperties
  /// @{
  core::properties::Property<std::string> rtsp_host = {
      this,
      "rtsp_host",
      "Host address (e.g. IP address) of the RTSP server to connect to",
      "127.0.0.1",
  };
  core::properties::Property<int> rtsp_port = {
      this,
      "rtsp_port",
      "Port of the RTSP server to connect to",
      6500,
  };
  core::properties::Property<std::string> rtsp_path = {
      this,
      "rtsp_path",
      "Path to the RTSP presentation on the server",
      "/3dvc",
  };
  core::properties::Property<bool> receive_audio = {
      this,
      "receive_audio",
      "Whether to receive an audio stream, in addition to the video stream",
      true,
  };
  core::properties::Property<bool> decode_video_frames = {
      this,
      "decode_video_frames",
      "Whether to decode video frames internally before outputting them",
      true,
  };  ///<.
      /// @}

 private:
  RtpClientReceiver m_receiver;
};

}  // namespace components
}  // namespace networkmodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_NETWORKMODULE_COMPONENTS_STREAMSOURCECOMPONENT_H_
