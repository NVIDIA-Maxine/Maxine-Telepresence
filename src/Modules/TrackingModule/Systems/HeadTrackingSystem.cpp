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

#include "HeadTrackingSystem.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "Core/EntityComponentSystem/Entity.h"
#include "Core/Error.h"
#include "Core/Util/Logger.h"
#include "Modules/CommonModule/CameraCalibration.h"
#include "Modules/CommonModule/Components/TransformComponent.h"
#include "Modules/CommonModule/Components/VideoFrameComponent.h"
#include "Modules/CommonModule/components/CameraCalibrationComponent.h"
#include "Modules/RenderModule/Components/StereoViewComponent.h"
#include "Modules/TrackingModule/Components/TrackedHeadComponent.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/euler_angles.hpp"
#include "glm/gtx/matrix_transform_2d.hpp"
#include "glm/gtx/norm.hpp"
#include "npp.h"
#include "nppi_geometry_transforms.h"
#include "nvARFaceExpressions.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Macros                                                                                                           ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static constexpr float kMeterPerCentimeter = 0.01f;
static constexpr float kCentimeterPerMeter = 1.0f / kMeterPerCentimeter;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Static function declarations                                                                                     ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static glm::mat4 CropRotationCorrection(const NvAR_Rect& face_box, uint32_t image_width, uint32_t image_height,
                                        float focal_length);

static NvAR_Rect RescaleBbox(const NvAR_Rect& bbox, float scaling_factor);

/// @brief Apply a crop and a rotation to an image
/// @param[in]     frame          The input image, 3 of 4 channel, U8, on GPU
/// @param[in,out] cropped_frame  Pre-allocated output image, 3 of 4 channel, U8, on GPU
/// @param[in]     crop_region    The region of the source image to crop
/// @param[in]     angle          The rotation angle in radians, to apply to the image
/// @param[in]     pixel_offset   Offset to where on the cropped frame to place the image [pixels]
/// @param[in]     npp_stream_ctx The NPP stream context on which to run the crop resizing
/// @return        nv3dvc::core::Error::ERR_NOT_PERMITTED If input image was not GPU allocated
///                nv3dvc::core::Error::ERR_GENERAL       If the affine transform operation failed
///                nv3dvc::core::Error::SUCCESS           if successful
static nv3dvc::core::Error ApplyCropResizeRotation(const NvCVImage* frame, NvCVImage* cropped_frame,
                                                   const NvAR_Rect& crop_region, float angle, const glm::vec2& offset,
                                                   const NppStreamContext& npp_stream_ctx);

/// @brief Extract a bounding box from a subset of the provided landmarks that are considered stable against expressions
/// @param[in] landmarks Full set of 126 landmarks
/// @return    The bounding box
static NvAR_Rect BboxFromLandmarks(const std::vector<NvAR_Point2f>& landmarks);

/// @brief Transform a 3D landmark from centimeters in camera space coordinates, to meters in display space coordinates.
/// @param[in] camera_to_display_transform The 4x4 transform from camera space coordinates to display space coordinates
/// @param[in] landmark                    The landmark to transform
/// @return    The landmark's coordinates in display space
static glm::vec3 LandmarkToDisplay(const glm::mat4& camera_to_display_transform, const NvAR_Point3f& landmark);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Static function definitions                                                                                      ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static glm::mat4 CropRotationCorrection(const NvAR_Rect& face_box, const uint32_t image_width,
                                        const uint32_t image_height, const float focal_length) {
  const float crop_center_u = face_box.x + (face_box.width - 1) * 0.5f - static_cast<float>(image_width - 1) * 0.5f;
  const float crop_center_v = face_box.y + (face_box.height - 1) * 0.5f - static_cast<float>(image_height - 1) * 0.5f;
  const glm::dvec3 crop_center_u_v3 = glm::normalize(glm::vec3(crop_center_u, 0.0f, focal_length));
  const glm::dvec3 crop_center_v_v3 = glm::normalize(glm::vec3(0.0f, crop_center_v, focal_length));
  constexpr glm::dvec3 cam_center_v3 = {0.0f, 0.0f, 1.0f};  // Normalized focal length
  // In radians
  const float y_angle = std::atan2<double>(glm::l2Norm(glm::cross(crop_center_u_v3, cam_center_v3)),
                                           glm::dot(crop_center_u_v3, cam_center_v3));
  const float x_angle = std::atan2<double>(glm::l2Norm(glm::cross(crop_center_v_v3, cam_center_v3)),
                                           glm::dot(crop_center_v_v3, cam_center_v3));
  const glm::mat4 rot_y = glm::eulerAngleY(crop_center_u > 0 ? -y_angle : y_angle);  // in gl space
  const glm::mat4 rot_x = glm::eulerAngleX(crop_center_v > 0 ? -x_angle : x_angle);  // in gl space
  const glm::mat4 crop_rot_correction = rot_x * rot_y;
  return crop_rot_correction;
}

