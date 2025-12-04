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

#include "StreamSourceComponent.h"

#include "Core/Error.h"

namespace nv3dvc {
namespace modules {
namespace networkmodule {
namespace components {

StreamSourceComponent::~StreamSourceComponent() {
  m_receiver.Disconnect();
  m_receiver.Shutdown();
}

core::Error StreamSourceComponent::Initialize() {
  return m_receiver.Initialize(rtsp_host, rtsp_port, rtsp_path, receive_audio, decode_video_frames);
}

core::Error StreamSourceComponent::SetVideoCallback(RtpClientReceiver::AppSinkCallback callback) {
  return m_receiver.SetVideoCallback(callback);
}

core::Error StreamSourceComponent::SetAudioCallback(RtpClientReceiver::AppSinkCallback callback) {
  return m_receiver.SetAudioCallback(callback);
}

core::Error StreamSourceComponent::SetOnConnectCallback(const std::function<void(void)>& callback) {
  return m_receiver.SetOnConnectCallback(callback);
}

bool StreamSourceComponent::HasVideoCallback() const { return m_receiver.HasVideoCallback(); }

bool StreamSourceComponent::HasAudioCallback() const { return m_receiver.HasAudioCallback(); }

core::Error StreamSourceComponent::PollVideoAppSink() {
  core::Error err = core::Error::SUCCESS;
  if (m_receiver.NeedsReinitialization()) CHECK_SUCCESS(Initialize());  // Initialize() calls Shutdown() if necessary.
  err = m_receiver.PollVideoAppSink();
bail:
  return err;
}

core::Error StreamSourceComponent::PollAudioAppSink() {
  core::Error err = core::Error::SUCCESS;
  // NOTE: We don't check `m_receiver.NeedsReinitialization()` here as the audio sink runs on a separate thread from
  // `PollVideoAppSink()`. We assume that a video sink exists, and will handle the reinitialization if necessary.
  err = m_receiver.PollAudioAppSink();
bail:
  return err;
}

}  // namespace components
}  // namespace networkmodule
}  // namespace modules
}  // namespace nv3dvc
