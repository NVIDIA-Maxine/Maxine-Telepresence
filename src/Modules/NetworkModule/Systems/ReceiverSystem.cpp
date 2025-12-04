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

#include "ReceiverSystem.h"

#include <chrono>  // NOLINT(build/c++11) (cpplint outside Chromium: https://github.com/google/styleguide/issues/483)
#include <thread>  // NOLINT(build/c++11) (cpplint outside Chromium: https://github.com/google/styleguide/issues/483)

#include "Core/EntityComponentSystem/Entity.h"
#include "Core/Error.h"
#include "Modules/CommonModule/Components/CallbackComponent.h"
#include "Modules/CommonModule/Components/VideoFrameComponent.h"
#include "Modules/ControlModule/Components/PoseCalibrationBehavior.h"
#include "Modules/NetworkModule/Components/StreamSourceComponent.h"
#include "cuda.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Macros                                                                                                           ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace nv3dvc {
namespace modules {
namespace networkmodule {
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

ReceiverSystem::ReceiverSystem() {}

core::Error ReceiverSystem::Initialize() { return core::Error::SUCCESS; }

core::Error ReceiverSystem::Uninitialize() { return core::Error::SUCCESS; }

core::Error ReceiverSystem::OnLoadScene(core::ecs::registry::EntityRegistry* reg) {
  core::Error err = core::Error::SUCCESS;
  // Initialize all sources
  auto entities = reg->view<components::StreamSourceComponent>();
  for (auto& entity : entities) {
    auto& stream_source = entity.GetComponent<components::StreamSourceComponent>();
    if (!stream_source.IsInitialized()) {
      CHECK_SUCCESS(stream_source.Initialize());
    }
    if (stream_source.decode_video_frames && !entity.HasComponent<commonmodule::components::VideoFrameComponent>()) {
      // If the StreamSourceComponent decodes the frames internally, the entity should have a VideoFrameComponent.
      LOG_WARNING(
          "The entity's StreamSourceComponent is decoding video frames internally, but the entity has no "
          "VideoFrameComponent. The decoded video frames will be discarded.");
    }
    if (!stream_source.decode_video_frames &&
        !entity.HasComponent<commonmodule::components::EncodedVideoCallbackComponent>()) {
      LOG_WARNING(
          "The entity's StreamSourceComponent is not decoding video frames internally, but the entity has no "
          "EncodedVideoCallbackComponent. The received video packets will be discarded.");
    }
    stream_source.SetOnConnectCallback([entity]() {
      if (entity.HasComponent<modules::controlmodule::components::PoseCalibrationBehavior>()) {
        auto& pose_calibration = entity.GetComponent<modules::controlmodule::components::PoseCalibrationBehavior>();
        pose_calibration.trigger_calibrate.OnChange();
      }
    });
  }
bail:
  return err;
}

core::Error ReceiverSystem::Run(core::ecs::registry::EntityRegistry* reg, float dt) {
  core::Error err = core::Error::SUCCESS;
  m_deltaTime = dt;
  // Set up StreamSourceComponent callbacks.
  for (auto& entity : reg->view<components::StreamSourceComponent>()) {
    auto& stream_source = entity.GetComponent<components::StreamSourceComponent>();
    // Skip entities who already have a callback set.
    if (stream_source.HasVideoCallback()) continue;

    if (stream_source.decode_video_frames && entity.HasComponent<commonmodule::components::VideoFrameComponent>()) {
      cudaStream_t cu_stream = GetStream();
      auto& video_frame_component = entity.GetComponent<commonmodule::components::VideoFrameComponent>();
      CHECK_SUCCESS(stream_source.SetVideoCallback(
          [entity, &video_frame_component, &err, cu_stream](uint8_t* /*metadata_ptr*/, size_t /*metadata_size*/,
                                                            uint8_t* buffer, size_t size) {
            auto dst_frame_data_image = video_frame_component.GetImagePtr();
            NvCVImage& decoded_frame = *reinterpret_cast<NvCVImage*>(buffer);
            CHECK_TRUE(sizeof(decoded_frame) == size, core::Error::ERR_GENERAL);

            if (dst_frame_data_image->width != decoded_frame.width ||
                dst_frame_data_image->height != decoded_frame.height) {
              LOG_DEBUG("Re-allocating VideoFrameComponent from %d x %d to %d x %d", dst_frame_data_image->width,
                        dst_frame_data_image->height, decoded_frame.width, decoded_frame.height);
              // Initialize the video frame component.
              CHECK_SUCCESS(video_frame_component.AllocateImageBuffer(
                  decoded_frame.width, decoded_frame.height, NvCVImage_PixelFormat::NVCV_BGR,
                  NvCVImage_ComponentType::NVCV_U8, NVCV_CHUNKY, NVCV_CUDA, 1));
            }
            // Write the decoded frame output
            CHECK_NVCV_SUCCESS(
                NvCVImage_Transfer(&decoded_frame, dst_frame_data_image, 1.0f, cu_stream, nullptr /* &m_tmpImage */));
            video_frame_component.UpdateRgbaImage(cu_stream, nullptr);
            CHECK_CU_SUCCESS(cuStreamSynchronize(cu_stream));
          bail:
            return;
          }));
    } else if (!stream_source.decode_video_frames &&
               entity.HasComponent<commonmodule::components::EncodedVideoCallbackComponent>()) {
      auto& callback_component = entity.GetComponent<commonmodule::components::EncodedVideoCallbackComponent>();
      CHECK_SUCCESS(stream_source.SetVideoCallback(
          [this, &callback_component](uint8_t* metadata_payload, size_t metadata_size, uint8_t* buffer, size_t size) {
            core::Error err = core::Error::SUCCESS;
            // When the StreamSourceComponent gets a new packet, we want to fire the EncodedVideoCallbackComponent with
            // the payload. We also attach the latest value of `m_deltaTime` (because `this` is captured).
            commonmodule::components::EncodedVideoCallbackComponent::Buffers buffers = {metadata_payload, metadata_size,
                                                                                        buffer, size, m_deltaTime};
            CHECK_SUCCESS(callback_component.Fire(&buffers, sizeof(buffers), nullptr));
          bail:
            return;
          }));
    }
  }

  // If we're running in a tight loop and the previous iteration was waiting for data, we want to wait a moment.
  std::this_thread::sleep_until(m_nextPoll);

  // Poll for data on all streams.
  bool any_stream_received_data = false;
  for (auto& entity : reg->view<components::StreamSourceComponent>()) {
    auto& stream_source = entity.GetComponent<components::StreamSourceComponent>();
    if (stream_source.IsConnected()) {
      core::Error e = stream_source.PollVideoAppSink();
      if (e == core::Error::SUCCESS) {
        any_stream_received_data = true;
      } else if (e == core::Error::ERR_DATA_UNAVAILABLE) {
        e = core::Error::SUCCESS;
      }
      if (err == core::Error::SUCCESS) err = e;  // Save first error
    }
  }

  if (!any_stream_received_data) {
    // If all the streams are waiting for data, we don't want to spin-wait by polling continuously in a tight loop.
    // We could sleep here, but that might delay another system that is running on the same thread.
    m_nextPoll = std::chrono::steady_clock::now() + std::chrono::milliseconds(20);
  }
bail:
  return err;
}

}  // namespace systems
}  // namespace networkmodule
}  // namespace modules
}  // namespace nv3dvc
