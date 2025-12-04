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

#include "CameraCaptureSystem.h"

#include <cuda_runtime_api.h>

#include "Core/EntityComponentSystem/Entity.h"
#include "Core/Error.h"
#include "Modules/CaptureModule/Components/WebCameraComponent.h"
#include "Modules/CommonModule/Components/CallbackComponent.h"
#include "Modules/CommonModule/Components/CameraCalibrationComponent.h"
#include "Modules/CommonModule/Components/VideoFrameComponent.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Macros                                                                                                           ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace nv3dvc {
namespace modules {
namespace capturemodule {
namespace systems {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Member function definitions                                                                                      ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CameraCaptureSystem::CameraCaptureSystem(core::engine::Engine* engine) : m_engine(engine) {}

nv3dvc::core::Error CameraCaptureSystem::Initialize() {
  core::Error err = core::Error::SUCCESS;
  CHECK_NONNULL(m_engine, core::Error::ERR_NULL_POINTER, "CameraCaptureSystem requires Engine");
bail:
  return err;
}

nv3dvc::core::Error CameraCaptureSystem::OnLoadScene(core::ecs::registry::EntityRegistry* reg) {
  core::Error ret = core::Error::SUCCESS;
  for (auto& entity :
       reg->view<components::WebCameraComponent, commonmodule::components::CameraCalibrationComponent>()) {
    auto& web_camera_component = entity.GetComponent<components::WebCameraComponent>();
    auto& camera_calibration = entity.GetComponent<commonmodule::components::CameraCalibrationComponent>();
    core::Error err = web_camera_component.Initialize(&camera_calibration, m_engine->GetCudaContext());
    if (ret == core::Error::SUCCESS) ret = err;  // Save first error

    // If the web camera provides audio and the scene wants to connect it with a WebCameraAudioCallbackComponent, hook
    // up the callback here. Anyone who calls WebCameraAudioCallbackComponent::Fire() will get the audio.
    if (web_camera_component.HasAudio()) {
      if (!entity.HasComponent<commonmodule::components::WebCameraAudioCallbackComponent>()) {
        LOG_DEBUG("WebCameraComponent has audio - adding WebCameraAudioCallbackComponent to entity");
        try {
          entity.AddComponent<commonmodule::components::WebCameraAudioCallbackComponent>();
        } catch (std::exception& e) {
          LOG_ERROR("Unable to add WebCameraAudioCallbackComponent to entity: %s", e.what());
          if (ret == core::Error::SUCCESS) ret = core::Error::ERR_SCENE;  // Save first error
        }
      }
      auto& webcam_audio_cb_component =
          entity.GetComponent<commonmodule::components::WebCameraAudioCallbackComponent>();
      int sample_rate = 0, num_channels = 0;
      err = web_camera_component.GetAudioFormat(&sample_rate, &num_channels);
      if (err == core::Error::SUCCESS) {
        webcam_audio_cb_component.SetFormat(sample_rate, num_channels);
        webcam_audio_cb_component.SetOnFiredCallback(
            [&](void* data_ptr, size_t data_size, size_t* pulled_size, int /* data_type */) -> core::Error {
              core::Error err = core::Error::SUCCESS;
              if (pulled_size) *pulled_size = 0;
              CHECK_NONNULL(data_ptr, core::Error::ERR_NULL_POINTER);
              float* const dst_data = static_cast<float*>(data_ptr);
              const int dst_samples = static_cast<int>(data_size);
              int pulled_samples = 0;
              err = web_camera_component.GetAudioData(dst_data, dst_samples, &pulled_samples);
              if (pulled_size) *pulled_size = static_cast<size_t>(pulled_samples);
            bail:
              return err;
            });
      } else {
        LOG_WARNING("Unable to get audio format from WebCameraComponent, audio will not be provided");
        if (ret == core::Error::SUCCESS) ret = err;  // Save first error
      }
    }
  }
  return ret;
}

nv3dvc::core::Error CameraCaptureSystem::Uninitialize() { return nv3dvc::core::SUCCESS; }

nv3dvc::core::Error CameraCaptureSystem::Run(core::ecs::registry::EntityRegistry* reg, float dt) {
  core::Error err = core::Error::SUCCESS;

  auto view = reg->view<components::WebCameraComponent, commonmodule::components::CameraCalibrationComponent,
                        commonmodule::components::VideoFrameComponent>();
  for (auto& entity : view) {
    auto& web_camera_component = entity.GetComponent<components::WebCameraComponent>();
    auto& camera_calibration_component = entity.GetComponent<commonmodule::components::CameraCalibrationComponent>();
    auto& frame_capture_component = entity.GetComponent<commonmodule::components::VideoFrameComponent>();

    const commonmodule::CameraCalibration calibration = camera_calibration_component.GetCameraCalibration();
    NvCVImage* image_gpu = frame_capture_component.GetImagePtr();
    if (image_gpu->width != calibration.image_width || image_gpu->height != calibration.image_height) {
      CHECK_SUCCESS(frame_capture_component.AllocateImageBuffer(calibration.image_width, calibration.image_height,
                                                                NVCV_BGR, NVCV_U8, NVCV_CHUNKY, NVCV_CUDA, 1));
    }

    NvCVImage frame = {};
    cudaStream_t cu_stream = GetStream();
    err = web_camera_component.GetFrame(&frame, cu_stream);
    if (err == core::Error::SUCCESS) {
      CHECK_NVCV_SUCCESS(NvCVImage_Transfer(&frame, image_gpu, 1.0f, cu_stream, &m_tmpImage));
      CHECK_SUCCESS(frame_capture_component.UpdateRgbaImage(cu_stream, &m_tmpImage));
      CHECK_CUDA_SUCCESS(cudaStreamSynchronize(cu_stream));
      frame_capture_component.SetIsMirrored(web_camera_component.GetCameraDescriptor().flip_horizontal);
    } else if (err == core::Error::ERR_DATA_UNAVAILABLE) {
      err = core::Error::SUCCESS;
    }
  }

bail:
  return err;
}

}  // namespace systems
}  // namespace capturemodule
}  // namespace modules
}  // namespace nv3dvc
