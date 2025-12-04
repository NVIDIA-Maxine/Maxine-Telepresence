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

#ifndef SRC_MODULES_RENDERMODULE_COMPONENTS_STEREOVIEWCOMPONENT_H_
#define SRC_MODULES_RENDERMODULE_COMPONENTS_STEREOVIEWCOMPONENT_H_

#include <mutex>  // NOLINT(build/c++11) (cpplint outside Chromium: https://github.com/google/styleguide/issues/483)
#include <string>
#include <vector>

#include "Core/EntityComponentSystem/Component.h"
#include "Core/Error.h"
#include "Core/Util/OneEuroFilter.h"
#include "Core/Util/TemporalLinearExtrapolator.h"
#include "Core/Util/Timeable.h"
#include "Modules/CommonModule/OneEuroFilterProperties.h"
#include "glm/glm.hpp"
#include "nvAR.h"

namespace nv3dvc {
namespace modules {
namespace rendermodule {
namespace components {

/// @defgroup StereoViewComponentProperties StereoViewComponent
/// @ingroup  ComponentProperties
/// @brief    Component used to enable view dependent rendering
///
/// This component is used by the rendering feature, to render the scene from the current view point, projected onto the
/// display using a skewed frustum perspective. Multiple viewpoints can be used if the display supports it.

/// @see StereoViewComponentProperties
class StereoViewComponent : public core::ecs::Component, public core::util::Timeable {
 public:
  constexpr static const char* NAME = "StereoViewComponent";
  std::string Name() const override { return NAME; };

  StereoViewComponent();

  /// @brief Set all the views at once
  ///
  /// Does not require subsequent call to UpdateViewsForEyes
  /// @param[in] view_points        View points in the reference frame of the display (right handed system with x right,
  /// and y up)
  /// @param[in] screen_size_pixels The size of the screen (width, height) [pixels]
  /// @param[in] screen_size_meters The size of the screen (width, height) [meters]
  /// @return    core::Error::SUCCESS If successful
  core::Error SetViews(const std::vector<glm::vec3>& view_points, const glm::vec2& screen_size_pixels,
                       const glm::vec2& screen_size_meters);

  /// @brief Set the first two view points representing the eyes
  ///
  /// Requires subsequent call to UpdateViewsForEyes
  /// @param[in] left  The left eye location in the reference frame of the display (right handed system with x right,
  ///                  and y up)
  /// @param[in] right The right eye location in the reference frame of the display (right handed system with x right,
  ///                  and y up)
  /// @param[in] dt    Time differential of the last rendered frame [seconds]. Used for temporal view filtering
  void SetEyes(const glm::vec3& left, const glm::vec3& right, float dt);

  /// @brief Set whether the view is considered confident or not
  /// @param[in] confident Whether the view is confident
  void SetIsConfident(bool confident);

  /// @brief Get the view matrices of all the views
  /// @return The view matrices
  const std::vector<glm::mat4>& ViewMatrices() const;

  /// @brief Get the camera intrinsics of all the views
  /// @return The camera intrinsics
  const std::vector<NvAR_RenderCameraIntrinsicParams>& CameraIntrinsics() const;

  /// @brief Compute the mean of all view point positions
  /// @return The mean view point in the reference frame of the display (right handed system with x right, and y up)
  glm::vec3 MeanViewPoint();

  /// @brief After eyes have been set, update view matrices and intrinsics
  /// @param[in] screen_size_pixels The size of the screen (width, height) [pixels]
  /// @param[in] screen_size_meters The size of the screen (width, height) [meters]
  /// @param[in] dt                 Time differential of the last rendered frame [seconds]. Used for temporal view
  /// filtering
  void UpdateViewsForEyes(const glm::vec2& screen_size_pixels, const glm::vec2& screen_size_meters, float dt);

  /// @brief Whether the view is confident
  /// @return Whether the view is confident
  bool IsConfident();

