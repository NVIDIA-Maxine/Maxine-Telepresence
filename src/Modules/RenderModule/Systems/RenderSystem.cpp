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

#include "RenderSystem.h"

#include <memory>
#include <vector>

#include "Core/EntityComponentSystem/Entity.h"
#include "Core/Error.h"
#include "Core/Events/WindowEvent.h"
#include "Core/Util/Logger.h"
#include "Modules/CommonModule/CommonModule.h"
#include "Modules/CommonModule/Components/CallbackComponent.h"
#include "Modules/CommonModule/Components/CameraComponent.h"
#include "Modules/CommonModule/Components/TransformComponent.h"
#include "Modules/RenderModule/Components/CubeMapComponent.h"
#include "Modules/RenderModule/Components/DisplayComponent.h"
#include "Modules/RenderModule/Components/RenderableTriplaneComponent.h"
#include "Modules/RenderModule/Components/StereoViewComponent.h"
#include "Modules/RenderModule/Components/TexturedQuadComponent.h"
#include "Modules/TriplaneModule/Components/TriplaneBufferComponent.h"
#include "Modules/TriplaneModule/Components/TriplaneVolumeComponent.h"
#include "glm/gtx/norm.hpp"

/// @brief Record the last rendered frame into either GPU or CPU memory by triggering the recording callback
/// @param[in] recording_cb_component The recording callback component which will receive the callback
/// @param[in] renderer               The Renderer which last rendered frame to record
/// @param[in] to_gpu                 Whether to record the frame to GPU, if false, the frame will be recorded to CPU
/// memory
/// @param[in] tmp_recording_buffer   A temporary recording buffer which may get reallocated. Only required when to_gpu
/// == false
/// @param[in] cu_stream              A cuda stream on which to map texture resources. Only used if to_gpu == true
/// @return    nv3dvc::core::Error::SUCCESS If successful
static nv3dvc::core::Error RecordFrame(
    nv3dvc::modules::commonmodule::components::RecordingCallbackComponent* recording_cb_component,
    nv3dvc::core::rendering::Renderer* renderer, const bool to_gpu, std::vector<uint8_t>* tmp_recording_buffer,
    CUstream cu_stream) {
  nv3dvc::core::Error err = nv3dvc::core::Error::SUCCESS;
  cudaArray* cuda_array = nullptr;  // Only used when to_gpu
  size_t pushed_bytes = 0;
  size_t data_size = 0;
  nv3dvc::modules::commonmodule::components::RecordingCallbackComponent::VideoFormat format;
  CHECK_NONNULL(recording_cb_component, nv3dvc::core::Error::ERR_NULL_POINTER);
  CHECK_NONNULL(renderer, nv3dvc::core::Error::ERR_NULL_POINTER);
  CHECK_TRUE(to_gpu || tmp_recording_buffer != nullptr, nv3dvc::core::Error::ERR_NULL_POINTER);
  format.width = renderer->GetFramebufferWidth();
  format.height = renderer->GetFramebufferHeight();
  format.pitch = format.width * 4 * sizeof(uint8_t);  // RGBA
  format.pixel_format = NVCV_RGBA;
  format.component_type = NVCV_U8;
  format.planar = NVCV_CHUNKY;
  format.gpu_mem = to_gpu ? NVCV_CUDA_ARRAY : NVCV_CPU;
  CHECK_SUCCESS(recording_cb_component->Fire(
      &format, sizeof(format), nullptr,
      nv3dvc::modules::commonmodule::components::RecordingCallbackComponent::VIDEO_FORMAT));
  data_size = format.pitch * format.height;
  if (to_gpu) {
    CHECK_SUCCESS(renderer->MapTextureResource(&cuda_array, cu_stream));
    CHECK_SUCCESS(recording_cb_component->Fire(
        cuda_array, data_size, &pushed_bytes,
        nv3dvc::modules::commonmodule::components::RecordingCallbackComponent::VIDEO_DATA));
    CHECK_SUCCESS(renderer->UnMapTextureResource(cu_stream));
  } else {
    tmp_recording_buffer->resize(data_size);
    renderer->ReadPixelsRgbaU8(tmp_recording_buffer->data(), 0, 0, format.width, format.height);
    // Flip image vertically.
    for (size_t y = 0; y < format.height / 2; ++y) {
      std::swap_ranges(tmp_recording_buffer->begin() + 4 * static_cast<size_t>(format.width) * y,
                       tmp_recording_buffer->begin() + 4 * static_cast<size_t>(format.width) * (y + 1),
                       tmp_recording_buffer->begin() + 4 * static_cast<size_t>(format.width) * (format.height - y - 1));
    }
    CHECK_SUCCESS(recording_cb_component->Fire(
        tmp_recording_buffer->data(), tmp_recording_buffer->size(), &pushed_bytes,
        nv3dvc::modules::commonmodule::components::RecordingCallbackComponent::VIDEO_DATA));
  }
  if (pushed_bytes != data_size) {
    BAIL(err, nv3dvc::core::Error::ERR_DATA_UNAVAILABLE);
  }
bail:
  return err;
}

