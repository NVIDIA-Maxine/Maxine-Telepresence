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

#include "StreamSinkComponent.h"

#include <string>

#include "Core/Error.h"
#include "Core/Util/Logger.h"

namespace nv3dvc {
namespace modules {
namespace networkmodule {
namespace components {

StreamSinkComponent::~StreamSinkComponent() {
  if (m_sender) {
    m_sender->EndStream();
    m_sender->Shutdown();
  }
}

bool StreamSinkComponent::IsInitialized() const { return m_rtspServer || m_sender; }

core::Error StreamSinkComponent::Initialize() {
  core::Error err = core::Error::SUCCESS;
  if (IsInitialized()) {
    LOG_WARNING("StreamSinkComponent already initialized");
    return core::Error::ERR_GENERAL;
  }

  switch (sink_type) {
    case StreamSinkType::RTP_UDP_SENDER:
      m_sender = std::make_unique<RtpClientSender>();
      CHECK_SUCCESS(m_sender->Initialize(udp_host, udp_port, send_audio));
      break;
    case StreamSinkType::RTSP_SERVER:
      m_rtspServer = std::make_unique<RtspServer>();
      CHECK_SUCCESS(m_rtspServer->Initialize(rtsp_port, rtsp_path, send_audio));
      break;
    default:
      LOG_ERROR("Unknown stream sink type");
      BAIL(err, core::Error::ERR_INITIALIZATION);
  }

bail:
  if (err != core::Error::SUCCESS) {
    m_sender.reset();
    m_rtspServer.reset();
  }
  return err;
}

core::Error StreamSinkComponent::SendVideoData(const uint8_t* data, size_t num_bytes) {
  if (m_sender) {
    return m_sender->SendVideoData(data, num_bytes);
  } else if (m_rtspServer) {
    return m_rtspServer->SendVideoData(data, num_bytes);
  } else {
    return core::Error::ERR_INITIALIZATION;
  }
}

core::Error StreamSinkComponent::SendAudioData(const uint8_t* data, size_t num_bytes) {
  if (m_sender) {
    return m_sender->SendAudioData(data, num_bytes);
  } else if (m_rtspServer) {
    return m_rtspServer->SendAudioData(data, num_bytes);
  } else {
    return core::Error::ERR_INITIALIZATION;
  }
}

}  // namespace components
}  // namespace networkmodule
}  // namespace modules
}  // namespace nv3dvc