static NvAR_Rect RescaleBbox(const NvAR_Rect& bbox, const float scaling_factor) {
  NvAR_Rect bbox_scaled = bbox;

  // Scale bounding box
  bbox_scaled.x -= (bbox_scaled.width * scaling_factor - bbox_scaled.width) * 0.5f;
  bbox_scaled.y -= (bbox_scaled.height * scaling_factor - bbox_scaled.height) * 0.5f;
  bbox_scaled.width = bbox_scaled.width * scaling_factor;
  bbox_scaled.height = bbox_scaled.height * scaling_factor;

  return bbox_scaled;
}

static nv3dvc::core::Error ApplyCropResizeRotation(const NvCVImage* frame, NvCVImage* cropped_frame,
                                                   const NvAR_Rect& crop_region, float angle, const glm::vec2& offset,
                                                   const NppStreamContext& npp_stream_ctx) {
  nv3dvc::core::Error err = nv3dvc::core::Error::SUCCESS;
  if (frame->gpuMem != NVCV_CUDA) {
    LOG_DEBUG("Frame for cropping not provided as CUDA pitch 2d device ptr");
    BAIL(err, nv3dvc::core::Error::ERR_NOT_PERMITTED);
  }

  const float scale_x = static_cast<float>(cropped_frame->width) / crop_region.width;
  const float scale_y = static_cast<float>(cropped_frame->height) / crop_region.height;

  // 2D matrix opertaions for affine transform
  // Translate center of source to origin
  const glm::mat3 trans_src = glm::translate(
      glm::mat3(1.0),
      -glm::vec2(crop_region.x + crop_region.width / 2.0 + 0.5, crop_region.y + crop_region.height / 2.0 + 0.5));
  // Scale and rotate frame
  const glm::mat3 scale = glm::scale(glm::mat3(1.0), glm::vec2(scale_x, scale_y));
  const glm::mat3 rot = glm::rotate(glm::mat3(1.0), angle);
  // Translate frame back to dst
  const glm::mat3 trans_dst = glm::translate(glm::mat3(1.0), glm::vec2(offset.x + cropped_frame->width / 2.0 - 0.5,
                                                                       offset.y + cropped_frame->height / 2.0 - 0.5));
  const glm::mat3 affine = trans_dst * rot * scale * trans_src;

  double affine_coeffs[2][3] = {
      {affine[0][0], affine[1][0], affine[2][0]},
      {affine[0][1], affine[1][1], affine[2][1]},
  };

  const Npp8u* const p_src = static_cast<Npp8u*>(frame->pixels);
  const NppiSize o_src_size = {static_cast<int>(frame->width), static_cast<int>(frame->height)};
  const int n_src_step = frame->pitch;
  const NppiRect o_src_rect_roi = {0, 0, o_src_size.width, o_src_size.height};
  Npp8u* const p_dst = static_cast<Npp8u*>(cropped_frame->pixels);
  const NppiSize o_dst_size = {static_cast<int>(cropped_frame->width), static_cast<int>(cropped_frame->height)};
  const int n_dst_step = cropped_frame->pitch;
  const NppiRect o_dst_rect_roi = {0, 0, o_dst_size.width, o_dst_size.height};

  NppStatus npp_err = NPP_SUCCESS;
  if (frame->numComponents == 4) {
    const Npp8u a_value[4] = {0, 0, 0, 0};
    npp_err = nppiSet_8u_C4R_Ctx(a_value, p_dst, n_dst_step, o_dst_size, npp_stream_ctx);
    CHECK_TRUE(npp_err == NPP_SUCCESS, nv3dvc::core::Error::ERR_GENERAL, "nppiSet_8u_C4R_Ctx failed with code %d",
               npp_err);
    npp_err = nppiWarpAffine_8u_C4R_Ctx(p_src, o_src_size, n_src_step, o_src_rect_roi, p_dst, n_dst_step,
                                        o_dst_rect_roi, affine_coeffs, NPPI_INTER_LINEAR, npp_stream_ctx);
    CHECK_TRUE(npp_err == NPP_SUCCESS, nv3dvc::core::Error::ERR_GENERAL,
               "nppiWarpAffine_8u_C4R_Ctx failed with code %d", npp_err);
  } else if (frame->numComponents == 3) {
    const Npp8u a_value[3] = {0, 0, 0};
    npp_err = nppiSet_8u_C3R_Ctx(a_value, p_dst, n_dst_step, o_dst_size, npp_stream_ctx);
    CHECK_TRUE(npp_err == NPP_SUCCESS, nv3dvc::core::Error::ERR_GENERAL, "nppiSet_8u_C3R_Ctx failed with code %d",
               npp_err);
    npp_err = nppiWarpAffine_8u_C3R_Ctx(p_src, o_src_size, n_src_step, o_src_rect_roi, p_dst, n_dst_step,
                                        o_dst_rect_roi, affine_coeffs, NPPI_INTER_LINEAR, npp_stream_ctx);
    CHECK_TRUE(npp_err == NPP_SUCCESS, nv3dvc::core::Error::ERR_GENERAL,
               "nppiWarpAffine_8u_C3R_Ctx failed with code %d", npp_err);
  }
bail:
  return err;
}

