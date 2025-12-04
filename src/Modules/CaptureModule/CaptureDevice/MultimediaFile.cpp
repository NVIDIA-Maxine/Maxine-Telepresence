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

#include "MultimediaFile.h"

#include <gst/app/gstappsink.h>
#include <gst/gst.h>

#include <filesystem>
#include <sstream>

#include "Core/Error.h"
#include "Core/Util/Logger.h"
#include "nvCVImage.h"

namespace nv3dvc {
namespace modules {
namespace capturemodule {
namespace capturedevice {

struct MultimediaFileCallbacks {
  static void PadAddedHandler(GstElement* decodebin, GstPad* decoder_pad, MultimediaFile* self) {
    core::Error err = core::Error::SUCCESS;
    // Check media type
    GstCaps* caps = gst_pad_query_caps(decoder_pad, NULL);
    GstStructure* str = gst_caps_get_structure(caps, 0);
    const char* structure_name = gst_structure_get_name(str);
    LOG_DEBUG("Decoder pad-added with caps name %s", structure_name);
    GError* gerror = nullptr;

    GstElement* dest_bin = nullptr;
    if (!self->m_hasAudio && g_str_has_prefix(structure_name, "audio/")) {
      // Create a new bin to connect first audio stream from the decoder to an appsink.
      std::ostringstream cmd;
      cmd << " queue"
             " ! audioconvert"
             " ! audioresample"
             " ! appsink name=audiosink"
             "           caps=audio/x-raw,format=F32LE,layout=interleaved,rate=48000,channels=2";
      LOG_DEBUG("Creating GStreamer bin: %s", cmd.str().c_str());
      dest_bin = gst_parse_bin_from_description(cmd.str().c_str(), /* ghost_unlinked_pads */ true, &gerror);
      CHECK_NONNULL(dest_bin, core::Error::ERR_AUDIO_STREAM_ELEMENT_CREATION);
      CHECK_TRUE(0 != gst_bin_add(GST_BIN(self->m_pipeline), dest_bin), core::ERR_GENERAL);
      self->m_audioSink = gst_bin_get_by_name(GST_BIN(dest_bin), "audiosink");
      self->m_hasAudio = true;
      self->m_fileContainsAudio = true;
    } else if (!self->m_hasVideo && g_str_has_prefix(structure_name, "video/")) {
      // Create a new bin to connect first video stream from the decoder to an appsink.
      std::ostringstream cmd;
      cmd << " queue"
             " ! videoconvert";
      if (self->m_cameraDescriptor.flip_horizontal) {
        cmd << " ! videoflip video-direction=horiz";
      }
      cmd << " ! appsink name=videosink"
             "           caps=video/x-raw,format=RGBA";
      LOG_DEBUG("Creating GStreamer bin: %s", cmd.str().c_str());
      dest_bin = gst_parse_bin_from_description(cmd.str().c_str(), /* ghost_unlinked_pads */ true, &gerror);
      CHECK_NONNULL(dest_bin, core::Error::ERR_VIDEO_STREAM_ELEMENT_CREATION);
      CHECK_TRUE(0 != gst_bin_add(GST_BIN(self->m_pipeline), dest_bin), core::ERR_GENERAL);
      self->m_videoSink = gst_bin_get_by_name(GST_BIN(dest_bin), "videosink");
      self->m_hasVideo = true;
    }
    gst_caps_unref(caps);

    if (dest_bin != nullptr) {
      GstPad* dest_pad = gst_element_get_static_pad(dest_bin, "sink");
      if (!GST_PAD_IS_LINKED(dest_pad)) {
        GstPadLinkReturn ret = gst_pad_link(decoder_pad, dest_pad);
        CHECK_TRUE(ret == GST_PAD_LINK_OK, core::Error::ERR_AUDIO_STREAM_ELEMENT_LINK);
        LOG_DEBUG("Linking decoder pad to '%s'", gst_element_get_name(dest_bin));
      } else {
        LOG_WARNING("Sink pad in '%s' is already linked", gst_element_get_name(dest_bin));
      }
      g_object_unref(dest_pad);
      gst_element_sync_state_with_parent(dest_bin);
    }
  bail:
    if (gerror) {
      LOG_WARNING("Error while creating bin: %s", gerror->message);
      g_error_free(gerror);
    }
    return;
  }

