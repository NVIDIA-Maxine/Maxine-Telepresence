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

#ifndef SRC_MODULES_TRACKINGMODULE_SYSTEMS_HEADTRACKINGSYSTEM_H_
#define SRC_MODULES_TRACKINGMODULE_SYSTEMS_HEADTRACKINGSYSTEM_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "CommonModule/OneEuroFilterProperties.h"
#include "Core/Engine/Engine.h"
#include "Core/EntityComponentSystem/Entity.h"
#include "Core/EntityComponentSystem/System.h"
#include "Core/Properties/Property.h"
#include "Core/Util/OneEuroFilter.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "nppdefs.h"
#include "nvAR.h"

namespace nv3dvc {

namespace core {
namespace ecs {
namespace registry {
class EntityRegistry;
}  // namespace registry
}  // namespace ecs
}  // namespace core

namespace modules {
namespace trackingmodule {
namespace systems {

/// @defgroup HeadTrackingSystemProperties HeadTrackingSystem
/// @ingroup  SystemProperties
/// @brief    System for performing head tracking
///
/// The system acts on entities with components components::TrackedHeadComponent and
/// commonmodule::components::VideoFrameComponent attached. During scene load, a state will be allocated for each
/// components::TrackedHeadComponent. During run, the video frame will be read, and fed to the head tracker. It will
/// estimate the head pose information and write it to the components::TrackedHeadComponent.
///
/// If the entity has a capturemodule::components::WebCameraComponent attached, its camera instrinsics will be used.
/// If the entity has a commonmodule::components::TransformComponent attached, it will be used to determine the camera's
/// extrinsics. If the entity's child has a commonmodule::components::VideoFrameComponent attached, the video frame of
/// the cropped head will be written to it. If the entity's parent has a rendermodule::components::StereoViewComponent
/// attached, the eye position based on the head tracking will be written to it.
///
/// Hence, a typical scene configuration for this system to act on consists of the following setup:
/// - Display entity containing
///   - rendermodule::components::StereoViewComponent
/// - Web camera entity (child of Display entity) containing
///   - components::TrackedHeadComponent
///   - capturemodule::components::WebCameraComponent : Defining camera intrinsics
///   - commonmodule::components::TransformComponent : Defining the extrinsic transform from camera to display
/// - Video out entity (child of Web camera entity) containing
///   - commonmodule::components::VideoFrameComponent

/// See @ref HeadTrackingSystemProperties
class HeadTrackingSystem : public core::ecs::System {
 public:
  constexpr static const char* NAME = "HeadTrackingSystem";
  std::string Name() const override { return NAME; };

  /// @brief Constructor
  /// @param[in,out] engine The engine. Used for queuing user prompts for head pose adjustment
  explicit HeadTrackingSystem(core::engine::Engine* engine);

  nv3dvc::core::Error OnLoadScene(core::ecs::registry::EntityRegistry* reg) override;
  nv3dvc::core::Error OnUnloadScene(core::ecs::registry::EntityRegistry* reg) override;
  nv3dvc::core::Error Run(core::ecs::registry::EntityRegistry* reg, float dt) override;

 public:
  /// @ingroup  HeadTrackingSystemProperties
  /// @{
  core::properties::Property<std::string> ar_sdk_model_dir = {
      this,
      "ar_sdk_model_dir",
      "Path to AR SDK model folder. The default value will be determined based on the environment variable ARSDK "
      "which should be set before running the engine. See README.md for details on setting up environment variables.",
      core::engine::Engine::GetArSdkDir() + "bin/models/",
  };
  core::properties::Property<glm::ivec2> resized_crop_image_size = {
      this,
      "resized_crop_image_size",
      "Size (in pixels) of the cropped image which will be written to the output video frame",
      {512, 512},
  };
  core::properties::Property<glm::vec2> threshold_head_confidence = {
      this,
      "threshold_head_confidence",
      "Defines a band of low and high cutoff values respectively for smooth stepping head tracking confidence",
      {0.40f, 0.50f},
  };
  core::properties::Property<glm::vec2> threshold_head_roll_confidence = {
      this,
      "threshold_head_roll_confidence",
      "Defines a band of low and high cutoff angles (in degrees) respectively for smooth stepping head roll confidence",
      {80.0f, 90.0f},
  };
  core::properties::Property<glm::vec2> threshold_eyes_confidence = {
      this,
      "threshold_eyes_confidence",
      "Defines a band of low and high cutoff values respectively for smooth stepping eye confidence",
      {0.15f, 0.25f},
  };
  core::properties::Property<std::atomic<bool>> limit_head_confidence = {
      this,
      "limit_head_confidence",
      "Whether to set head confidence to 0.0 if the head tracker is not confident on the main face.",
      true,
  };
  commonmodule::OneEuroFilterProperties face_box_position_filter_props = {
      this,  //
      "face_box_position_filter_props",
      0.05f,
      1.0f,
      0.05f,
  };
  commonmodule::OneEuroFilterProperties face_box_size_filter_props = {
      this,  //
      "face_box_size_filter_props",
      0.05f,
      1.0f,
      0.05f,
  };
  commonmodule::OneEuroFilterProperties crop_angle_filter_props = {
      this,  //
      "crop_angle_filter_props",
      0.05f,
      100.0f,
      0.05f,
  };
  /// @}

