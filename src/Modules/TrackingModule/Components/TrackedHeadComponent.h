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

#ifndef SRC_MODULES_TRACKINGMODULE_COMPONENTS_TRACKEDHEADCOMPONENT_H_
#define SRC_MODULES_TRACKINGMODULE_COMPONENTS_TRACKEDHEADCOMPONENT_H_

#include <mutex>  // NOLINT(build/c++11) (cpplint outside Chromium: https://github.com/google/styleguide/issues/483)
#include <string>

#include "Core/EntityComponentSystem/Component.h"
#include "Core/Util/Types.h"
#include "glm/detail/type_quat.hpp"
#include "glm/glm.hpp"

namespace nv3dvc {
namespace modules {
namespace trackingmodule {
namespace components {

/// @defgroup TrackedHeadComponentProperties TrackedHeadComponent
/// @ingroup  ComponentProperties
/// @brief    Component containing tracked head information
///
/// Acted on by the systems::HeadTrackingSystem system

/// See @ref TrackedHeadComponentProperties
class TrackedHeadComponent : public core::ecs::Component {
 public:
  constexpr static const char* NAME = "TrackedHeadComponent";
  std::string Name() const override { return NAME; }

  TrackedHeadComponent() = default;

  /// @brief Get the minimum coordinate of the 3D bounding box
  /// @return The minimum coordinate of the 3D bounding box
  glm::vec3 GetBboxMin() const;

  /// @brief Get the maximum coordinate of the 3D bounding box
  /// @return The maximum coordinate of the 3D bounding box
  glm::vec3 GetBboxMax() const;

  /// @brief Get the face crop scale
  /// @return The face crop scale
  float GetFaceCropScale() const;

  /// @brief Get the face crop offset
  /// @return The face crop offset [pixels]
  glm::vec2 GetFaceCropOffset() const;

  /// @brief Get the tracked translation with added offset
  /// This function is mutex guarded, and hence thread safe
  /// @return The tracked translation with added offset
  glm::vec3 TrackedTranslationWithOffset();

  /// @brief Set the translation offset
  /// The translation offset will be applied when getting the translation using the function
  /// TrackedTranslationWithOffset. When getting the raw translation using tracked_translation, the offset is ignored
  /// This function is mutex guarded, and hence thread safe
  /// @param[in] offset The translation offset
  void SetTranslationOffset(glm::vec3 offset);

 public:
  /// @ingroup TrackedHeadComponentProperties
  /// @{
  core::properties::Property<glm::vec3> translation_offset{
      this,
      "translation_offset",
      "Offset added to translation in centimeter units",
      glm::vec3(0.0f, 0.0f, 0.0f),
  };
  core::properties::Property<glm::vec3> tracked_bbox_min_cm{
      this,
      "tracked_bbox_min_cm",
      "Tracked bounding box min in centimeter units",
      glm::vec3(-20.0f, -20.0f, 30.0f),
  };
  core::properties::Property<glm::vec3> tracked_bbox_max_cm{
      this,
      "tracked_bbox_max_cm",
      "Tracked bounding box max in centimeter units",
      glm::vec3(20.0f, 20.0f, 70.0f),
  };
  core::properties::Property<float> face_crop_scale{
      this,
      "face_crop_scale",
      "Scaling factor applied to 2D face bbox",
      3.4f,
  };
  core::properties::Property<glm::vec2> face_crop_offset{
      this,
      "face_crop_offset",
      "Offset applied to 2D face box [pixels]",
      {0.0f, 0.0f},
  };
  core::properties::Property<std::atomic<bool>> apply_roll_correction{
      this,
      "apply_roll_correction",
      "Whether to apply roll correction",
      true,
  };
  core::properties::Property<std::atomic<bool>> apply_crop{
      this,
      "apply_crop",
      "Whether to apply crop",
      true,
  };
  /// @}

  bool has_facebox = false;  ///< Whether a face has been found
  glm::vec3 raw_pose_euler;  ///< Pose rotation from tracking, relative to the face box. Rotation will be zero when
                             ///< looking directly at the camera.
  glm::vec3 face_box_to_display_translation;  ///< Translation relative to the display
  glm::quat face_box_to_display_rotation;     ///< Correct for rotation from face box in view space to display space
  float tracking_confidence = 0.0f;           ///< Based on landmark confidence

 private:
  std::mutex m_translationOffsetMutexLock;
};

}  // namespace components
}  // namespace trackingmodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_TRACKINGMODULE_COMPONENTS_TRACKEDHEADCOMPONENT_H_