static NvAR_Rect BboxFromLandmarks(const std::vector<NvAR_Point2f>& landmarks) {
  // Subset of landmarks minimally affected by expressions
  static const std::vector<uint32_t> stable_landmark_idx = {0,  1,  2,  3,  4,  5,  27, 28, 29, 30, 31, 32, 51, 52, 53,
                                                            54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 68, 81, 85};
  glm::vec2 lmk_min{std::numeric_limits<float>::max()};
  glm::vec2 lmk_max{std::numeric_limits<float>::lowest()};
  for (uint32_t idx : stable_landmark_idx) {
    const glm::vec2 landmark{landmarks[idx].x, landmarks[idx].y};
    lmk_min = glm::min(lmk_min, landmark);
    lmk_max = glm::max(lmk_max, landmark);
  }
  const glm::vec2 size = lmk_max - lmk_min;
  const glm::vec2 center = lmk_min + size / 2.0f;
  const float max_size = std::max(size.x, size.y);
  return {center.x - max_size / 2.0f, center.y - max_size / 2.0f, max_size, max_size};
}

static glm::vec3 LandmarkToDisplay(const glm::mat4& camera_to_display_transform, const NvAR_Point3f& landmark) {
  const glm::vec4 pt_camera_space_h{glm::vec3(landmark.x, landmark.y, landmark.z) * kMeterPerCentimeter, 1.0f};
  return glm::vec3(camera_to_display_transform * pt_camera_space_h);
}

namespace nv3dvc {
namespace modules {
namespace trackingmodule {
namespace systems {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Member function definitions                                                                                      ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HeadTrackingSystem::HeadTrackingSystem(core::engine::Engine* engine) : m_engine(engine) {
  auto reset_filter_fn = [this]() {
    for (auto& [entity, state] : m_trackersState) {
      state.face_box_filter_pos.Reset(face_box_position_filter_props.min_cutoff_freq,
                                      face_box_position_filter_props.cutoff_slope,
                                      face_box_position_filter_props.deriv_cutoff_freq);
      state.face_box_filter_size.Reset(face_box_size_filter_props.min_cutoff_freq,
                                       face_box_size_filter_props.cutoff_slope,
                                       face_box_size_filter_props.deriv_cutoff_freq);
      state.crop_angle_filter.Reset(crop_angle_filter_props.min_cutoff_freq, crop_angle_filter_props.cutoff_slope,
                                    crop_angle_filter_props.deriv_cutoff_freq);
    }
  };
  face_box_position_filter_props.min_cutoff_freq.SetOnChangeFunction(reset_filter_fn);
  face_box_position_filter_props.cutoff_slope.SetOnChangeFunction(reset_filter_fn);
  face_box_position_filter_props.deriv_cutoff_freq.SetOnChangeFunction(reset_filter_fn);

  face_box_size_filter_props.min_cutoff_freq.SetOnChangeFunction(reset_filter_fn);
  face_box_size_filter_props.cutoff_slope.SetOnChangeFunction(reset_filter_fn);
  face_box_size_filter_props.deriv_cutoff_freq.SetOnChangeFunction(reset_filter_fn);

