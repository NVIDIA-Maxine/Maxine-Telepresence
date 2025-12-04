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

#include "EncoderSystem.h"

#include <utility>
#include <vector>

#include "Core/Error.h"
#include "Core/Serialization/CustomJsonTypesGlm.h"
#include "Core/Util/Logger.h"
#include "Modules/AudioModule/Components/AudioSinkComponent.h"
#include "Modules/CodecModule/Components/AudioEncoderComponent.h"
#include "Modules/CodecModule/Components/FrameEncoderComponent.h"
#include "Modules/CommonModule/Components/DataBufferComponent.h"
#include "Modules/CommonModule/Components/VideoFrameComponent.h"
#include "Modules/TrackingModule/Components/TrackedHeadComponent.h"
#include "nvCVStatus.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Macros                                                                                                           ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace nv3dvc {
namespace modules {
namespace codecmodule {

struct SourceVideoPackage {
  int clientId;
  glm::vec2 gazeAngle;
  glm::quat headPoseQuaternion;
  glm::vec3 headPoseTranslation;
  float headScale;
  float confidence;
  glm::vec3 rawTrackingPoseEuler;
  int64_t timestamp;
};

void from_json(const nlohmann::json& j,
               SourceVideoPackage& source_video_package) {  // NOLINT: [runtime/references] (Owned by nlohmann::json)
  if (j.contains("clientId")) j.at("clientId").get_to(source_video_package.clientId);
  if (j.contains("gazeAngle")) j.at("gazeAngle").get_to(source_video_package.gazeAngle);
  if (j.contains("headPoseQuaternion")) j.at("headPoseQuaternion").get_to(source_video_package.headPoseQuaternion);
  if (j.contains("headPoseTranslation")) j.at("headPoseTranslation").get_to(source_video_package.headPoseTranslation);
  if (j.contains("headScale")) j.at("headScale").get_to(source_video_package.headScale);
  if (j.contains("confidence")) j.at("confidence").get_to(source_video_package.confidence);
  if (j.contains("rawTrackingPoseEuler"))
    j.at("rawTrackingPoseEuler").get_to(source_video_package.rawTrackingPoseEuler);
  if (j.contains("timestamp")) j.at("timestamp").get_to(source_video_package.timestamp);
}

void to_json(nlohmann::json& j,  // NOLINT: [runtime/references] (Owned by nlohmann::json)
             const SourceVideoPackage& source_video_package) {
  j = nlohmann::json{{"clientId", source_video_package.clientId},
                     {"gazeAngle", source_video_package.gazeAngle},
                     {"headPoseQuaternion", source_video_package.headPoseQuaternion},
                     {"headPoseTranslation", source_video_package.headPoseTranslation},
                     {"headScale", source_video_package.headScale},
                     {"confidence", source_video_package.confidence},
                     {"rawTrackingPoseEuler", source_video_package.rawTrackingPoseEuler},
                     {"timestamp", source_video_package.timestamp}};
}

namespace systems {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Static function declarations                                                                                     ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Static function definitions                                                                                      ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Member function definitions                                                                                      ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EncoderSystem::EncoderSystem(core::engine::Engine* engine) : m_engine(engine) {}

core::Error EncoderSystem::Initialize() {
  core::Error err = core::Error::SUCCESS;
  CHECK_NONNULL(m_engine, core::Error::ERR_NULL_POINTER, "EncoderSystem requires Engine");
bail:
  return err;
}

core::Error EncoderSystem::Uninitialize() { return core::Error::SUCCESS; }

core::Error EncoderSystem::OnLoadScene(core::ecs::registry::EntityRegistry* reg) {
  core::Error err = core::Error::SUCCESS;

  // Audio sink ==> audio encoder.
  auto audio_sink_encoder_entities = reg->view<audiomodule::components::AudioSinkComponent,  //
                                               codecmodule::components::AudioEncoderComponent>();
  LOG_DEBUG("Found %d entities with AudioSinkComponent & AudioEncoderComponent",
            (int)audio_sink_encoder_entities.size());
  for (auto& entity : audio_sink_encoder_entities) {
    auto& sink_component = entity.GetComponent<audiomodule::components::AudioSinkComponent>();
    auto& encoder_component = entity.GetComponent<codecmodule::components::AudioEncoderComponent>();
    if (!sink_component.IsInitialized()) {
      if (!sink_component.Initialize()) {
        LOG_ERROR("Failed to initialize AudioSinkComponent");
        return core::ERR_GENERAL;
      }
    }
    CHECK_SUCCESS(encoder_component.Initialize(sink_component.GetSampleRate(), sink_component.GetNumChannels()),
                  "Failed to initialize AudioEncoderComponent");
    encoder_component.SetEncodeCallback(std::bind(&audiomodule::components::AudioSinkComponent::PopSinkAudio,
                                                  &sink_component, std::placeholders::_1, std::placeholders::_2,
                                                  std::placeholders::_3));
  }
bail:
  return err;
}

core::Error EncoderSystem::Run(core::ecs::registry::EntityRegistry* reg, float dt) {
  core::Error err = core::Error::SUCCESS;
  for (auto& entity : reg->view<components::FrameEncoderComponent, commonmodule::components::VideoFrameComponent,
                                commonmodule::components::EncodedVideoBufferComponent>()) {
    auto& frame_encoder = entity.GetComponent<components::FrameEncoderComponent>();
    auto& video_frame = entity.GetComponent<commonmodule::components::VideoFrameComponent>();
    auto& encoded_buffer = entity.GetComponent<commonmodule::components::EncodedVideoBufferComponent>();

    NvCVImage* image = video_frame.GetImagePtr();
    if (!image->pixels) {
      LOG_DEBUG("EncoderSystem got video frame with empty image");
      continue;
    }

    // Input format is RGBA -- kInputFormatNvCVImage and kInputFormatNvEnc must match.
    constexpr NvCVImage_PixelFormat kInputFormatNvCVImage = NVCV_RGBA;
    // Word-ordered format with R in the lowest 8 bits and A in the highest 8 bits.
    constexpr NV_ENC_BUFFER_FORMAT kInputFormatNvEnc = NV_ENC_BUFFER_FORMAT_ABGR;

    if (!frame_encoder.IsInitialized()) {
      const NvEncoderInitParam encode_options(" -codec h264 -tuninginfo ultralowlatency -preset p3 -profile high ",
                                              nullptr, true);
      CUcontext context = m_engine->GetCudaContext();
      CHECK_SUCCESS(frame_encoder.Initialize(context, image->width, image->height, encode_options, kInputFormatNvEnc));
    }

    trackingmodule::components::TrackedHeadComponent* tracked_head = nullptr;
    if (entity.HasParent() && entity.GetParent().HasComponent<trackingmodule::components::TrackedHeadComponent>()) {
      tracked_head = &entity.GetParent().GetComponent<trackingmodule::components::TrackedHeadComponent>();
    }
    const NvEncInputFrame* frame = frame_encoder.GetNextInputFrame();

    std::vector<std::vector<uint8_t>> packets;
    if (frame->inputPtr) {
      // Wrap the encoder's input `frame` as a NvCVImage, so that we can easily copy into it from `image`.
      NvCVImage image_wrapper;
      CHECK_NVCV_SUCCESS(NvCVImage_Init(&image_wrapper, image->width, image->height, image->width * 4, frame->inputPtr,
                                        kInputFormatNvCVImage, NVCV_U8, NVCV_CHUNKY, NVCV_GPU));
      CHECK_NVCV_SUCCESS(NvCVImage_Transfer(image, &image_wrapper, 1.0f, GetStream(), &m_tmpImg));

      SourceVideoPackage source_video_package;  // Metadata package to be sent as SEI

      source_video_package.clientId = 1;  // Unused but reserved for multi client setups
      source_video_package.gazeAngle = {0.0f, 0.0f};
      source_video_package.headPoseQuaternion = {1.0f, 0.0f, 0.0f, 0.0f};
      source_video_package.headPoseTranslation = {0.0f, 0.0f, 0.0f};
      source_video_package.headScale = 0.5f;
      source_video_package.confidence = 0.0f;
      source_video_package.rawTrackingPoseEuler = {0.0f, 0.0f, 0.0f};
      source_video_package.timestamp = m_engine->GetTimeStamp();

      if (tracked_head) {
        glm::vec3 tracked_translation_m = tracked_head->TrackedTranslationWithOffset() * 0.01f;  // cm to m
        source_video_package.headPoseQuaternion = tracked_head->face_box_to_display_rotation;
        source_video_package.headPoseTranslation = tracked_translation_m;
        source_video_package.headScale = 0.5f;
        source_video_package.rawTrackingPoseEuler = tracked_head->raw_pose_euler;
        source_video_package.confidence = tracked_head->tracking_confidence;
      }

      nlohmann::json metadata_json;
      metadata_json["SourceVideoPackage"] = nlohmann::json(source_video_package);

      std::vector<uint8_t> sei_payload = nlohmann::json::to_cbor(metadata_json);
      constexpr size_t kUuidMsgSize = 16;
      uint8_t uuid_msg[kUuidMsgSize] = "NVDS_MAXINEMETA";
      std::vector<uint8_t> sei_msg;
      sei_msg.resize(kUuidMsgSize);
      memcpy(sei_msg.data(), uuid_msg, kUuidMsgSize);

      sei_msg.insert(sei_msg.end(), std::make_move_iterator(sei_payload.begin()),
                     std::make_move_iterator(sei_payload.end()));

      bool encode_last_frame = false;

      CHECK_SUCCESS(frame_encoder.EncodeLowLatency(&packets, &sei_msg, encode_last_frame));

      encoded_buffer.Append(std::move(packets));
    }
  }
bail:
  return err;
}

}  // namespace systems
}  // namespace codecmodule
}  // namespace modules
}  // namespace nv3dvc
