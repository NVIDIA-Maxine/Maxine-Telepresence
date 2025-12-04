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

#include "RtpClientReceiver.h"

#include <chrono>  // NOLINT(build/c++11) (cpplint outside Chromium: https://github.com/google/styleguide/issues/483)
#include <functional>
#include <iostream>
#include <sstream>
#include <vector>

#include "Core/Error.h"
#include "Core/Util/Logger.h"
#include "gst/app/gstappsink.h"
#include "gst/gstbus.h"
#include "gst/gstpipeline.h"
#include "gst/gstutils.h"
#include "nvCVImage.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Macros                                                                                                           ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define UUID_SIZE 16
#define USER_DATA_UNREGISTERED_TYPE 5

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Static function declarations                                                                                     ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static bool CheckUuid(uint8_t* stream, const char* sei_uuid_string);
static uint8_t* ParseSeiUnit(uint8_t* bs_ptr, guint* size, const char* sei_uuid_string);
static uint8_t* ParseSeiData(uint8_t* bs, uint32_t size, uint32_t* payload_size, const char* sei_uuid_string);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Static function definitions                                                                                      ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool CheckUuid(uint8_t* stream, const char* sei_uuid_string) {
  char uuid_string[UUID_SIZE] = {0};
  uint32_t size = snprintf(uuid_string, UUID_SIZE, "%s", stream);
  if (size == (UUID_SIZE - 1)) {
    return !strncmp(uuid_string, sei_uuid_string, (UUID_SIZE - 1));
  } else {
    return false;
  }
}

uint8_t* ParseSeiUnit(uint8_t* bs_ptr, guint* size, const char* sei_uuid_string) {
  int payload_type = 0;
  int payload_size = 0;
  uint8_t* payload = NULL;
  int i;

  payload_type = *bs_ptr++;

  while (payload_size % 0xFF == 0) {
    payload_size += *bs_ptr++;
  }
  if (!CheckUuid(bs_ptr, sei_uuid_string)) {
    bs_ptr += (payload_size - UUID_SIZE);
    return NULL;
  } else {
    bs_ptr += UUID_SIZE;
  }

  *size = payload_size;

  if (payload_type == USER_DATA_UNREGISTERED_TYPE) {
    payload = static_cast<uint8_t*>(malloc((payload_size - UUID_SIZE) * sizeof(uint8_t)));

    for (i = 0; i < (payload_size - UUID_SIZE); i++) {
      payload[i] = *bs_ptr;

      if (strncmp(sei_uuid_string, "VST_CUSTOM_META", (UUID_SIZE - 1)) != 0) {
        // drop emulation prevention bytes
        if ((*(bs_ptr) == 0x03) && (*(bs_ptr - 1) == 0x00) && (*(bs_ptr - 2) == 0x00)) {
          i--;
        }
      }
      bs_ptr++;
    }
    return payload;
  } else {
    return NULL;
  }
}

