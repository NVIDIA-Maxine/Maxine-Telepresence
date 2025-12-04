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

#include "RtspServer.h"

#include <glib.h>
#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
#include <gst/gstbin.h>
#include <gst/rtsp-server/rtsp-server.h>

#include <sstream>

#include "Core/Util/Logger.h"

namespace nv3dvc {
namespace modules {
namespace networkmodule {

struct RtspServerCallbacks {
  static void MediaConfigure(GstRTSPMediaFactory* /* factory */, GstRTSPMedia* media, RtspServer* self) {
    LOG_DEBUG("RtspServer: Configuring media");

    GstElement* pipeline = gst_rtsp_media_get_element(media);
    self->m_videoSrc = gst_bin_get_by_name(GST_BIN(pipeline), "videosrc");
    if (self->m_sendAudio) {
      self->m_audioSrc = gst_bin_get_by_name(GST_BIN(pipeline), "audiosrc");
    }
    gst_object_unref(pipeline);
    // Allow SetData to push data to the `appsrc`s.
    self->m_connected = true;
    // Listen to state changes so we know when client disconnects.
    g_signal_connect(media, "new-state", G_CALLBACK(StateChangedCallback), self);
  }

  static void StateChangedCallback(GstRTSPMedia* media, GstState state, RtspServer* self) {
    LOG_DEBUG("RtspServer: Got state \"%s\"", gst_element_state_get_name(state));
    if (state == GST_STATE_PLAYING) {
    } else if (state == GST_STATE_NULL) {
      // When client disconnects, media state is set to NULL.
      self->m_connected = false;
    } else {
      LOG_WARNING("RtspServer: Unexpected state");
    }
  }
};

core::Error RtspServer::Initialize(int out_port, const std::string& rtsp_path, const bool send_audio) {
  core::Error err = core::Error::SUCCESS;

  // Each element with pay%d names will be a stream
  // Build complete command.
  std::ostringstream cmd;
  // Video appsrc sends video packets to the RTSP server.
  cmd << " appsrc name=videosrc"
         "        format=time"
         "        is-live=true"
         "        stream-type=stream"
         "        do-timestamp=true"
         "        emit-signals=true"
         "        caps=video/x-h264,stream-format=byte-stream,alignment=au,colorimetry=bt709"
         " ! h264parse"
         " ! video/x-h264"
         " ! queue"
         " ! rtph264pay mtu=65507"
         "              config-interval=1"
         "              aggregate-mode=1"
         " ! application/x-rtp,media=video,clock-rate=90000,encoding-name=H264,payload=96"
         " ! identity name=pay0";
  if (send_audio) {
    // Audio appsrc sends audio packets to the RTSP server.
    cmd << " appsrc name=audiosrc"
           "        format=time"
           "        is-live=true"
           "        stream-type=stream"
           "        do-timestamp=true"
           "        caps=audio/x-opus"
           " ! opusparse"
           " ! audio/x-opus"
           " ! queue"
           " ! rtpopuspay mtu=65507"
           " ! application/x-rtp,media=audio,clock-rate=48000,encoding-name=OPUS,payload=96"
           " ! identity name=pay1";
  }

  CHECK_TRUE(rtsp_path.size() >= 1 && rtsp_path[0] == '/', core::Error::ERR_GENERAL, "RTSP path must start with /");

  m_sendAudio = send_audio;

  m_connected = false;

  m_server = gst_rtsp_server_new();
  CHECK_NONNULL(m_server, core::Error::ERR_INITIALIZATION);
  g_object_set(m_server, "service", std::to_string(out_port).c_str(), NULL);

  {
    GstRTSPMediaFactory* factory = gst_rtsp_media_factory_new();
    CHECK_NONNULL(factory, core::Error::ERR_INITIALIZATION);
    gst_rtsp_media_factory_set_launch(factory, cmd.str().c_str());
    // This is called whenever someone asks for the media and a new pipeline with our appsrc is created.
    g_signal_connect(factory, "media-configure", G_CALLBACK(RtspServerCallbacks::MediaConfigure), this);
    gst_rtsp_media_factory_set_shared(factory, true);
    gst_rtsp_media_factory_set_enable_rtcp(factory, true);

    GstRTSPMountPoints* mounts = gst_rtsp_server_get_mount_points(m_server);
    CHECK_NONNULL(mounts, core::Error::ERR_INITIALIZATION);
    // Docs for `gst_rtsp_mount_points_add_factory` say: "Ownership is taken of the reference on `factory` so that
    // `factory` should not be used after calling this function."
    gst_rtsp_mount_points_add_factory(mounts, rtsp_path.c_str(), factory);
    gst_object_unref(mounts);
  }

  // Start the server by attaching it to our GMainContext (event loop).
  m_context = g_main_context_new();
  CHECK_NONNULL(m_context, core::Error::ERR_INITIALIZATION);
  gst_rtsp_server_attach(m_server, m_context);

  LOG_DEBUG("RTSP stream ready at rtsp://127.0.0.1:%d%s", out_port, rtsp_path.c_str());

  // Run the RTSP server event loop on a background thread.
  m_thread = std::thread([=]() {
    g_main_context_push_thread_default(m_context);
    // Create loop that will poll the GMainContext (event loop);
    m_loop = g_main_loop_new(m_context, false);
    // Start the loop, which will run until g_main_loop_quit is called.
    g_main_loop_run(m_loop);
    // After the loop finishes, unreference it.
    g_main_loop_unref(m_loop);
    m_loop = nullptr;
    g_main_context_pop_thread_default(m_context);
  });

bail:
  return err;
}

RtspServer::~RtspServer() {
  if (m_audioSrc) {
    gst_object_unref(m_audioSrc);
    m_audioSrc = nullptr;
  }
  if (m_videoSrc) {
    gst_object_unref(m_videoSrc);
    m_videoSrc = nullptr;
  }
  if (m_loop) {
    // Queue g_main_loop_quit on the GMainContext (event loop).
    g_main_context_invoke(m_context, reinterpret_cast<GSourceFunc>(g_main_loop_quit), m_loop);
  }
  if (m_thread.joinable()) {
    // Wait for thread to finish.
    m_thread.join();
  }
  if (m_context) {
    g_main_context_unref(m_context);
    m_context = nullptr;
  }
  if (m_server) {
    gst_object_unref(m_server);
    m_server = nullptr;
  }
}

core::Error RtspServer::SendAudioData(const uint8_t* data, size_t num_bytes) {
  if (!m_connected) return core::Error::SUCCESS;

  core::Error err = core::Error::SUCCESS;
  m_gotFirstAudioFrame = true;
  CHECK_NONNULL(m_audioSrc, core::Error::ERR_NOT_SUPPORTED, "Audio stream not enabled");

  if (!m_videoSrc || m_gotFirstVideoFrame) {
    // If we're sending audio+video, wait until we have started sending video before we send any audio.
    err = SendData(data, num_bytes, m_audioSrc);
  } else {
    LOG_DEBUG("Dropping audio data pushed before first video frame");
  }

bail:
  return err;
}

core::Error RtspServer::SendVideoData(const uint8_t* data, size_t num_bytes) {
  if (!m_connected) return core::Error::SUCCESS;

  core::Error err = core::Error::SUCCESS;
  m_gotFirstVideoFrame = true;
  CHECK_NONNULL(m_videoSrc, core::Error::ERR_NOT_SUPPORTED, "Video stream not enabled");

  if (!m_audioSrc || m_gotFirstAudioFrame) {
    // If we're sending audio+video, wait until we have started sending audio before we send any video.
    err = SendData(data, num_bytes, m_videoSrc);
  } else {
    LOG_DEBUG("Dropping video data pushed before first audio frame");
  }

bail:
  return err;
}

core::Error RtspServer::SendData(const uint8_t* data, size_t num_bytes, GstElement* appsrc) {
  if (num_bytes == 0) {
    return core::Error::ERR_DATA_UNAVAILABLE;
  }

  core::Error err = core::Error::SUCCESS;

  GstBuffer* buffer = gst_buffer_new_and_alloc(num_bytes);
  CHECK_NONNULL(buffer, core::Error::ERR_MEMORY);

  GstMapInfo m;
  CHECK_TRUE(gst_buffer_map(buffer, &m, GST_MAP_WRITE), core::Error::ERR_MEMORY);

  // add payload
  memcpy(m.data, data, num_bytes);
  gst_buffer_unmap(buffer, &m);

  // Send buffer to gstreamer
  {
    GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(appsrc), buffer);
    // If the client disconnected but we haven't been notified yet, ret maybe be "flushing".
    CHECK_TRUE(ret == GST_FLOW_OK || ret == GST_FLOW_FLUSHING, core::Error::ERR_NETWORK_CONNECTION_INACTIVE, "Got %s",
               gst_flow_get_name(ret));
  }

bail:
  return err;
}

}  // namespace networkmodule
}  // namespace modules
}  // namespace nv3dvc
