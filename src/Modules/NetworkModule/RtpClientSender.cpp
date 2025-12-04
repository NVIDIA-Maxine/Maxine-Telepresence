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

#include "RtpClientSender.h"

#include <gst/app/gstappsrc.h>
#include <gst/gst.h>

#include <chrono>  // NOLINT(build/c++11) (cpplint outside Chromium: https://github.com/google/styleguide/issues/483)
#include <iostream>
#include <sstream>

#include "Core/Error.h"
#include "Core/Util/Logger.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Macros                                                                                                           ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Static function declarations                                                                                     ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Static function definitions                                                                                      ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace nv3dvc {
namespace modules {
namespace networkmodule {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Member function definitions                                                                                      ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

core::Error RtpClientSender::Initialize(const std::string& host_address, int host_port, bool send_audio) {
  core::Error err = core::Error::SUCCESS;

  m_hostAddress = host_address;
  m_port = host_port;

  GError* gerror = nullptr;
  GstStateChangeReturn state_change;
  // Build complete command.
  std::ostringstream cmd;
  // Video appsrc sends video packets to the muxer.
  cmd << " appsrc name=videosrc"
         "        format=time"
         "        is-live=true"
         "        stream-type=stream"
         "        do-timestamp=true"
         "        emit-signals=true"
         "        caps=video/x-h264,stream-format=byte-stream,alignment=au,colorimetry=bt709"
         " ! h264parse"
         " ! queue"
         " ! mux.";
  if (send_audio) {
    // Audio appsrc sends audio packets to the muxer.
    cmd << " appsrc name=audiosrc"
           "        format=time"
           "        is-live=true"
           "        stream-type=stream"
           "        do-timestamp=true"
           "        caps=audio/x-opus"
           " ! opusparse"
           " ! queue"
           " ! mux.";
  }
  // Muxer connects to demuxer. This bottleneck should ensure video and audio stay in sync.
  cmd << " matroskamux name=mux"  //
         " ! queue"
         " ! matroskademux name=demux";
  // Video packets from demuxer will go to RTP over UDP.
  cmd << " demux."
         " ! video/x-h264"
         " ! queue"
         " ! rtph264pay mtu=65507"
         "              config-interval=1"
         "              aggregate-mode=1"
         " ! udpsink sync=false"
         "           buffer-size=65536"
         "           max-lateness=10000000"
      << "           host=" << m_hostAddress  //
      << "           port=" << m_port;
  if (send_audio) {
    // Audio packets from demuxer will go to RTP over UDP.
    cmd << " demux."
           " ! audio/x-opus"
           " ! queue"
           " ! rtpopuspay mtu=65507"
           " ! udpsink sync=false"
           "           buffer-size=65536"
           "           max-lateness=10000000"
        << "           host=" << m_hostAddress  //
        << "           port=" << m_port + 1;
  }

  LOG_DEBUG("Launching GStreamer pipeline: %s", cmd.str().c_str());
  m_pipeline = gst_parse_launch(cmd.str().c_str(), &gerror);
  CHECK_NULL(gerror, core::Error::ERR_VIDEO_CONFIGURATION, "Error constructing playback pipeline: %s", gerror->message);
  CHECK_NONNULL(m_pipeline, core::Error::ERR_VIDEO_CONFIGURATION, "Failed to create pipeline");

  // Get the elements that we need to interact with.
  m_videoSrc = gst_bin_get_by_name(GST_BIN(m_pipeline), "videosrc");
  CHECK_NONNULL(m_videoSrc, core::Error::ERR_VIDEO_STREAM_ELEMENT_CREATION, "Failed to get media pipeline elements");
  if (send_audio) {
    m_audioSrc = gst_bin_get_by_name(GST_BIN(m_pipeline), "audiosrc");
    CHECK_NONNULL(m_audioSrc, core::Error::ERR_AUDIO_STREAM_ELEMENT_CREATION, "Failed to get media pipeline elements");
  }

  // Start pipeline
  state_change = gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
  CHECK_FALSE(state_change == GST_STATE_CHANGE_FAILURE, core::ERR_STREAM_STATE_CHANGE);
  m_pipelineControl.active.store(true);

bail:
  if (err != core::Error::SUCCESS) {
    gst_object_unref(m_pipeline);
    m_pipeline = nullptr;
  }
  return err;
}

bool RtpClientSender::IsSenderReady() { return m_pipelineControl.is_live.load(); }

void RtpClientSender::InformEncodingReset() { m_pipelineControl.resetEncoding.store(true); }

bool RtpClientSender::ShouldSenderResetEncoding() {
  bool res = m_pipelineControl.resetEncoding.load();
  m_pipelineControl.resetEncoding.store(false);
  return res;
}

core::Error RtpClientSender::SendData(const uint8_t* data, size_t num_bytes, MediaType media_type) {
  core::Error err = core::Error::SUCCESS;

  if (!m_pipelineControl.is_live.load()) {
    return core::Error::ERR_NETWORK_CONNECTION_INACTIVE;
  }

  if (num_bytes == 0) {
    return core::Error::ERR_DATA_UNAVAILABLE;
  }

  if (!m_pipelineControl.active.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    return core::Error::ERR_NETWORK_CONNECTION_INACTIVE;
  }

  GstBuffer* buffer = gst_buffer_new_and_alloc(num_bytes);
  CHECK_NONNULL(buffer, core::Error::ERR_MEMORY);

  GstMapInfo m;
  CHECK_TRUE(gst_buffer_map(buffer, &m, GST_MAP_WRITE), core::Error::ERR_MEMORY);

  // add payload
  memcpy(m.data, data, num_bytes);
  gst_buffer_unmap(buffer, &m);

  // Send buffer to gstreamer
  {
    GstElement* appsrc = nullptr;
    if (media_type == Video) {
      appsrc = m_videoSrc;
    } else if (media_type == Audio) {
      appsrc = m_audioSrc;
    }
    GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(appsrc), buffer);
    CHECK_TRUE(ret == GST_FLOW_OK, core::Error::ERR_NETWORK_CONNECTION_INACTIVE);
  }