uint8_t* ParseSeiData(uint8_t* bs, uint32_t size, uint32_t* payload_size, const char* sei_uuid_string) {
  if (sei_uuid_string == NULL) return NULL;
  int checklen = 0;
  unsigned int sei_payload_size = 0;
  uint8_t* bs_ptr = bs;
  uint8_t* bs_ptr_end = bs + size;
  uint8_t* payload = NULL;
  while (bs_ptr_end > bs_ptr) {
    if (checklen < 2 && *bs_ptr++ == 0x00) {
      checklen++;
    } else if (checklen == 2 && *bs_ptr++ == 0x00) {
      checklen++;
    } else if (checklen == 3 && *bs_ptr++ == 0x01) {
      checklen++;
    } else if (checklen == 4 && *bs_ptr++ == 0x06) {
      payload = ParseSeiUnit(bs_ptr, &sei_payload_size, sei_uuid_string);
      checklen = 0;
      if (payload != NULL) {
        *payload_size = (sei_payload_size - 16);
        return payload;
      } else {
        continue;
      }
    } else {
      checklen = 0;
    }
  }
  return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Member function definitions                                                                                      ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace nv3dvc {
namespace modules {
namespace networkmodule {

struct RtpClientReceiverCallbacks {
  static void PadAddedHandler(GstElement* rtspsrc, GstPad* src_pad, RtpClientReceiver* self) {
    // Check media type
    GstCaps* caps = gst_pad_query_caps(src_pad, NULL);
    GstStructure* str = gst_caps_get_structure(caps, 0);
    const char* structure_name = gst_structure_get_name(str);
    const char* media = gst_structure_get_string(str, "media");
    LOG_DEBUG("Decoder pad added with caps name %s, media %s", structure_name, media);

    GstElement* dest_queue = nullptr;
    if (g_str_equal(media, "audio")) {
      dest_queue = self->m_aqueue;
    } else if (self->m_shouldReceiveAudio && g_str_equal(media, "video")) {
      dest_queue = self->m_vqueue;
    }
    gst_caps_unref(caps);

    if (dest_queue != nullptr) {
      GstPad* dest_pad = gst_element_get_static_pad(dest_queue, "sink");
      if (!GST_PAD_IS_LINKED(dest_pad)) {
        LOG_DEBUG("Linking rtspsrc pad to '%s'", gst_element_get_name(dest_queue));
        GstPadLinkReturn ret = gst_pad_link(src_pad, dest_pad);
        if (GST_PAD_LINK_FAILED(ret)) {
          LOG_ERROR("Failed to link pad");
        } else {
          LOG_DEBUG("Pad linked successfully");
        }
      } else {
        LOG_WARNING("Sink pad in '%s' is already linked", gst_element_get_name(dest_queue));
      }
      gst_object_unref(dest_pad);
    }
  }
};

core::Error RtpClientReceiver::PollVideoAppSink() { return PollAppSink(m_pipelineControl, MediaType::Video); }

core::Error RtpClientReceiver::PollAudioAppSink() { return PollAppSink(m_pipelineControl, MediaType::Audio); }

core::Error RtpClientReceiver::PollAppSink(const PipelineControl& control, const MediaType media_type) {
  core::Error err = core::Error::SUCCESS;

  if (!ShouldRun()) {
    LOG_DEBUG("RtpClientReceiver not started");
    return core::Error::ERR_NETWORK_CONNECTION_INACTIVE;
  }

  GstSample* sample = nullptr;

  if (!control.active.load()) {
    LOG_VERBOSE("RtpClientReceiver connection is inactive");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    return core::Error::ERR_NETWORK_CONNECTION_INACTIVE;
  }

  // Pull the sample (synchronous, wait)
  if (media_type == Video)
    sample = gst_app_sink_try_pull_sample(GST_APP_SINK(m_vappsink), 0);
  else if (media_type == Audio)
    sample = gst_app_sink_try_pull_sample(GST_APP_SINK(m_aappsink), 0);
  if (sample == nullptr) {
    return core::Error::ERR_DATA_UNAVAILABLE;
  }

  m_lastDataTime = std::chrono::steady_clock::now();

  std::vector<uint8_t> temp_buffer;

  GstBuffer* buffer = gst_sample_get_buffer(sample);

  if (buffer->pts == GST_CLOCK_TIME_NONE) buffer->pts = 0;
  if (buffer->duration == GST_CLOCK_TIME_NONE) buffer->duration = 0;
  if (buffer->offset == GST_CLOCK_TIME_NONE) buffer->offset = 0;

  // map buffer to access memory
  GstMapInfo m;
  gst_buffer_map(buffer, &m, GST_MAP_READ);

  temp_buffer.resize(m.size);
  memcpy(temp_buffer.data(), m.data, m.size);

  m_incomingBytesTotal += m.size;
  m_incomingBytesPeriod += m.size;

  // Don't forget to unmap the buffer and unref the sample
  gst_buffer_unmap(buffer, &m);
  gst_sample_unref(sample);

  if (media_type == Video && m_videoSinkCallback) {
    NvCVImage decoded_img;
    if (m_decodeVideoFrames) {
      if (m_videoWidth == 0) {
        GstPad* videopad = gst_element_get_static_pad(m_vappsink, "sink");
        CHECK_NONNULL(videopad, core::Error::ERR_VIDEO_STREAM_ELEMENT_CREATION, "Could not get videosink pad 'sink'");
        GstCaps* videocaps = gst_pad_get_current_caps(videopad);
        if (videocaps) {
          LOG_DEBUG("Video caps:  %s", gst_caps_to_string(videocaps));
          GstStructure* structure = gst_caps_get_structure(videocaps, 0);
          const bool got_width = gst_structure_get_int(structure, "width", &m_videoWidth);
          const bool got_height = gst_structure_get_int(structure, "height", &m_videoHeight);
          const char* format = gst_structure_get_string(structure, "format");
          if (!got_width || !got_height || !format) {
            LOG_WARNING("Unable to get video format, disabling video");
          }
          LOG_DEBUG("Video format is: %s", format);
          gst_caps_unref(videocaps);
        }
        gst_object_unref(videopad);
      }
      CHECK_NVCV_SUCCESS(NvCVImage_Init(&decoded_img, m_videoWidth, m_videoHeight, m_videoWidth, temp_buffer.data(),
                                        NvCVImage_PixelFormat::NVCV_YUV420, NvCVImage_ComponentType::NVCV_U8, NVCV_NV12,
                                        NVCV_CPU));
      m_videoSinkCallback(nullptr, 0, reinterpret_cast<uint8_t*>(&decoded_img), sizeof(decoded_img));
    } else {
      // Parse SEI data
      uint32_t sei_payload_size = 0;
      uint8_t* sei_type5_payload =
          ParseSeiData(temp_buffer.data(), temp_buffer.size(), &sei_payload_size, "NVDS_MAXINEMETA");
      m_videoSinkCallback(sei_type5_payload, sei_payload_size, temp_buffer.data(), temp_buffer.size());
      if (sei_type5_payload) {
        free(sei_type5_payload);
      }
    }
  }
  if (media_type == Audio && m_audioSinkCallback) {
    m_audioSinkCallback(nullptr, 0, temp_buffer.data(), temp_buffer.size());
  }

  // Check if 1 second has passed
  if (m_reportStats.load()) {
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - m_startStats);
    if (duration.count() >= m_reportIntervalSeconds) {
      LOG_VERBOSE("%s: Incoming traffic : %.2f Mbit/s\n", NAME,
                  8.f * (float)m_incomingBytesPeriod / 1000000.f / (float)m_reportIntervalSeconds);
      m_incomingBytesPeriod = 0;
      m_startStats = std::chrono::steady_clock::now();
    }
  }

bail:
  return err;
}

bool RtpClientReceiver::ShouldRun() { return m_pipelineControl.shouldRun.load(); }

bool RtpClientReceiver::IsConnected() { return m_pipelineControl.isConnected.load(); }

void RtpClientReceiver::Disconnect() { m_pipelineControl.shouldRun.store(false); }

bool RtpClientReceiver::IsInitialized() const { return m_isInitialized; }

core::Error RtpClientReceiver::SetOnConnectCallback(const std::function<void(void)>& on_connect_callback) {
  m_pipelineControl.SetOnConnectCallback(on_connect_callback);
  return core::Error::SUCCESS;
}

core::Error RtpClientReceiver::Initialize(const std::string& host_address, int host_port, const std::string& rtsp_path,
                                          bool receive_audio, bool decode_video_frames) {
  core::Error err = core::Error::SUCCESS;
  GstCaps* caps = nullptr;
  std::stringstream location_sstr;

  CHECK_TRUE(rtsp_path.size() >= 1 && rtsp_path[0] == '/', core::Error::ERR_GENERAL, "RTSP path must start with /");

  if (m_isInitialized) {
    Shutdown();
  }
  m_shouldReceiveAudio = receive_audio;
  m_decodeVideoFrames = decode_video_frames;
  m_hostAddress = host_address;
  m_port = host_port;

  // Create the video elements
  m_pipeline = gst_pipeline_new("rtp-receiver");
  m_src = gst_element_factory_make("rtspsrc", "rtspsrc");

  m_vqueue = gst_element_factory_make("queue", "myvqueue");
  m_vrtpDepay = gst_element_factory_make("rtph264depay", "vrtph264depay");
  m_vparser = gst_element_factory_make("h264parse", "h264parser");

  m_vappsink = gst_element_factory_make("appsink", "vappsink");
  m_capsfilter = gst_element_factory_make("capsfilter", NULL);

  if (!m_pipeline || !m_vappsink || !m_src || !m_vrtpDepay || !m_vparser || !m_vqueue || !m_capsfilter) {
    BAIL(err, core::Error::ERR_VIDEO_STREAM_ELEMENT_CREATION);
  }

  if (m_decodeVideoFrames) {
    m_vdecoder = gst_element_factory_make("nvh264dec", NULL);
    m_vconverter = gst_element_factory_make("videoconvert", NULL);
    m_vcapsfilter = gst_element_factory_make("capsfilter", NULL);
    caps = gst_caps_from_string("video/x-raw,format=NV12");
    g_object_set(G_OBJECT(m_vcapsfilter), "caps", caps, NULL);
    gst_caps_unref(caps);
    // Minimize internal queue inside the video app sink, to improve A/V sync.
    // Should not do this with encoded output, since the decoder needs all packets.
    gst_app_sink_set_max_buffers(GST_APP_SINK(m_vappsink), 1);
    gst_app_sink_set_drop(GST_APP_SINK(m_vappsink), true);
    if (!m_vdecoder || !m_vconverter || !m_vcapsfilter) {
      BAIL(err, core::Error::ERR_VIDEO_STREAM_ELEMENT_CREATION);
    }
  }

  // Create the audio elements
  if (m_shouldReceiveAudio) {
    m_aqueue = gst_element_factory_make("queue", "myaqueue");
    m_artpDepay = gst_element_factory_make("rtpopusdepay", "artph264depay");
    m_aappsink = gst_element_factory_make("appsink", "aappsink");
    m_aparser = gst_element_factory_make("opusparse", "opusparser");
    if (!m_artpDepay || !m_aparser || !m_aqueue || !m_aappsink) {
      BAIL(err, core::Error::ERR_AUDIO_STREAM_ELEMENT_CREATION);
    }
  }

  // Add source pads
  g_signal_connect(m_src, "pad-added", G_CALLBACK(RtpClientReceiverCallbacks::PadAddedHandler), this);

  // Create the empty pipeline
  caps = gst_caps_from_string(
      "video/x-h264, stream-format=(string)byte-stream, alignment=(string)au, parsed=(boolean)true");
  g_object_set(G_OBJECT(m_capsfilter), "caps", caps, NULL);
  gst_caps_unref(caps);

  // Set the source location for rtsp
  location_sstr << "rtsp://" << m_hostAddress << ":" << m_port << rtsp_path;
  g_object_set(G_OBJECT(m_src), "location", location_sstr.str().c_str(), NULL);
  g_object_set(G_OBJECT(m_src), "udp-buffer-size", 104857600, NULL);
  g_object_set(G_OBJECT(m_src), "protocols", 1, NULL);  // GstRTSPLowerTrans::GST_RTSP_LOWER_TRANS_UDP
  g_object_set(G_OBJECT(m_src), "latency", 100, NULL);  // milliseconds
  g_object_set(G_OBJECT(m_src), "drop-on-latency", 1, NULL);
  g_object_set(m_vparser, "config-interval", 1, NULL);

  // Add app sinks
  g_object_set(G_OBJECT(m_vappsink), "sync", 0, NULL);
  if (m_shouldReceiveAudio) {
    g_object_set(G_OBJECT(m_aappsink), "sync", 0, NULL);
  }

  // add pipeline objects
  gst_bin_add_many(GST_BIN(m_pipeline), m_src, m_vqueue, m_vrtpDepay, m_vparser, m_vappsink, m_capsfilter, NULL);
  if (m_shouldReceiveAudio) {
    gst_bin_add_many(GST_BIN(m_pipeline), m_aqueue, m_artpDepay, m_aparser, m_aappsink, NULL);
  }

  if (!gst_element_link_many(m_vqueue, m_vrtpDepay, m_vparser, NULL)) {
    gst_object_unref(m_pipeline);
    BAIL(err, core::Error::ERR_VIDEO_STREAM_ELEMENT_LINK);
  }

  if (m_decodeVideoFrames) {
    gst_bin_add_many(GST_BIN(m_pipeline), m_vdecoder, m_vconverter, m_vcapsfilter, NULL);
    CHECK_TRUE(gst_element_link_many(m_vparser, m_vdecoder, m_vconverter, m_vcapsfilter, m_vappsink, NULL),
               core::ERR_VIDEO_STREAM_ELEMENT_LINK);
  } else {
    CHECK_TRUE(gst_element_link_many(m_vparser, m_capsfilter, m_vappsink, NULL), core::ERR_VIDEO_STREAM_ELEMENT_LINK);
  }

  if (m_shouldReceiveAudio) {
    if (!gst_element_link_many(m_aqueue, m_artpDepay, m_aparser, m_aappsink, NULL)) {
      gst_object_unref(m_pipeline);
      BAIL(err, core::Error::ERR_AUDIO_STREAM_ELEMENT_LINK);
    }
  }

  // Start pipeline
  GstStateChangeReturn ret = gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
  CHECK_TRUE(ret == GST_STATE_CHANGE_ASYNC, core::ERR_STREAM_STATE_CHANGE);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    return core::Error::ERR_STREAM_STATE_CHANGE;
  }
  GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(m_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "rtsp_receiver");

  // at this point pipeline is initialized
  m_isInitialized = true;

  // setup bus handler
  m_bus = gst_pipeline_get_bus(GST_PIPELINE(m_pipeline));

  // Start the bus thread
  m_pipelineControl.shouldRun.store(true);  // activate sink thread
  m_pipelineControl.isConnected.store(false);
  m_pipelineControl.busThreadIsRunning.store(true);

  m_lastDataTime = std::chrono::steady_clock::now();

  SpawnBusThread(m_pipeline, &m_pipelineControl, "RtpClientReceiverPipeline");
bail:
  return err;
}

core::Error RtpClientReceiver::SetVideoCallback(AppSinkCallback callback) {
  m_videoSinkCallback = callback;
  return core::Error::SUCCESS;
}

core::Error RtpClientReceiver::SetAudioCallback(AppSinkCallback callback) {
  core::Error err = core::Error::SUCCESS;
  BAIL_IF_FALSE(m_shouldReceiveAudio, err, core::Error::ERR_INITIALIZATION);
  m_audioSinkCallback = callback;
bail:
  return err;
}

bool RtpClientReceiver::HasVideoCallback() const { return m_videoSinkCallback != nullptr; }

bool RtpClientReceiver::HasAudioCallback() const { return m_audioSinkCallback != nullptr; }

bool RtpClientReceiver::NeedsReinitialization() const {
  const bool bus_thread_needs_restart = m_pipelineControl.shouldRun.load() && !m_pipelineControl.busThreadIsRunning;
  const bool no_recent_data = std::chrono::steady_clock::now() - std::chrono::milliseconds(1000) > m_lastDataTime;
  if (no_recent_data) LOG_DEBUG("RtpClientReceiver: No recent data received in the last 1000ms");
  if (bus_thread_needs_restart) LOG_DEBUG("RtpClientReceiver: Bus thread needs restart");
  return bus_thread_needs_restart || no_recent_data;
}

void RtpClientReceiver::Shutdown() {
  m_pipelineControl.shouldRun.store(false);

  JoinBusThread();

  // Free resources
  if (m_msg != nullptr) {
    gst_message_unref(m_msg);
  }
  // clean up pipeline
  gst_object_unref(m_bus);
  gst_element_set_state(m_pipeline, GST_STATE_NULL);
  gst_object_unref(m_pipeline);
  m_lastDataTime = std::chrono::steady_clock::time_point::min();
  m_isInitialized = false;
}

void RtpClientReceiver::SpawnBusThread(GstElement* pipeline, PipelineControl* control, const std::string& prefix) {
  // Start the bus thread
  m_busThread =
      std::thread([pipeline, control, prefix] { RtpClientReceiver::CodeThreadBus(pipeline, control, prefix); });
}

void RtpClientReceiver::JoinBusThread() { m_busThread.join(); }

core::Error RtpClientReceiver::BusProcessMsg(GstElement* pipeline, PipelineControl* control, GstMessage* msg,
                                             const std::string& prefix) {
  GstMessageType mType = GST_MESSAGE_TYPE(msg);
  switch (mType) {
    case GST_MESSAGE_ERROR:
      // Parse error and exit program, hard exit
      GError* err;
      gchar* dbg;
      gst_message_parse_error(msg, &err, &dbg);
      LOG_ERROR("%s: %s from %s", NAME, err->message, GST_OBJECT_NAME(msg->src));
      LOG_DEBUG("%s: %s", NAME, dbg);
      g_clear_error(&err);
      g_free(dbg);
      gst_element_set_state(pipeline, GST_STATE_READY);
      return core::Error::ERR_NETWORK_ERROR;
    case GST_MESSAGE_EOS:
      // Soft exit on EOS
      LOG_INFO("%s: EOS", NAME);
      gst_element_set_state(pipeline, GST_STATE_READY);
      return core::Error::ERR_NETWORK_EOS;
    case GST_MESSAGE_BUFFERING: {
      gint percent = 0;
      gst_message_parse_buffering(msg, &percent);
      LOG_INFO("%s: Buffering (%3d%%)", NAME, percent);
      // Wait until buffering is complete before start/resume playing
      if (percent < 100)
        gst_element_set_state(pipeline, GST_STATE_PAUSED);
      else
        gst_element_set_state(pipeline, GST_STATE_PLAYING);
      break;
    }
    case GST_MESSAGE_STATE_CHANGED:
      // Parse state change, print extra info for pipeline only
      LOG_DEBUG("%s: State changed on element %s", NAME, gst_element_get_name(GST_MESSAGE_SRC(msg)));
      if (GST_MESSAGE_SRC(msg) == GST_OBJECT(pipeline)) {
        GstState state_old, state_new, state_pending;
        gst_message_parse_state_changed(msg, &state_old, &state_new, &state_pending);
        LOG_INFO("%s: Pipeline changed from %s to %s", NAME, gst_element_state_get_name(state_old),
                 gst_element_state_get_name(state_new));
        if (state_new == GST_STATE_PLAYING) {
          control->active.store(true);
          control->isConnected.store(true);
          control->OnConnect();
        } else if (state_new == GST_STATE_PAUSED) {
          control->active.store(false);
        }
      }
      break;
    case GST_MESSAGE_CLOCK_LOST:
      LOG_INFO("%s: Message clock lost.Getting new clock", NAME);
      // Get a new clock
      gst_element_set_state(pipeline, GST_STATE_PAUSED);
      gst_element_set_state(pipeline, GST_STATE_PLAYING);
      break;
    case GST_MESSAGE_STEP_START:
      LOG_INFO("%s: Step start", NAME);
      break;
    case GST_MESSAGE_STREAM_STATUS:
      LOG_INFO("%s: Stream status", NAME);
      break;
    case GST_MESSAGE_ELEMENT:
      LOG_INFO("%s: Message element", NAME);
      break;
    default:
      LOG_DEBUG("%s: Unknown message", NAME);
  }
  return core::Error::SUCCESS;
}

void RtpClientReceiver::CodeThreadBus(GstElement* pipeline, PipelineControl* control, const std::string& prefix) {
  LOG_INFO("%s: Bus thread started : %s", NAME, prefix.c_str());
  control->busThreadIsRunning.store(true);
  GstBus* bus = gst_element_get_bus(pipeline);

  core::Error err;
  while (control->shouldRun.load()) {
    GstClockTime wait_nanoseconds = 1000000000;  // 1 Sec
    GstMessage* msg = gst_bus_timed_pop(bus, wait_nanoseconds);
    if (!msg) continue;
    err = BusProcessMsg(pipeline, control, msg, prefix);
    gst_message_unref(msg);
    if (err != core::Error::SUCCESS) {
      LOG_INFO("%s: %s", NAME, core::ErrorStringFromCode(err));
      break;
    }
  }
  gst_object_unref(bus);
  LOG_INFO("%s: Bus thread finished : %s", NAME, prefix.c_str());
  control->busThreadIsRunning.store(false);
}

}  // namespace networkmodule
}  // namespace modules
}  // namespace nv3dvc
