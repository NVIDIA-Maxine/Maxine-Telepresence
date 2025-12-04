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

#ifndef SRC_MODULES_NETWORKMODULE_COMPONENTS_STREAMSINKCOMPONENT_H_
#define SRC_MODULES_NETWORKMODULE_COMPONENTS_STREAMSINKCOMPONENT_H_

#include <memory>
#include <string>

#include "Core/EntityComponentSystem/Component.h"
#include "Core/Error.h"
#include "Core/Properties/Property.h"
#include "Modules/NetworkModule/RtpClientSender.h"
#include "Modules/NetworkModule/RtspServer.h"

namespace nv3dvc {
namespace modules {
namespace networkmodule {

/// @ingroup EnumProperties
/// @defgroup StreamSinkType StreamSinkType
/// @brief Types of outbound media stream
/// @{
enum class StreamSinkType {
  UNKNOWN,         ///< Represents missing or unknown sink type
  RTP_UDP_SENDER,  ///< Send media using RTP over UDP to a specific receiver address
  RTSP_SERVER      ///< Create an RTSP server, to which receivers can connect
};
/// @}

namespace components {

/// @defgroup StreamSinkComponentProperties StreamSinkComponent
/// @ingroup  ComponentProperties
/// @brief    This component a multimedia stream that should be sent to a remote participant
///

/// See @ref StreamSinkComponentProperties
class StreamSinkComponent : public core::ecs::Component {
 public:
  constexpr static const char* NAME = "StreamSinkComponent";
  std::string Name() const override { return NAME; }

  ~StreamSinkComponent() override;

  core::Error Initialize();
  bool IsInitialized() const;

  core::Error SendVideoData(const uint8_t* data, size_t num_bytes);

  core::Error SendAudioData(const uint8_t* data, size_t num_bytes);

 public:
  /// @ingroup StreamSinkComponentProperties
  /// @{
  core::properties::Property<bool> send_audio = {
      this,
      "send_audio",
      "Whether to send an audio stream, in addition to the video stream",
      true,
  };
  core::properties::Property<StreamSinkType> sink_type = {
      this,
      "sink_type",
      "Which kind of outbound media stream to create",
      StreamSinkType::RTSP_SERVER,
  };
  core::properties::Property<std::string> udp_host = {
      this,
      "udp_host",
      "If sink_type == RTP_UDP_SENDER, the receiver's host address to send the media to",
      std::string("127.0.0.1"),
  };
  core::properties::Property<int> udp_port = {
      this,
      "udp_port",
      "If sink_type == RTP_UDP_SENDER, the receiver's port to send the media to",
      6000,
  };
  core::properties::Property<int> rtsp_port = {
      this,
      "rtsp_port",
      "If sink_type == RTSP_SERVER, the port of the RTSP server to create",
      6500,
  };
  core::properties::Property<std::string> rtsp_path = {
      this,
      "rtsp_path",
      "If sink_type == RTSP_SERVER, the path of the RTSP server to create",
      "/3dvc",
  };
  /// @}

 private:
  std::unique_ptr<RtpClientSender> m_sender = nullptr;
  std::unique_ptr<RtspServer> m_rtspServer = nullptr;
};

}  // namespace components
}  // namespace networkmodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_NETWORKMODULE_COMPONENTS_STREAMSINKCOMPONENT_H_