  // report stats
  m_outgoingBytesTotal += num_bytes;
  m_outgoingBytesPeriod += num_bytes;

  if (m_reportStats.load()) {
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - m_startStats);
    if (duration.count() >= m_reportIntervalSeconds) {
      LOG_VERBOSE("%s: Outgoing traffic : %.2f Mbit/s\n", NAME,
                  8.f * (float)m_outgoingBytesPeriod / 1000000.f / (float)m_reportIntervalSeconds);
      m_outgoingBytesPeriod = 0;
      m_startStats = std::chrono::steady_clock::now();
    }
  }

bail:
  return err;
}

core::Error RtpClientSender::EndStream() {
  core::Error err = core::Error::SUCCESS;
  // Signal EOS to the pipeline
  CHECK_TRUE(gst_app_src_end_of_stream(GST_APP_SRC(m_videoSrc)) == GST_FLOW_OK, core::Error::ERR_STREAM_STATE_CHANGE);
  if (m_audioSrc) {
    CHECK_TRUE(gst_app_src_end_of_stream(GST_APP_SRC(m_audioSrc)) == GST_FLOW_OK, core::Error::ERR_STREAM_STATE_CHANGE);
  }
  m_pipelineControl.shouldRun.store(false);

bail:
  return err;
}

void RtpClientSender::Shutdown() {
  GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(m_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "rtp_sender");

  EndStream();
  gst_element_set_state(m_pipeline, GST_STATE_NULL);
  gst_object_unref(m_pipeline);
}

core::Error RtpClientSender::SendAudioData(const uint8_t* data, size_t num_bytes) {
  core::Error err = core::Error::SUCCESS;
  m_gotFirstAudioFrame = true;
  CHECK_NONNULL(m_audioSrc, core::Error::ERR_NOT_SUPPORTED, "Audio stream not enabled");

  if (!m_videoSrc || m_gotFirstVideoFrame) {
    // If we're sending audio+video, wait until we have started sending video before we send any audio.
    err = SendData(data, num_bytes, MediaType::Audio);
  } else {
    LOG_DEBUG("Dropping audio data pushed before first video frame");
  }

bail:
  return err;
}

core::Error RtpClientSender::SendVideoData(const uint8_t* data, size_t num_bytes) {
  core::Error err = core::Error::SUCCESS;
  m_gotFirstVideoFrame = true;
  CHECK_NONNULL(m_videoSrc, core::Error::ERR_NOT_SUPPORTED, "Video stream not enabled");

  if (!m_audioSrc || m_gotFirstAudioFrame) {
    // If we're sending audio+video, wait until we have started sending audio before we send any video.
    err = SendData(data, num_bytes, MediaType::Video);
  } else {
    LOG_DEBUG("Dropping video data pushed before first audio frame");
  }

bail:
  return err;
}

}  // namespace networkmodule
}  // namespace modules
}  // namespace nv3dvc