namespace nv3dvc {
namespace modules {
namespace rendermodule {
namespace systems {

RenderSystem::RenderSystem(core::engine::Engine* engine)
    : m_engine(engine), m_windowSizeX(1920), m_windowSizeY(1080), m_isInitialized(false) {
  renderer = std::make_unique<core::rendering::Renderer>("renderer");
  AddSubOwner(renderer.get());
}

RenderSystem::~RenderSystem() {}

nv3dvc::core::Error RenderSystem::Initialize() { return core::Error::SUCCESS; }

nv3dvc::core::Error RenderSystem::Uninitialize() {
  renderer = nullptr;
  return core::Error::SUCCESS;
}

core::Error RenderSystem::OnLoadScene(core::ecs::registry::EntityRegistry* reg) {
  core::Error err = core::Error::SUCCESS;
  auto display_view = reg->view<components::DisplayComponent>();
  core::rendering::display::Display* display_ptr = nullptr;
  int num_views = 0;
  int quilt_cols = 1;
  int quilt_rows = 1;
  uint32_t render_width = 512;
  uint32_t render_height = 512;
  for (core::ecs::Entity& entity : display_view) {
    auto& display = entity.GetComponent<components::DisplayComponent>();
    if (num_views != 0 && num_views != display.GetDesiredNumViews()) {
      LOG_ERROR("Only one display type selection supported");
      BAIL(err, core::Error::ERR_SCENE);
    }
    num_views = display.GetDesiredNumViews();
    quilt_cols = display.GetDesiredQuiltCols();
    quilt_rows = display.GetDesiredQuiltRows();
    render_width = display.GetDesiredRenderWidth();
    render_height = display.GetDesiredRenderHeight();
    display_ptr = display.GetDisplayPtr();
  }
  for (core::ecs::Entity& entity : reg->view<components::CubeMapComponent>()) {
    auto& cube_map = entity.GetComponent<components::CubeMapComponent>();
    err = cube_map.Load();
    // Failure here ok
    if (display_ptr != nullptr && entity.HasComponent<commonmodule::components::TransformComponent>()) {
      auto& transform = entity.GetComponent<commonmodule::components::TransformComponent>();
      float width_m = display_ptr->ScreenWidthMm() * 0.001f;
      float height_m = display_ptr->ScreenHeightMm() * 0.001f;
      transform.scale = {width_m, height_m, width_m * 1.5f};
    }
  }
  for (core::ecs::Entity& entity : reg->view<components::TexturedQuadComponent>()) {
    auto& quad = entity.GetComponent<components::TexturedQuadComponent>();
    err = quad.Load();
  }
  if (num_views == 0) num_views = 1;
  err = renderer->Uninitialize();
  BAIL_IF_APERR(err);
  err = renderer->Initialize(render_width, render_height, num_views, quilt_cols, quilt_rows);
  BAIL_IF_APERR(err);

bail:
  if (err == core::Error::SUCCESS) {
    m_isInitialized = true;
  }
  return err;
}

core::Error RenderSystem::OnUnloadScene(core::ecs::registry::EntityRegistry* reg) {
  core::Error err = core::Error::SUCCESS;
  CHECK_SUCCESS(renderer->Uninitialize());
bail:
  m_isInitialized = false;
  return err;
}

nv3dvc::core::Error RenderSystem::Run(core::ecs::registry::EntityRegistry* reg, float dt) {
  core::Error err = core::Error::SUCCESS;
  if (!reg) return core::Error::ERR_REGISTRY;
  renderer->Clear();  // Clear, regardless if initialization failed
  if (!m_isInitialized) {
    return core::Error::ERR_INITIALIZATION;
  }

  // Read from triplane buffer if it exists
  for (auto& entity : reg->view<components::RenderableTriplaneComponent>()) {
    auto& renderable_triplane = entity.GetComponent<components::RenderableTriplaneComponent>();
    triplanemodule::TriplaneFrame* triplane_frame = nullptr;
    if (entity.HasComponent<triplanemodule::components::TriplaneBufferComponent>() &&
        (triplane_frame =
             entity.GetComponent<triplanemodule::components::TriplaneBufferComponent>().GetReadableTriplaneFrame())) {
      if (entity.HasComponent<commonmodule::components::TransformComponent>()) {
        auto& transform = entity.GetComponent<commonmodule::components::TransformComponent>();
        transform.SetTranslation(triplane_frame->triplane_package.head_pose_translation +
                                 glm::rotate(triplane_frame->triplane_package.head_pose_quaternion,
                                             triplane_frame->triplane_package.triplane_translation));
        transform.SetRotation(triplane_frame->triplane_package.head_pose_quaternion *
                              triplane_frame->triplane_package.triplane_quaternion);

        float focal_scale_strength = renderable_triplane.FocalScaleStrength();
        float pose_mode_scale = 1.0f + triplane_frame->triplane_package.focal_scale * focal_scale_strength;
        transform.scale = glm::vec3(triplane_frame->triplane_package.head_scale * pose_mode_scale);
      }
      triplane_frame->triplane_volume.confidence =
          renderable_triplane.FilterConfidence(triplane_frame->triplane_package.confidence, dt);
      renderable_triplane.SetPtr(&triplane_frame->triplane_volume);
      if (m_engine) {
        uint32_t entity_id = static_cast<uint32_t>(entity);
        m_engine->SetTimeStampForEntity(entity_id, triplane_frame->triplane_package.timestamp);
      }
      entity.GetComponent<triplanemodule::components::TriplaneBufferComponent>().SignalConsume();
    } else if (entity.HasComponent<triplanemodule::components::TriplaneVolumeComponent>()) {
      // If there is no triplane buffer frame, we may be able to render a triplane volume component
      auto& triplane_volume = entity.GetComponent<triplanemodule::components::TriplaneVolumeComponent>();
      if (triplane_volume.GetTriplaneVolumePtr()->triplanes.pixels) {
        renderable_triplane.SetPtr(triplane_volume.GetTriplaneVolumePtr());
      }
    }
  }

  commonmodule::components::CameraComponent* main_camera = commonmodule::components::CameraComponent::MainCamera();
  if (!main_camera) return nv3dvc::core::Error::SUCCESS;
  if (!reg) return core::Error::ERR_REGISTRY;

  for (core::ecs::Entity& camera_entity : reg->view<commonmodule::components::CameraComponent>()) {
    auto& camera_component = camera_entity.GetComponent<commonmodule::components::CameraComponent>();
    core::rendering::display::Display* display =
        camera_entity.HasComponent<components::DisplayComponent>()
            ? camera_entity.GetComponent<components::DisplayComponent>().GetDisplayPtr()
            : nullptr;
    if (main_camera != &camera_component ||
        (display && camera_entity.GetComponent<components::DisplayComponent>().disable_rendering)) {
      continue;
    }

    // Per default, we use camera parameters from the camera component. If there exists a stereo view component, we will
    // use its parameters instead
    NvAR_RenderCameraIntrinsicParams camera_intrinsics;
    camera_intrinsics.fy = main_camera->GetFocalLength();
    camera_intrinsics.fx = camera_intrinsics.fy;
    camera_intrinsics.cx = main_camera->GetPrincipalPoint().x;
    camera_intrinsics.cy = main_camera->GetPrincipalPoint().y;
    const std::vector<NvAR_RenderCameraIntrinsicParams> single_intrinsic_from_camera = {camera_intrinsics};
    const std::vector<glm::mat4> single_unit_view_matrix = {glm::mat4(1.0f)};
    std::vector<NvAR_RenderCameraIntrinsicParams> single_intrinsic_from_stereoview;
    std::vector<glm::mat4> single_view_matrix_from_stereoview;
    const std::vector<glm::mat4>* view_matrices = &single_unit_view_matrix;
    const std::vector<NvAR_RenderCameraIntrinsicParams>* intrinsics_vec = &single_intrinsic_from_camera;

    const glm::vec2 screen_size_pixels = glm::vec2(renderer->GetRenderWidth(), renderer->GetRenderHeight());
    static const float kMmPerInch = 25.4;                       // Millimeters per inch
    const float resolution_ppi = 110;                           // A typical resolution [pixels per inch]
    const float resolution_ppmm = resolution_ppi / kMmPerInch;  // [pixels per millimeter]
    const glm::vec2 screen_size_meters = (display ? glm::vec2(display->ScreenWidthMm(), display->ScreenHeightMm())
                                                  : screen_size_pixels / resolution_ppmm) *
                                         0.001f;
    if (camera_entity.HasComponent<components::StereoViewComponent>() && display) {
      auto& stereo_view = camera_entity.GetComponent<components::StereoViewComponent>();
      intrinsics_vec = &stereo_view.CameraIntrinsics();
      view_matrices = &stereo_view.ViewMatrices();
      if (display->IsActiveStereo()) {
        // Use view points from display stereo
        std::vector<glm::vec3> view_points;
        if (display->GetStereoViewPoints(&view_points)) {
          float screen_width_pixels = display->RenderWidth();
          float screen_height_pixels = display->RenderHeight();
          stereo_view.SetViews(view_points, {screen_width_pixels, screen_height_pixels}, screen_size_meters);
        }
      } else {
        stereo_view.UpdateViewsForEyes(screen_size_pixels, screen_size_meters, dt);
      }
      view_matrices = &stereo_view.ViewMatrices();
      intrinsics_vec = &stereo_view.CameraIntrinsics();
      if (renderer->NumViews() == 1) {
        // Update parameters used for rendering
        // Assume right eye dominant
        single_view_matrix_from_stereoview = {stereo_view.ViewMatrices()[1]};
        single_intrinsic_from_stereoview = {stereo_view.CameraIntrinsics()[1]};
        view_matrices = &single_view_matrix_from_stereoview;
        intrinsics_vec = &single_intrinsic_from_stereoview;
      }

      if (m_engine) {
        uint32_t entity_id = static_cast<uint32_t>(camera_entity);
        m_engine->SetTimeStampForEntity(entity_id, stereo_view.GetTimeStamp());
      }
    }

    renderer->BeginFrame(main_camera->GetViewMatrix(), *view_matrices, *intrinsics_vec);

    // Render cube maps into the composite FBO.
    for (core::ecs::Entity& entity : reg->view<components::CubeMapComponent>()) {
      auto& cube_map = entity.GetComponent<components::CubeMapComponent>();
      glm::mat4 transform_matrix = glm::mat4(1.0f);
      bool at_infinity = true;
      if (entity.HasComponent<commonmodule::components::TransformComponent>()) {
        auto& transform = entity.GetComponent<commonmodule::components::TransformComponent>();
        if (cube_map.is_infinite) {
          // Only rotation
          transform_matrix = commonmodule::GetGlobalTransform(entity.GetParent()) * transform.GetRotationMatrix();
        } else {
          // Full transform
          transform_matrix = commonmodule::GetGlobalTransform(entity);
          at_infinity = false;
        }
      }
      renderer->DrawCubeMapModel(cube_map.GetCubeMapModelPtr(), transform_matrix, at_infinity);
    }

    // Render textured quads into the composite FBO.
    for (core::ecs::Entity& entity : reg->view<components::TexturedQuadComponent>()) {
      auto& quad = entity.GetComponent<components::TexturedQuadComponent>();
      glm::mat4 transform_matrix = glm::mat4(1.0f);
      if (entity.HasComponent<commonmodule::components::TransformComponent>()) {
        transform_matrix = commonmodule::GetGlobalTransform(entity);
      }
      renderer->DrawUnlitMesh(quad.GetMeshModelPtr(), transform_matrix);
    }

    // Render triplane models into the foreground FBO.
    for (core::ecs::Entity& entity : reg->view<components::RenderableTriplaneComponent>()) {
      auto& renderable_triplane = entity.GetComponent<components::RenderableTriplaneComponent>();
      NvCVTriplaneVolume* triplane_volume = renderable_triplane.GetPtr();
      if (!triplane_volume) continue;
      if (entity.HasComponent<commonmodule::components::TransformComponent>()) {
        entity.GetComponent<commonmodule::components::TransformComponent>().Update(dt);
      }
      glm::mat4 transform_matrix = commonmodule::GetGlobalTransformFiltered(entity);
      renderer->DrawTriplaneModel(triplane_volume, transform_matrix);
    }

    // Add the shadows and foreground into the composite FBO.
    renderer->CompositeForegroundBackground();

    renderer->RenderVignette();

    renderer->EndFrame();
    renderer->BlitToDisplayBuffer(display, 0, 0, m_windowSizeX, m_windowSizeY);

    if (camera_entity.HasComponent<commonmodule::components::RecordingCallbackComponent>()) {
      auto& recording_cb_component = camera_entity.GetComponent<commonmodule::components::RecordingCallbackComponent>();
      // Only do the work if someone is watching.
      if (recording_cb_component.HasOnFiredCallback()) {
        CHECK_SUCCESS(
            RecordFrame(&recording_cb_component, renderer.get(), record_frames_gpu, &m_tmpRecordingBuffer, GetStream()),
            "Failed to capture rendered frame");
      }
    }
  }
bail:
  return err;
}

core::Error RenderSystem::OnEvent(core::ecs::registry::EntityRegistry* reg, core::events::Event* e) {
  switch (e->Type()) {
    case core::events::EventType::WINDOW_SIZE_EVENT: {
      auto& window_size_event = e->As<core::events::WindowSizeEvent>();
      m_windowSizeX = window_size_event.GetXsize();
      m_windowSizeY = window_size_event.GetYsize();
      break;
    }
    default:
      break;
  }
  return core::Error::SUCCESS;
}

}  // namespace systems
}  // namespace rendermodule
}  // namespace modules
}  // namespace nv3dvc