  static void NoMorePadsHandler(GstElement* decodebin, MultimediaFile* self) {
    core::Error err = core::Error::SUCCESS;
    LOG_DEBUG("Decoder no-more-pads");
    GError* gerror = nullptr;

    if (!self->m_hasAudio) LOG_WARNING("Multimedia file does not contain audio");

    if (!self->m_hasAudio && self->m_addSilentAudioIfMissing) {
      LOG_INFO("Adding a silent audio track");
      // If the file doesn't contain audio, generate a silent audio stream.
      const std::string cmd =
          " audiotestsrc wave=silence freq=200"
          " ! appsink name=audiosink"
          "           caps=audio/x-raw,format=F32LE,layout=interleaved,rate=48000,channels=2";
      LOG_DEBUG("Creating GStreamer bin: %s", cmd.c_str());
      GstElement* dest_bin = gst_parse_bin_from_description(cmd.c_str(), /* ghost_unlinked_pads */ true, &gerror);
      CHECK_NONNULL(dest_bin, core::Error::ERR_AUDIO_STREAM_ELEMENT_CREATION);
      CHECK_TRUE(0 != gst_bin_add(GST_BIN(self->m_pipeline), dest_bin), core::ERR_GENERAL);
      self->m_audioSink = gst_bin_get_by_name(GST_BIN(dest_bin), "audiosink");
      self->m_hasAudio = true;
      gst_element_sync_state_with_parent(dest_bin);
    }

    // Save out .dot file if env var `GST_DEBUG_DUMP_DOT_DIR` is set to a basepath.
    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(self->m_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "multimedia_file");

  bail:
    if (gerror) {
      LOG_WARNING("Error while creating bin: %s", gerror->message);
      g_error_free(gerror);
    }
    self->m_waitForDecoderPads = false;
  }
};

MultimediaFile::MultimediaFile(bool add_silent_audio_if_missing)
    : m_cameraDescriptor{}, m_addSilentAudioIfMissing(add_silent_audio_if_missing) {}

MultimediaFile::~MultimediaFile() {
  m_isInitialized = false;
  MultimediaFile::StopCapture();
}

core::Error MultimediaFile::Initialize(const CameraDescriptor& camera_descriptor, const uint32_t width,
                                       const uint32_t height) {
  core::Error err = core::Error::SUCCESS;

  m_cameraDescriptor = camera_descriptor;

  // Variables must be declared here due to goto statements.
  std::ostringstream cmd;
  GError* gerror = nullptr;
  GstStateChangeReturn state_change;
  GstBus* bus = nullptr;

  // Build command to open file and connect it to decodebin.
  cmd << " filesrc location=" << std::filesystem::path{m_cameraDescriptor.camera_device_path}  //
      << " ! decodebin name=decoder";
  LOG_DEBUG("Launching GStreamer pipeline: %s", cmd.str().c_str());
  m_pipeline = gst_parse_launch(cmd.str().c_str(), &gerror);
  CHECK_NULL(gerror, core::Error::ERR_VIDEO_CONFIGURATION, "Error constructing playback pipeline: %s", gerror->message);
  CHECK_NONNULL(m_pipeline, core::Error::ERR_VIDEO_CONFIGURATION, "Failed to create pipeline");

  m_decoder = gst_bin_get_by_name(GST_BIN(m_pipeline), "decoder");
  CHECK_NONNULL(m_decoder, core::Error::ERR_VIDEO_STREAM_ELEMENT_CREATION, "Failed to get media pipeline elements");

  m_waitForDecoderPads = true;
  m_fileContainsAudio = false;
  m_hasAudio = false;
  m_hasVideo = false;
  g_signal_connect(m_decoder, "pad-added", G_CALLBACK(MultimediaFileCallbacks::PadAddedHandler), this);
  g_signal_connect(m_decoder, "no-more-pads", G_CALLBACK(MultimediaFileCallbacks::NoMorePadsHandler), this);

  // Start pipeline
  state_change = gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
  CHECK_FALSE(state_change == GST_STATE_CHANGE_FAILURE, core::Error::ERR_STREAM_STATE_CHANGE);

  // Wait to get video dimensions.
  bus = gst_pipeline_get_bus(GST_PIPELINE(m_pipeline));
  // Wait until there are no more streams left, and we have the caps for all the streams.
  while (m_waitForDecoderPads || (m_hasAudio && m_audioSampleRate == 0) || (m_hasVideo && m_videoWidth == 0)) {
    // Get any state-changed messages.
    GstMessage* msg = gst_bus_timed_pop_filtered(bus, 10 * GST_SECOND, GST_MESSAGE_STATE_CHANGED);
    CHECK_NONNULL(msg, core::Error::ERR_INITIALIZATION, "Timed out waiting to open file source");
    // Listen for state changes on the audio and video sinks.
    // When the sinks go into state PAUSED, they will have complete caps that we can query.
    if (GST_MESSAGE_SRC(msg) == GST_OBJECT(m_audioSink)) {
      GstPad* audiopad = gst_element_get_static_pad(m_audioSink, "sink");
      CHECK_NONNULL(audiopad, core::Error::ERR_VIDEO_STREAM_ELEMENT_CREATION, "Could not get audiosink pad 'sink'");
      GstCaps* audiocaps = gst_pad_get_current_caps(audiopad);
      if (audiocaps) {
        LOG_DEBUG("Audio caps:  %s", gst_caps_to_string(audiocaps));
        GstStructure* structure = gst_caps_get_structure(audiocaps, 0);
        const bool got_rate = gst_structure_get_int(structure, "rate", &m_audioSampleRate);
        const bool got_channels = gst_structure_get_int(structure, "channels", &m_audioNumChannels);
        if (!got_rate || !got_channels) {
          LOG_WARNING("Unable to get audio format, disabling audio");
          m_hasAudio = false;
        }
        gst_caps_unref(audiocaps);
      }
      gst_object_unref(audiopad);
    } else if (GST_MESSAGE_SRC(msg) == GST_OBJECT(m_videoSink)) {
      GstPad* videopad = gst_element_get_static_pad(m_videoSink, "sink");
      CHECK_NONNULL(videopad, core::Error::ERR_VIDEO_STREAM_ELEMENT_CREATION, "Could not get videosink pad 'sink'");
      GstCaps* videocaps = gst_pad_get_current_caps(videopad);
      if (videocaps) {
        LOG_DEBUG("Video caps:  %s", gst_caps_to_string(videocaps));
        GstStructure* structure = gst_caps_get_structure(videocaps, 0);
        const bool got_width = gst_structure_get_int(structure, "width", &m_videoWidth);
        const bool got_height = gst_structure_get_int(structure, "height", &m_videoHeight);
        if (!got_width || !got_height) {
          LOG_WARNING("Unable to get video format, disabling video");
          m_hasVideo = false;
        }
        int fps_numerator = 0, fps_denominator = 1;
        const bool got_fps = gst_structure_get_fraction(structure, "framerate", &fps_numerator, &fps_denominator);
        if (got_fps) {
          m_videoFrameRate = static_cast<float>(fps_numerator) / fps_denominator;
        } else {
          LOG_WARNING("Unable to get video frame-rate");
        }
        gst_caps_unref(videocaps);
      }
      gst_object_unref(videopad);
    }
  }

  CHECK_TRUE(m_videoSink || m_audioSink, core::Error::ERR_VIDEO_CONFIGURATION);

  if (m_videoSink) {
    // Minimize internal queue inside the video app sink, to improve A/V sync.
    gst_app_sink_set_max_buffers(GST_APP_SINK(m_videoSink), 2);
    gst_app_sink_set_drop(GST_APP_SINK(m_videoSink), true);
  }

  LOG_INFO("Opened video %s of resolution %u x %u and framerate %f", m_cameraDescriptor.camera_device_path.c_str(),
           m_videoWidth, m_videoHeight, m_videoFrameRate);

  NvCVImage_Alloc(&m_latestFrameRgbaPinned, m_videoWidth, m_videoHeight, NVCV_RGBA, NVCV_U8, NVCV_CHUNKY,
                  NVCV_CPU_PINNED, 1);

  m_isInitialized = true;

bail:
  if (gerror) {
    g_error_free(gerror);
  }
  if (err != core::Error::SUCCESS && m_pipeline) {
    gst_object_unref(m_pipeline);
    m_pipeline = nullptr;
  }
  return err;
}

bool MultimediaFile::FileContainsAudio() const { return m_fileContainsAudio; }

CaptureApi MultimediaFile::GetCaptureApi() const { return CaptureApi::MULTIMEDIA_FILE; }

core::Error MultimediaFile::GetFrame(NvCVImage* frame, cudaStream_t stream) {
  core::Error err = core::Error::SUCCESS;

  CHECK_TRUE(m_isInitialized, core::Error::ERR_INITIALIZATION);

  const size_t dst_bytes = m_latestFrameRgbaPinned.height * m_latestFrameRgbaPinned.pitch;
  size_t pulled_bytes = 0;
  err = PullData(m_videoSink, m_latestFrameRgbaPinned.pixels, dst_bytes, &pulled_bytes);
  if (err == core::Error::SUCCESS) {
    NvCVImage_InitView(frame, &m_latestFrameRgbaPinned, 0, 0, m_latestFrameRgbaPinned.width,
                       m_latestFrameRgbaPinned.height);
  }
bail:
  return err;
}

core::Error MultimediaFile::GetFramerate(float* framerate) const {
  if (!framerate) return core::Error::ERR_NULL_POINTER;
  if (!m_isInitialized) return core::Error::ERR_INITIALIZATION;
  *framerate = m_videoFrameRate;
  return core::Error::SUCCESS;
}

core::Error MultimediaFile::GetResolution(int* width, int* height) const {
  if (!width || !height) return core::Error::ERR_NULL_POINTER;
  if (!m_isInitialized) return core::Error::ERR_INITIALIZATION;
  *width = m_videoWidth;
  *height = m_videoHeight;
  return core::Error::SUCCESS;
}

core::Error MultimediaFile::StopCapture() {
  core::Error err = core::Error::SUCCESS;

  if (!m_isInitialized) return core::Error::ERR_INITIALIZATION;

  if (m_pipeline) {
    GstStateChangeReturn state_change = gst_element_set_state(m_pipeline, GST_STATE_NULL);
    CHECK_FALSE(state_change == GST_STATE_CHANGE_FAILURE, core::Error::ERR_STREAM_STATE_CHANGE);
    gst_object_unref(m_pipeline);
    m_pipeline = nullptr;
  }

bail:
  return err;
}

bool MultimediaFile::HasAudio() const { return m_hasAudio; }

core::Error MultimediaFile::GetAudioFormat(int* sample_rate, int* num_channels) const {
  if (!sample_rate || !num_channels) return core::Error::ERR_NULL_POINTER;
  if (!m_isInitialized) return core::Error::ERR_INITIALIZATION;
  *sample_rate = m_audioSampleRate;
  *num_channels = m_audioNumChannels;
  return core::Error::SUCCESS;
}

core::Error MultimediaFile::PullAudioData(float* dst_data, int dst_samples, int* pulled_samples) {
  core::Error err = core::Error::SUCCESS;

  CHECK_TRUE(HasAudio(), core::Error::ERR_DATA_UNAVAILABLE);

  const size_t dst_bytes = dst_samples * m_audioNumChannels * sizeof(float);
  size_t pulled_bytes = 0;
  err = PullData(m_audioSink, dst_data, dst_bytes, &pulled_bytes);
  if (pulled_samples) *pulled_samples = pulled_bytes / (sizeof(float) * m_audioNumChannels);
bail:
  return err;
}

core::Error MultimediaFile::PullData(GstElement* appsink, void* dst_data, size_t dst_bytes, size_t* pulled_bytes) {
  if (!appsink) {
    LOG_DEBUG("MultimediaFile is not initialized");
    return core::ERR_INITIALIZATION;
  }

  // Try to pull a sample. Don't wait, since all cameras are running on the same thread.
  GstSample* sample = gst_app_sink_try_pull_sample(GST_APP_SINK(appsink), 0);
  if (sample == nullptr) {
    if (pulled_bytes) *pulled_bytes = 0;
    return gst_app_sink_is_eos(GST_APP_SINK(appsink)) ? core::Error::ERR_EOF : core::Error::ERR_DATA_UNAVAILABLE;
  }

  GstCaps* caps = gst_sample_get_caps(sample);

  GstBuffer* buffer = gst_sample_get_buffer(sample);

  if (buffer->pts == GST_CLOCK_TIME_NONE) buffer->pts = 0;
  if (buffer->duration == GST_CLOCK_TIME_NONE) buffer->duration = 0;
  if (buffer->offset == GST_CLOCK_TIME_NONE) buffer->offset = 0;

  // map buffer to access memory
  GstMapInfo m;
  gst_buffer_map(buffer, &m, GST_MAP_READ);
  if (dst_bytes < m.size) {
    LOG_ERROR("Destination buffer too small to pull sample");
    gst_buffer_unmap(buffer, &m);
    gst_sample_unref(sample);
    return core::Error::ERR_READ;
  }
  memcpy(dst_data, m.data, m.size);
  if (pulled_bytes) *pulled_bytes = m.size;

  // Don't forget to unmap the buffer and unref the sample
  gst_buffer_unmap(buffer, &m);
  gst_sample_unref(sample);

  return core::Error::SUCCESS;
}

}  // namespace capturedevice
}  // namespace capturemodule
}  // namespace modules
}  // namespace nv3dvc
