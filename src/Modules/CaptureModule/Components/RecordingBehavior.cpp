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

#include "RecordingBehavior.h"

#include <gst/app/gstappsrc.h>
#include <gst/gst.h>

#include <filesystem>
#include <sstream>
#include <string>

#include "Core/Error.h"
#include "Core/Util/Logger.h"
#include "Modules/CommonModule/Components/CallbackComponent.h"

namespace nv3dvc {
namespace modules {
namespace capturemodule {
namespace components {

core::Error RecordingBehavior::Stop() {
  core::Error err = core::Error::SUCCESS;

  CHECK_TRUE(HasComponent<commonmodule::components::RecordingCallbackComponent>(), core::Error::ERR_SCENE,
             "RecordingBehavior requires a RecordingCallbackComponent to exist on the entity");

  // Disconnect the callback so that the component can receive data.
  GetComponent<commonmodule::components::RecordingCallbackComponent>().SetOnFiredCallback(nullptr);

  if (m_pipeline) {
    GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(m_pipeline));

    // Signal EOS to the pipeline.
    if (gst_app_src_end_of_stream(GST_APP_SRC(m_videosrc)) != GST_FLOW_OK) {
      LOG_ERROR("Failed to send EOS to video appsrc");
    }
    if (gst_app_src_end_of_stream(GST_APP_SRC(m_audiosrc)) != GST_FLOW_OK) {
      LOG_ERROR("Failed to send EOS to video appsrc");
    }

    // Wait here until the pipeline is fully flushed.
    gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_EOS);
    gst_object_unref(bus);

    // GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(m_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-finished");