 private:
  /// @brief State of a single head tracker
  struct HeadTrackingState {
    NvAR_FeatureHandle pose_est = nullptr;  ///< The face expression estimation feature handle

    NvAR_BBoxes face_boxes = {};                                    ///< All face boxes
    std::vector<NvAR_Rect> face_box_data;                           ///< All face box data
    std::vector<float> face_box_confidences;                        ///< Confidences for all face boxes
    NvAR_Quaternion head_pose_rotation = {0.0f, 0.0f, 0.0f, 1.0f};  ///< From 6DOF pose tracker, in cam space
    NvAR_Point3f head_pose_translation = {0.0f, 0.0f, 0.0f};        ///< From 6DOF pose tracker, in cam space
    float capture_camera_intrinsics[3] = {0.0f, 0.0f, 0.0f};        ///< fy, cx, cy
    uint32_t num_landmarks = 0;                                     ///< number of landmarks per head
    std::vector<NvAR_Point2f> landmarks_2d;                         ///< All 2D landmark confidences for main head
    std::vector<NvAR_Point3f> landmarks_3d;                         ///< All 3D landmarks for main head
    std::vector<float> landmarks_confidence;                        ///< All landmark confidences for main head
    float avg_landmark_confidence = 0.0f;                           ///< Average for main head
    float tracked_head_confidence = 0.0f;                           ///< Confidence for main head
    float reload_timer_seconds = 0.0f;                              ///< Down ticking timer
    float reload_interval_seconds = 1.0f;                           ///< Check every second if reload is required
    core::util::filter::OneEuroFilter<glm::vec2> face_box_filter_pos;
    core::util::filter::OneEuroFilter<glm::vec2> face_box_filter_size;
    core::util::filter::OneEuroFilter<float> crop_angle_filter;
  };

  /// @brief Initialize the state by loading the required TRT models
  /// @param[in,out] state         The resulting HeadTrackingState
  /// @param[in] ar_sdk_model_path The path where AR SDK TRT models are stored
  /// @param[in] stream            The CUDA stream which the feature will be configured to run on
  /// @return    core::Error::SUCCESS If successful
  core::Error InitializeState(HeadTrackingState* state, const std::string& ar_sdk_model_path, cudaStream_t stream);

  /// @brief Unload the feature and destroy the feature handle
  /// @param[in,out] state The state object containing the feature handle to unload
  /// @return        core::Error::SUCCESS If successful
  core::Error UninitializeState(HeadTrackingState* state);

  /// @brief Will cause a reload of the feature every second, if not confident
  /// @param[in] state         The head tracking state
  /// @param[in] is_confident  Whether confident on the head tracking
  /// @param[in] dt            Delta time [seconds]
  /// @return                  true if feature was reloaded, otherwise false
  bool MaybeReload(HeadTrackingState* state, bool is_confident, float dt);

  /// @brief Check whether the bounding box of a head is out of bounds
  /// Prompts the user to adjust their pose if not confident
  /// @param[in] frame_width   The frame width of the image that was used when tracking
  /// @param[in] frame_height  The frame height of the image that was used when tracking
  /// @param[in] bbox          The latest bounding box to test
  /// @return                  Whether the bounding box is out of bounds
  bool CheckBboxOutOfBounds(const int frame_width, const int frame_height, const NvAR_Rect& bbox);

  /// @brief Check whether the bounding box of a head is too small
  /// Prompts the user to adjust their pose if not confident
  /// @param[in] frame_width   The frame width of the image that was used when tracking
  /// @param[in] frame_height  The frame height of the image that was used when tracking
  /// @param[in] bbox          The latest bounding box to test
  /// @return                  Whether the bounding box is too small
  bool CheckBboxSmall(const int frame_width, const int frame_height, const NvAR_Rect& bbox);

  /// @brief Check whether the tracking of a head is confident
  /// Prompts the user to adjust their pose if not confident
  /// @param[in] confidence  The latest confidence to test
  /// @return                Whether the confidence is too small
  bool CheckConfidenceLow(const float confidence);

  std::unordered_map<core::ecs::Entity, HeadTrackingState, core::ecs::EntityHash> m_trackersState;
  NppStreamContext m_nppStreamContext = {};
  core::engine::Engine* m_engine;
};

}  // namespace systems
}  // namespace trackingmodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_TRACKINGMODULE_SYSTEMS_HEADTRACKINGSYSTEM_H_