  crop_angle_filter_props.min_cutoff_freq.SetOnChangeFunction(reset_filter_fn);
  crop_angle_filter_props.cutoff_slope.SetOnChangeFunction(reset_filter_fn);
  crop_angle_filter_props.deriv_cutoff_freq.SetOnChangeFunction(reset_filter_fn);
}

nv3dvc::core::Error HeadTrackingSystem::InitializeState(HeadTrackingState* state, const std::string& ar_sdk_model_path,
                                                        cudaStream_t stream) {
  core::Error err = core::Error::SUCCESS;
  if (!m_engine) {
    LOG_WARNING("Engine not provided to HeadTrackingSystem. No user prompts to set.");
  }
  state->avg_landmark_confidence = 0.0f;

  // Configure and allocate face box data
  state->face_boxes.max_boxes = 1;
  state->face_box_confidences.resize(state->face_boxes.max_boxes);
  state->face_box_data.resize(state->face_boxes.max_boxes);
  state->face_boxes.boxes = state->face_box_data.data();

  // Configure
  CHECK_NVCV_SUCCESS(NvAR_Create(NvAR_Feature_FaceExpressions, &state->pose_est));
  CHECK_NVCV_SUCCESS(NvAR_SetString(state->pose_est, NvAR_Parameter_Config(ModelDir), ar_sdk_model_path.c_str()));
  CHECK_NVCV_SUCCESS(NvAR_SetU32(state->pose_est, NvAR_Parameter_Config(Temporal), 0));
  CHECK_NVCV_SUCCESS(NvAR_SetU32(state->pose_est, NvAR_Parameter_Config(PoseMode), 2));  // 6DOF head pose tracking
  CHECK_NVCV_SUCCESS(NvAR_SetCudaStream(state->pose_est, NvAR_Parameter_Config(CUDAStream), stream));

  // Load
  CHECK_NVCV_SUCCESS(NvAR_Load(state->pose_est));

  // inputs/outputs (We wait with setting input image since its size is not known yet)
  CHECK_NVCV_SUCCESS(NvAR_SetObject(state->pose_est, NvAR_Parameter_Output(Pose), &state->head_pose_rotation,
                                    sizeof(NvAR_Quaternion)));
  CHECK_NVCV_SUCCESS(NvAR_SetObject(state->pose_est, NvAR_Parameter_Output(PoseTranslation),
                                    &state->head_pose_translation, sizeof(NvAR_Point3f)));
  CHECK_NVCV_SUCCESS(
      NvAR_SetObject(state->pose_est, NvAR_Parameter_Output(BoundingBoxes), &state->face_boxes, sizeof(NvAR_BBoxes)));
  CHECK_NVCV_SUCCESS(NvAR_SetF32Array(state->pose_est, NvAR_Parameter_Output(BoundingBoxesConfidence),
                                      state->face_box_confidences.data(),
                                      static_cast<int>(state->face_box_confidences.size())));
  CHECK_NVCV_SUCCESS(NvAR_GetU32(state->pose_est, NvAR_Parameter_Config(Landmarks_Size), &state->num_landmarks));
  state->landmarks_2d.resize(state->num_landmarks);
  state->landmarks_3d.resize(state->num_landmarks);
  state->landmarks_confidence.resize(state->num_landmarks);
  CHECK_NVCV_SUCCESS(NvAR_SetObject(state->pose_est, NvAR_Parameter_Output(Landmarks), state->landmarks_2d.data(),
                                    sizeof(NvAR_Point2f)));
  CHECK_NVCV_SUCCESS(NvAR_SetObject(state->pose_est, NvAR_Parameter_Output(Landmarks3d), state->landmarks_3d.data(),
                                    sizeof(NvAR_Point3f)));
  CHECK_NVCV_SUCCESS(NvAR_SetF32Array(state->pose_est, NvAR_Parameter_Output(LandmarksConfidence),
                                      state->landmarks_confidence.data(), sizeof(float)));
bail:
  if (core::SUCCESS != err)
    LOG_DEBUG("Failure to initialize Face Expression Pose Estimation with model dir \"%s\"", ar_sdk_model_path.c_str());
  return err;
}

bool HeadTrackingSystem::CheckBboxOutOfBounds(const int frame_width, const int frame_height, const NvAR_Rect& bbox) {
  const float center_x_scaled =
      static_cast<float>(bbox.x + bbox.width / 2 - frame_width / 2) / static_cast<float>(frame_width / 2);
  const float center_y_scaled =
      static_cast<float>(bbox.y + bbox.height / 2 - frame_height / 2) / static_cast<float>(frame_height / 2);
  const float horizontal_tolerance = 0.7f;
  const float vertical_tolerance = 0.7f;
  const bool is_bbox_outside_bounds =
      std::abs(center_x_scaled) > horizontal_tolerance || std::abs(center_y_scaled) > vertical_tolerance;
  if (is_bbox_outside_bounds && m_engine)
    m_engine->Control().Queue<core::engine::command::PromptUser>("Please center face in view");
  return is_bbox_outside_bounds;
}

bool HeadTrackingSystem::CheckBboxSmall(const int frame_width, const int frame_height, const NvAR_Rect& bbox) {
  const float width_scaled = static_cast<float>(bbox.width) / static_cast<float>(frame_width);
  const float height_scaled = static_cast<float>(bbox.height) / static_cast<float>(frame_height);
  const float width_tolerance = 0.08f;
  const float height_tolerance = 0.08f;
  const bool is_bbox_small = width_scaled < width_tolerance || height_scaled < height_tolerance;
  if (is_bbox_small && m_engine)
    m_engine->Control().Queue<core::engine::command::PromptUser>("Please sit closer to the camera");
  return is_bbox_small;
}

bool HeadTrackingSystem::CheckConfidenceLow(const float confidence) {
  const float confidence_tolerance = 0.2f;
  const bool is_confidence_low = confidence < confidence_tolerance;
  if (is_confidence_low && m_engine)
    m_engine->Control().Queue<core::engine::command::PromptUser>("Low confidence in face tracking");
  return is_confidence_low;
}

nv3dvc::core::Error HeadTrackingSystem::UninitializeState(HeadTrackingState* state) {
  core::Error err = core::Error::SUCCESS;
  if (state->pose_est) {
    CHECK_NVCV_SUCCESS(NvAR_Destroy(state->pose_est));
  }
bail:
  return err;
}

bool HeadTrackingSystem::MaybeReload(HeadTrackingState* state, const bool is_confident, const float dt) {
  core::Error err = core::Error::SUCCESS;
  bool did_reload = false;
  if (is_confident) {
    state->reload_timer_seconds = 0.0f;
  } else {
    state->reload_timer_seconds += dt;
    if (state->reload_timer_seconds >= state->reload_interval_seconds) {
      state->reload_timer_seconds = 0.0f;
      LOG_INFO("Reloading face expression estimation feature to reset bounding box");
      CHECK_SUCCESS(UninitializeState(state));
      CHECK_SUCCESS(InitializeState(state, ar_sdk_model_dir, GetStream()));
      did_reload = true;
    }
  }
bail:
  return did_reload;
}

nv3dvc::core::Error HeadTrackingSystem::OnLoadScene(core::ecs::registry::EntityRegistry* reg) {
  core::Error err = core::Error::SUCCESS;
  for (core::ecs::Entity& entity : reg->view<components::TrackedHeadComponent>()) {
    // Create state for this entity.
    LOG_DEBUG("Creating head tracker for entity \"%s\"",
              entity.HasComponent<std::string>() ? entity.GetComponent<std::string>().c_str() : "Unknown");
    CHECK_TRUE(m_trackersState.count(entity) == 0, core::Error::ERR_INITIALIZATION);
    HeadTrackingState& state = m_trackersState[entity];
    CHECK_SUCCESS(InitializeState(&state, ar_sdk_model_dir, GetStream()), "Entity: \"%s\"",
                  entity.HasComponent<std::string>() ? entity.GetComponent<std::string>().c_str() : "Unknown");
  }

  // Create an execution context for npp
  int dev_id;
  int shared_mem_per_block;
  CHECK_CUDA_SUCCESS(cudaGetDevice(&dev_id));
  m_nppStreamContext.nCudaDeviceId = dev_id;
  CHECK_CUDA_SUCCESS(cudaDeviceGetAttribute(&m_nppStreamContext.nCudaDevAttrComputeCapabilityMajor,
                                            cudaDevAttrComputeCapabilityMajor, dev_id));
  CHECK_CUDA_SUCCESS(cudaDeviceGetAttribute(&m_nppStreamContext.nCudaDevAttrComputeCapabilityMinor,
                                            cudaDevAttrComputeCapabilityMinor, dev_id));
  CHECK_CUDA_SUCCESS(
      cudaDeviceGetAttribute(&m_nppStreamContext.nMultiProcessorCount, cudaDevAttrMultiProcessorCount, dev_id));
  CHECK_CUDA_SUCCESS(cudaDeviceGetAttribute(&m_nppStreamContext.nMaxThreadsPerMultiProcessor,
                                            cudaDevAttrMaxThreadsPerMultiProcessor, dev_id));
  CHECK_CUDA_SUCCESS(
      cudaDeviceGetAttribute(&m_nppStreamContext.nMaxThreadsPerBlock, cudaDevAttrMaxThreadsPerBlock, dev_id));
  CHECK_CUDA_SUCCESS(cudaDeviceGetAttribute(&shared_mem_per_block, cudaDevAttrMaxSharedMemoryPerBlock, dev_id));

  m_nppStreamContext.nSharedMemPerBlock = shared_mem_per_block;
  m_nppStreamContext.hStream = GetStream();
  CHECK_CUDA_SUCCESS(cudaStreamGetFlags(m_nppStreamContext.hStream, &m_nppStreamContext.nStreamFlags));

  // Trigger filter params reset.
  face_box_position_filter_props.min_cutoff_freq.OnChange();
  face_box_size_filter_props.min_cutoff_freq.OnChange();
  crop_angle_filter_props.min_cutoff_freq.OnChange();
bail:
  return err;
}

nv3dvc::core::Error HeadTrackingSystem::OnUnloadScene(core::ecs::registry::EntityRegistry* reg) {
  core::Error err = core::Error::SUCCESS;
  for (auto& [entity, state] : m_trackersState) {
    const core::Error e = UninitializeState(&state);
    if (err == core::Error::SUCCESS) err = e;  // Save first error
  }
bail:
  m_trackersState.clear();
  return err;
}

nv3dvc::core::Error HeadTrackingSystem::Run(core::ecs::registry::EntityRegistry* reg, float dt) {
  auto view_tracked_heads =
      reg->view<commonmodule::components::VideoFrameComponent, components::TrackedHeadComponent>();
  core::Error err = core::Error::SUCCESS;

  for (auto& entity : view_tracked_heads) {
    auto parent = entity.GetParent();
    auto& captured_frame_component = entity.GetComponent<commonmodule::components::VideoFrameComponent>();
    auto* nvcv_image = captured_frame_component.GetImagePtr();
    auto& tracked_head = entity.GetComponent<components::TrackedHeadComponent>();

    if (!nvcv_image->pixels) {
      LOG_DEBUG("Skipping empty image in entity \"%s\"",
                entity.HasComponent<std::string>() ? entity.GetComponent<std::string>().c_str() : "Unknown");
      continue;
    }

    // Check we have tracker state for the entity.
    CHECK_TRUE(m_trackersState.count(entity) > 0, core::Error::ERR_ITEM_NOT_FOUND, "Entity \"%s\"",
               entity.HasComponent<std::string>() ? entity.GetComponent<std::string>().c_str() : "Unknown");
    auto& state = m_trackersState.at(entity);

    glm::mat4 camera_to_display_transform = glm::mat4(1.0f);

    if (entity.HasComponent<commonmodule::components::TransformComponent>()) {
      // The entity is expected to be the child of an entity representing the display
      // (think webcam attached to the upper portion of the display). If this is the case, we compute the relative
      // transform between the two so that the tracked eye locations can be transferred from the webcam's 3D space
      // to the display's 3D space
      auto& transform = entity.GetComponent<commonmodule::components::TransformComponent>();
      camera_to_display_transform = transform.GetLocalMatrix();
    } else {
      // Assume camera points along the display normal (rotated 180 degrees), and centered at the display origin
      camera_to_display_transform = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    }
    if (entity.HasComponent<commonmodule::components::CameraCalibrationComponent>()) {
      auto& camera_calibration_component = entity.GetComponent<commonmodule::components::CameraCalibrationComponent>();
      const commonmodule::CameraCalibration& camera_calibration = camera_calibration_component.GetCameraCalibration();
      state.capture_camera_intrinsics[0] = camera_calibration.fy;
      state.capture_camera_intrinsics[1] = camera_calibration.cx;
      state.capture_camera_intrinsics[2] = camera_calibration.cy;
    } else {
      // Make a guess
      state.capture_camera_intrinsics[0] = static_cast<float>(nvcv_image->height);             // vertical focal length
      state.capture_camera_intrinsics[1] = static_cast<float>(nvcv_image->width - 1) * 0.5f;   // principal point x
      state.capture_camera_intrinsics[2] = static_cast<float>(nvcv_image->height - 1) * 0.5f;  // principal point y
    }

    CHECK_NVCV_SUCCESS(NvAR_SetF32Array(state.pose_est, NvAR_Parameter_Input(CameraIntrinsicParams),
                                        state.capture_camera_intrinsics, 3));
    CHECK_NVCV_SUCCESS(NvAR_SetObject(state.pose_est, NvAR_Parameter_Input(Image), nvcv_image, sizeof(NvCVImage)));
    NvCV_Status nvcv_err = NvAR_Run(state.pose_est);
    if (nvcv_err != NVCV_SUCCESS) {
      MaybeReload(&state, false, dt);
    }

    // Get the pose estimated by AR SDK.
    glm::quat ar_pose_rotation{state.head_pose_rotation.w, state.head_pose_rotation.x, state.head_pose_rotation.y,
                               state.head_pose_rotation.z};
    glm::vec3 ar_pose_translation{state.head_pose_translation.x * kMeterPerCentimeter,
                                  state.head_pose_translation.y * kMeterPerCentimeter,
                                  state.head_pose_translation.z * kMeterPerCentimeter};
    if (glm::any(glm::isnan(ar_pose_rotation))) {
      ar_pose_rotation = glm::identity<glm::quat>();
    }

    glm::vec3 head_pose_viewpoint{0.0f};

    // Compute average landmark confidence
    state.avg_landmark_confidence = 0.0f;
    for (float conf : state.landmarks_confidence) {
      state.avg_landmark_confidence += conf;
    }
    state.avg_landmark_confidence /= state.landmarks_confidence.size();
    // low threshold just to determine if any landmarks were found at all
    static constexpr float kLandmarkConfidenceThreshold = 0.1f;

    bool is_confident_on_main_face = false;
    bool is_confident_on_eyes = false;
    if (state.face_boxes.num_boxes == 0) {
      tracked_head.tracking_confidence = 0.0f;
      tracked_head.has_facebox = false;
    } else {
      // The AR SDK estimates the 3D rotation of the head with respect to the cropped image region around the detected
      // face. The face region is treated as if it were orthogonal to the ray that passes from the camera center through
      // the center of the face. In order to transfer the estimated head rotation into the camera's local coordinate
      // frame, we must apply an additional rotation, depending on the angle of that ray.
      // For example if the face is detected at an (azimuth, elevation) of (30°, 15°) away from the optical axis, we
      // must apply a correction of 30° to the head pose azimuth, and 15° to the head pose elevation.
      const glm::vec3 raw_pose_euler = glm::eulerAngles(ar_pose_rotation);

      const NvAR_Rect landmark_bbox_raw = BboxFromLandmarks(state.landmarks_2d);
      const glm::vec2 bbox_pos_vals = state.face_box_filter_pos.Filter({landmark_bbox_raw.x, landmark_bbox_raw.y}, dt);
      const glm::vec2 bbox_size_vals =
          state.face_box_filter_size.Filter({landmark_bbox_raw.width, landmark_bbox_raw.height}, dt);
      const NvAR_Rect landmark_bbox = {bbox_pos_vals.x, bbox_pos_vals.y, bbox_size_vals.x, bbox_size_vals.y};

      // Modify bbox based on padding, offset, and scaling
      NvAR_Rect crop_bbox = RescaleBbox(landmark_bbox, tracked_head.GetFaceCropScale());
      // Compute 3D position of crop box by projecting a ray through its center to a depth of `raw_pose_depth`.
      const float focal_length = state.capture_camera_intrinsics[0];
      const glm::mat4 crop_box_rotation_correction =
          CropRotationCorrection(crop_bbox, nvcv_image->width, nvcv_image->height, focal_length);
      const glm::mat4 crop_box_to_cam =
          glm::translate(glm::mat4(1.0f), ar_pose_translation) * crop_box_rotation_correction;
      const glm::mat4 crop_box_to_display = camera_to_display_transform * crop_box_to_cam;

      // Head-based view point is 4cm above the center of the face box.
      head_pose_viewpoint = glm::vec3(crop_box_to_display * glm::vec4(0.0f, 0.04f, 0.0f, 1.0f));

      float head_confidence = glm::smoothstep(threshold_head_confidence.get()->x, threshold_head_confidence.get()->y,
                                              state.avg_landmark_confidence);
      head_confidence *=
          1.0f - glm::smoothstep(threshold_head_roll_confidence.get()->x, threshold_head_roll_confidence.get()->y,
                                 glm::abs(glm::degrees(raw_pose_euler.z)));
      const float eyes_confidence = glm::smoothstep(threshold_eyes_confidence.get()->x,
                                                    threshold_eyes_confidence.get()->y, state.avg_landmark_confidence);
      const bool is_bbox_oob = CheckBboxOutOfBounds(nvcv_image->width, nvcv_image->height, landmark_bbox);
      const bool is_bbox_small = CheckBboxSmall(nvcv_image->width, nvcv_image->height, landmark_bbox);
      is_confident_on_main_face = !(CheckConfidenceLow(head_confidence) || is_bbox_oob || is_bbox_small);
      is_confident_on_eyes = !(CheckConfidenceLow(eyes_confidence) || is_bbox_oob || is_bbox_small);
      if (MaybeReload(&state, is_confident_on_main_face && is_confident_on_eyes, dt)) {
        is_confident_on_eyes = false;
      }
      if (limit_head_confidence.get()->load() && !is_confident_on_main_face) head_confidence = 0.0f;

      bool apply_roll_rotation = tracked_head.apply_roll_correction.get()->load();

      tracked_head.has_facebox = true;
      tracked_head.raw_pose_euler = raw_pose_euler;
      tracked_head.face_box_to_display_rotation = glm::quat(crop_box_to_display);
      // Clamp tracked head translation to 3D bounding box
      tracked_head.face_box_to_display_translation = glm::clamp(glm::vec3(crop_box_to_display[3]) * kCentimeterPerMeter,
                                                                tracked_head.GetBboxMin(), tracked_head.GetBboxMax());
      tracked_head.tracking_confidence = head_confidence;

      float rotation_angle = state.crop_angle_filter.Filter(raw_pose_euler.z, dt);

      auto children = entity.GetChildren();
      for (auto& child : children) {
        if (child.HasComponent<commonmodule::components::VideoFrameComponent>()) {
          auto& cropped_head_frame_component = child.GetComponent<commonmodule::components::VideoFrameComponent>();
          NvCVImage* cropped_head_image = cropped_head_frame_component.GetImagePtr();

          if (cropped_head_image->pixels == nullptr) {
            CHECK_SUCCESS(cropped_head_frame_component.AllocateImageBuffer(
                resized_crop_image_size.get()->x, resized_crop_image_size.get()->y, NvCVImage_PixelFormat::NVCV_BGR,
                NvCVImage_ComponentType::NVCV_U8, NVCV_CHUNKY, NVCV_CUDA, 1));
          }
          if (!tracked_head.apply_crop.get()->load()) {
            crop_bbox.x = 0;
            crop_bbox.y = 0;
            crop_bbox.width = nvcv_image->width;
            crop_bbox.height = nvcv_image->height;
          }
          const float rotation_angle = tracked_head.apply_roll_correction.get()->load() ? raw_pose_euler.z : 0.0f;
          CHECK_SUCCESS(ApplyCropResizeRotation(nvcv_image, cropped_head_image, crop_bbox, rotation_angle,
                                                tracked_head.GetFaceCropOffset(), m_nppStreamContext));
          cropped_head_frame_component.SetTimeStamp(captured_frame_component.GetTimeStamp());
        }
      }
    }

    // If we have a StereoViewComponent, set the tracked eye locations based on tracked eye landmarks
    if (parent.HasComponent<rendermodule::components::StereoViewComponent>()) {
      auto& stereo_view = parent.GetComponent<rendermodule::components::StereoViewComponent>();

      if (state.face_boxes.num_boxes != 0 && state.avg_landmark_confidence > kLandmarkConfidenceThreshold) {
        // Transform eye locations from camera space to screen centric. From centimeters to meters
        static constexpr int kLeftIrisLandmarkIdx = 97;
        static constexpr int kRightIrisLandmarkIdx = 80;
        glm::vec3 eye_left = LandmarkToDisplay(camera_to_display_transform, state.landmarks_3d[kLeftIrisLandmarkIdx]);
        glm::vec3 eye_right = LandmarkToDisplay(camera_to_display_transform, state.landmarks_3d[kRightIrisLandmarkIdx]);

        if (stereo_view.viewpoint_from_head_pose) {
          // Replace actual eye positions with head position.
          eye_left = head_pose_viewpoint;
          eye_right = head_pose_viewpoint;
        }

        if (captured_frame_component.IsMirrored()) {
          eye_left.x *= -1.0f;
          eye_right.x *= -1.0f;
        }

        stereo_view.SetEyes(eye_left, eye_right, dt);
        stereo_view.SetTimeStamp(captured_frame_component.GetTimeStamp());
      }

      if (limit_head_confidence.get()->load()) {
        stereo_view.SetIsConfident(is_confident_on_eyes);
      }
    }
  }

bail:
  return err;
}

}  // namespace systems
}  // namespace trackingmodule
}  // namespace modules
}  // namespace nv3dvc