    gst_element_set_state(m_pipeline, GST_STATE_NULL);
    gst_object_unref(m_pipeline);
  }
  m_audioSampleRate = 0;
  m_audioNumChannels = 0;
  m_videoWidth = 0;
  m_videoHeight = 0;
  m_audiosrc = nullptr;
  m_videosrc = nullptr;
  m_pipeline = nullptr;

bail:
  return err;
}

core::Error RecordingBehavior::OnInitialize() {
  core::Error err = core::Error::SUCCESS;

  CHECK_TRUE(HasComponent<commonmodule::components::RecordingCallbackComponent>(), core::Error::ERR_SCENE,
             "RecordingBehavior requires a RecordingCallbackComponent to exist on the entity");

  if (enable) CHECK_SUCCESS(Start());

  // Value of m_enabled in the OnChangeFunction is AFTER the change.
  enable.SetOnChangeFunction([this]() {
    core::Error err = core::Error::SUCCESS;
    if (enable) {
      CHECK_SUCCESS(Start());
    } else {
      CHECK_SUCCESS(Stop());
    }
  bail:
    return;
  });

bail:
  return err;
}

core::Error RecordingBehavior::OnUnloadScene() { return Stop(); }

core::Error RecordingBehavior::Start() {
  core::Error err = core::Error::SUCCESS;

  // Variables must be declared here due to goto statements.
  std::filesystem::path file_path_s, extension;
  std::ostringstream cmd;
  std::string muxer;
  GError* gerror = nullptr;
  GstStateChangeReturn state_change;

  CHECK_NULL(m_pipeline, core::Error::ERR_GENERAL, "Recording already started");

  file_path_s = {std::string{file_path}};
  extension = file_path_s.extension();
  CHECK_TRUE(extension == ".mkv" || extension == ".mp4", core::Error::ERR_VIDEO_CONFIGURATION,
             "Recording filename must have .mkv or .mp4 extension -- got %s", extension.c_str());

  // Muxer
  muxer = extension == ".mkv" ? "matroskamux" : "mp4mux";
  // Build complete command.
  cmd << " appsrc name=audiosrc"
         "        format=time"
         "        is-live=true"
         "        stream-type=stream"
         //  "        do-timestamp=true"  // seems to create clicks in the audio
         " ! queue"
         " ! audioconvert"
         " ! avenc_aac"
         " ! queue"
         " ! mux."
         " appsrc name=videosrc"
         "        format=time"
         "        is-live=true"
         "        stream-type=stream"
         "        do-timestamp=true"
         " ! queue"
         " ! videoconvert"
         " ! capsfilter caps=video/x-raw,format=NV12"
         " ! x264enc tune=zerolatency"
         "           bitrate=10000"  // kbit/sec
         " ! queue"
         " ! h264parse"
         " ! mux."
      << " " << muxer << " name=mux"  //
      << " ! queue"                   //
      << " ! filesink sync=false location=" << file_path_s;
  LOG_DEBUG("Launching GStreamer pipeline: %s", cmd.str().c_str());
  GstElement* pipeline = gst_parse_launch(cmd.str().c_str(), &gerror);
  CHECK_NULL(gerror, core::Error::ERR_VIDEO_CONFIGURATION, "Error constructing recording pipeline: %s",
             gerror->message);
  CHECK_NONNULL(pipeline, core::Error::ERR_VIDEO_CONFIGURATION, "Failed to create pipeline");

  // GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-parsed");

  m_audiosrc = gst_bin_get_by_name(GST_BIN(pipeline), "audiosrc");
  m_videosrc = gst_bin_get_by_name(GST_BIN(pipeline), "videosrc");
  CHECK_NONNULL(m_audiosrc, core::Error::ERR_AUDIO_STREAM_ELEMENT_CREATION, "Failed to get audio input element");
  CHECK_NONNULL(m_videosrc, core::Error::ERR_VIDEO_STREAM_ELEMENT_CREATION, "Failed to get video input element");

  // Start pipeline
  state_change = gst_element_set_state(pipeline, GST_STATE_PLAYING);
  CHECK_FALSE(state_change == GST_STATE_CHANGE_FAILURE, core::Error::ERR_STREAM_STATE_CHANGE,
              "%s: Failed to start recording pipeline", NAME);

  m_audioSampleRate = 0;
  m_audioNumChannels = 0;
  m_videoWidth = 0;
  m_videoHeight = 0;
  m_pipeline = pipeline;

  // Connect the callback so that the component can receive data.
  GetComponent<commonmodule::components::RecordingCallbackComponent>().SetOnFiredCallback(
      [&](void* data_ptr, size_t data_size, size_t* pushed_size, int data_type) -> core::Error {
        core::Error err = core::Error::SUCCESS;
        if (pushed_size) *pushed_size = 0;
        CHECK_NONNULL(data_ptr, core::Error::ERR_NULL_POINTER);
        using commonmodule::components::RecordingCallbackComponent;
        switch (data_type) {
          case RecordingCallbackComponent::VIDEO_FORMAT: {
            auto* const format = static_cast<const RecordingCallbackComponent::VideoFormat*>(data_ptr);
            CHECK_SUCCESS(SetVideoSize(format->width, format->height));
            if (pushed_size) *pushed_size = data_size;
            break;
          }
          case RecordingCallbackComponent::AUDIO_FORMAT: {
            auto* const format = static_cast<const RecordingCallbackComponent::AudioFormat*>(data_ptr);
            CHECK_SUCCESS(SetAudioFormat(format->sample_rate, format->num_channels));
            if (pushed_size) *pushed_size = data_size;
            break;
          }
          case RecordingCallbackComponent::VIDEO_DATA: {
            const uint8_t* const src_rgba_pixels = static_cast<const uint8_t*>(data_ptr);
            CHECK_SUCCESS(PushVideoData(src_rgba_pixels));
            if (pushed_size) *pushed_size = data_size;
            break;
          }
          case RecordingCallbackComponent::AUDIO_DATA: {
            const float* const src_data = static_cast<const float*>(data_ptr);
            const int src_samples = static_cast<int>(data_size);
            CHECK_SUCCESS(PushAudioData(src_data, src_samples));
            if (pushed_size) *pushed_size = data_size;
            break;
          }
        }
      bail:
        return err;
      });

bail:
  if (gerror) {
    g_error_free(gerror);
  }
  if (err != core::Error::SUCCESS) {
    if (pipeline) gst_object_unref(pipeline);
    assert(m_pipeline == nullptr);  // m_pipeline should not have been modified
  }
  return err;
}

core::Error RecordingBehavior::SetVideoSize(int width, int height) {
  core::Error err = core::Error::SUCCESS;

  if (width == m_videoWidth && height == m_videoHeight) return core::Error::SUCCESS;

  LOG_DEBUG("Setting RecordingBehavior video size to %d x %d", width, height);

  // Make caps.
  std::stringstream caps_ss;
  caps_ss << "video/x-raw,"
          << "format=RGBA,framerate=0/1,width=" << width << ",height=" << height;
  GstCaps* caps = gst_caps_from_string(caps_ss.str().c_str());
  CHECK_NONNULL(caps, core::ERR_GENERAL);

  // Set appsrc caps.
  gst_app_src_set_caps(GST_APP_SRC(m_videosrc), caps);
  gst_caps_unref(caps);

  m_videoWidth = width;
  m_videoHeight = height;

bail:
  return err;
}

core::Error RecordingBehavior::SetAudioFormat(int sample_rate, int num_channels) {
  core::Error err = core::Error::SUCCESS;

  if (sample_rate == m_audioSampleRate && num_channels == m_audioNumChannels) return core::Error::SUCCESS;

  LOG_DEBUG("Setting RecordingBehavior audio format to %d x %d", sample_rate, num_channels);

  // Make caps.
  std::stringstream caps_ss;
  caps_ss << "audio/x-raw,"
          << "format=F32LE,layout=interleaved,rate=" << sample_rate << ",channels=" << num_channels;
  GstCaps* caps = gst_caps_from_string(caps_ss.str().c_str());
  CHECK_NONNULL(caps, core::ERR_GENERAL);

  // Set appsrc caps.
  gst_app_src_set_caps(GST_APP_SRC(m_audiosrc), caps);
  gst_caps_unref(caps);

  m_audioSampleRate = sample_rate;
  m_audioNumChannels = num_channels;

bail:
  return err;
}

core::Error RecordingBehavior::PushVideoData(const uint8_t* src_rgba_pixels) {
  core::Error err = core::Error::SUCCESS;

  if (m_videoWidth == 0 || m_videoHeight == 0) {
    LOG_DEBUG("Video format not yet set, skipping frame");
    return core::Error::SUCCESS;
  }
  const size_t src_bytes = m_videoWidth * m_videoHeight * 4 * sizeof(uint8_t);
  CHECK_SUCCESS(PushData(m_videosrc, src_rgba_pixels, src_bytes));
bail:
  return err;
}

core::Error RecordingBehavior::PushAudioData(const float* src_data, int src_samples) {
  core::Error err = core::Error::SUCCESS;

  if (m_audioSampleRate == 0 || m_audioNumChannels == 0) {
    LOG_DEBUG("Audio format not yet set, skipping frame");
    return core::Error::SUCCESS;
  }
  const size_t src_bytes = src_samples * m_audioNumChannels * sizeof(float);
  CHECK_SUCCESS(PushData(m_audiosrc, src_data, src_bytes));
bail:
  return err;
}

core::Error RecordingBehavior::PushData(GstElement* appsrc, const void* src_data, size_t src_bytes) {
  if (!appsrc) {
    LOG_DEBUG("RecordingBehavior is not initialized");
    return core::ERR_INITIALIZATION;
  }

  GstBuffer* buffer = gst_buffer_new_and_alloc(src_bytes);
  if (!buffer) return core::ERR_WRITE;

  // Manual timestamping should not be necessary because "do-timestamp" is set on the appsrc.
  // This should tell GStreamer to assign the timestamp inside gst_app_src_push_buffer.
  // https://gitlab.freedesktop.org/gstreamer/gstreamer/-/blob/main/subprojects/gst-plugins-base/gst-libs/gst/app/gstappsrc.c#L2353

  GST_BUFFER_PTS(buffer) = GST_CLOCK_TIME_NONE;
  GST_BUFFER_DTS(buffer) = GST_CLOCK_TIME_NONE;
  // GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, 2);

  GstMapInfo m;
  if (!gst_buffer_map(buffer, &m, GST_MAP_WRITE)) {
    return core::ERR_WRITE;
  }

  // add payload
  memcpy(m.data, src_data, src_bytes);
  gst_buffer_unmap(buffer, &m);

  // Send buffer to gstreamer
  GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(appsrc), buffer);
  if (ret != GST_FLOW_OK) return core::ERR_WRITE;

  return core::SUCCESS;
}

}  // namespace components
}  // namespace capturemodule
}  // namespace modules
}  // namespace nv3dvc