 public:
  /// @ingroup StereoViewComponentProperties
  /// @{
  core::properties::Property<float> scale_factor = {
      this,
      "scale_factor",
      "Scale factor applied to all view point coordinates. Simulates what view would be rendered if the relative size "
      "of the display was different.",
      1.0f,
  };
  core::properties::Property<float> parallax_falloff_rate = {
      this,
      "parallax_falloff_rate",
      "Recovery speed factor for when view confidence changes",
      1.0f,
  };
  core::properties::Property<bool> fix_focal_length = {
      this,
      "fix_focal_length",
      "Whether to use a fixed focal length (i.e. the default_eye_location's z coordinate) for all views even if the "
      "value changes",
      false,
  };
  core::properties::Property<bool> fix_view = {
      this,
      "fix_view",
      "Use default view",
      false,
  };
  core::properties::Property<bool> extrapolate_view = {
      this,
      "extrapolate_view",
      "Whether to apply temporal linear extrepolation to view points",
      false,
  };
  core::properties::Property<bool> filter_view = {
      this,
      "filter_view",
      "Whether to temporally filter the viewpoints",
      true,
  };
  core::properties::Property<bool> filter_at_observation = {
      this,
      "filter_at_observation",
      "Whether to filter the view points at observation, rather than at rendering time",
      false,
  };
  core::properties::Property<bool> detect_static_eyes = {
      this,
      "detect_static_eyes",
      "Whether to apply motion detection to the eyes in SetEyes. When motion is not detected, the viewpoint will be "
      "heavily smoothed for stability",
      true,
  };
  core::properties::Property<glm::vec2> eyes_speed_thresholds = {
      this,
      "eyes_speed_thresholds",
      "Hysteresis thresholds on speed of eyes (in m/s) for static eyes detection. When the speed exceeds the upper "
      "threshold, the eyes will be classified as moving until the speed goes below the lower threshold.",
      {0.02f, 0.04f},
  };
  core::properties::Property<float> eyes_distance_threshold = {
      this,
      "eyes_distance_threshold",
      "Threshold on distance of eyes (in m) from last update, for static eyes detection",
      0.005f,
  };
  core::properties::Property<bool> viewpoint_from_head_pose = {
      this,
      "viewpoint_from_head_pose",
      "Use the head pose to set the viewpoint. Otherwise use the right pupil.",
      false,
  };
  core::properties::Property<glm::vec3> default_eye_location = {
      this,
      "default_eye_location",
      "The default eye location used when confidence in the view is low, or if focal length is fixed",
      {0.0f, 0.0f, 0.6f},
  };
  commonmodule::OneEuroFilterProperties one_euro_filter_params = {
      this, "one_euro_filter_params", 10.0f, 100.0f, 10.0f,
  };
  core::properties::Property<float> parallax_interpolation_factor = {
      this,
      "parallax_interpolation_factor",
      "Under normal circumstances, this value is 1.0. If the tracked view is not confident, however, this value moves "
      "towards 0.0. 0.0 means no parallax display, and instead a fixed camera projection",
      1.0f,
  };
  core::properties::Property<bool> flip_y = {
      this,
      "flip_y",
      "Whether to invert the resulting camera instrinsic's vertical focal length. This reqults in flipping the image "
      "verically",
      false,
  };
  /// @}

 private:
  void ReAllocateViewPointFilters(size_t num_viewpoints);

  std::vector<glm::vec3> m_eyeData;
  std::vector<glm::vec3> m_viewPoints;
  std::vector<glm::mat4> m_viewMatrices;
  std::vector<core::util::filter::OneEuroFilter<glm::vec3>> m_viewPointFilters;
  std::vector<core::util::filter::TemporalLinearExtrapolator<glm::vec3>> m_viewPointExtrapolators;
  std::vector<NvAR_RenderCameraIntrinsicParams> m_cameraIntrinsics;
  std::mutex m_eyeDataAccessorMutex;
  bool m_hasNewObservation = false;
  bool m_eyesStatic = false;
  std::atomic<bool> m_isConfident = {true};
};

}  // namespace components
}  // namespace rendermodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_RENDERMODULE_COMPONENTS_STEREOVIEWCOMPONENT_H_
