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

#ifndef SRC_MODULES_NETWORKMODULE_RTPCLIENT_H_
#define SRC_MODULES_NETWORKMODULE_RTPCLIENT_H_

#include <atomic>
#include <chrono>  // NOLINT(build/c++11) (cpplint outside Chromium: https://github.com/google/styleguide/issues/483)
#include <functional>
#include <string>

// Forward declarations
typedef struct _GstBus GstBus;
typedef struct _GstCaps GstCaps;
typedef struct _GstElement GstElement;
typedef struct _GstMessage GstMessage;

namespace nv3dvc {
namespace modules {
namespace networkmodule {

class RtpClient {
 public:
  enum MediaType { Audio, Video, Other };

 protected:
  /// @brief Control struct for multi threaded access.
  /// The bus thread may read/write pipeline control data
  struct PipelineControl {
    std::atomic<bool> active = {false};
    std::atomic<bool> shouldRun = {true};
    std::atomic<bool> isConnected = {false};

    void SetOnConnectCallback(const std::function<void(void)>& callback) { m_onConnect = callback; }
    void OnConnect() {
      if (m_onConnect) m_onConnect();
    }

   private:
    std::function<void(void)> m_onConnect;
  };

  std::string m_hostAddress = "127.0.0.1";
  int m_port = 0;

  std::atomic<bool> m_reportStats{true};
  std::chrono::steady_clock::time_point m_startStats = std::chrono::steady_clock::now();

  int m_reportIntervalSeconds = 1;
  size_t m_outgoingBytesTotal = 0;
  size_t m_outgoingBytesPeriod = 0;
  size_t m_incomingBytesTotal = 0;
  size_t m_incomingBytesPeriod = 0;
};

}  // namespace networkmodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_NETWORKMODULE_RTPCLIENT_H_
