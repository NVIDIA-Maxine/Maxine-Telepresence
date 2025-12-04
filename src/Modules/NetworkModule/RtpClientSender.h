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

#ifndef SRC_MODULES_NETWORKMODULE_RTPCLIENTSENDER_H_
#define SRC_MODULES_NETWORKMODULE_RTPCLIENTSENDER_H_

#include <atomic>
#include <string>

#include "Core/Error.h"
#include "RtpClient.h"

// Forward declarations
typedef struct _GstElement GstElement;

namespace nv3dvc {
namespace modules {
namespace networkmodule {

class RtpClientSender : public RtpClient {
  struct PipelineControl : RtpClient::PipelineControl {
    std::atomic<bool> is_live{true};         /// if is live, send the frames
    std::atomic<bool> resetEncoding{false};  /// Informs encoder to resend SPS header
  };

 public:
  RtpClientSender() = default;

  core::Error Initialize(const std::string& host_address = std::string("127.0.0.1"), int host_port = 5000,
                         bool send_audio = false);

  bool IsSenderReady();
  void InformEncodingReset();
  bool ShouldSenderResetEncoding();
  core::Error SendAudioData(const uint8_t* data, size_t num_bytes);
  core::Error SendVideoData(const uint8_t* data, size_t num_bytes);
  core::Error EndStream();
  void Shutdown();

 private:
  constexpr static const char* NAME = "RtpClientSender";  // For named logging

  core::Error SendData(const uint8_t* data, size_t num_bytes, MediaType media_type);

  GstElement* m_pipeline = nullptr;
  GstElement* m_audioSrc = nullptr;
  GstElement* m_videoSrc = nullptr;
  bool m_gotFirstAudioFrame = false, m_gotFirstVideoFrame = false;

  PipelineControl m_pipelineControl;
};

}  // namespace networkmodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_NETWORKMODULE_RTPCLIENTSENDER_H_
