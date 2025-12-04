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

#include "StereoViewComponent.h"

#include <numeric>

#include "Modules/CommonModule/Components/CameraComponent.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/glm.hpp"
#include "glm/gtx/norm.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Static function declarations                                                                                     ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Determine if a point is inside or outside of a frustum defined by a 4x4 projection matrix
/// @param[in] point                     The 3D point to test. In camera space
/// @param[in] frustum_projection_matrix Projection matrix. Should transform from camera space to clip space
/// @return    true                      If the point is inside the frusutm
///            false                     Otherwise
static bool IsPointInsideFrustum(const glm::vec3& point, const glm::mat4 frustum_projection_matrix);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Static function definitions                                                                                      ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool IsPointInsideFrustum(const glm::vec3& point, const glm::mat4 frustum_projection_matrix) {
  glm::vec4 point_h = glm::vec4(point, 1.0f);
  glm::vec4 point_clip_space_h = frustum_projection_matrix * point_h;
  // Perspective division
  point_clip_space_h = point_clip_space_h / point_clip_space_h.w;
  return point_clip_space_h.x >= -1.0f && point_clip_space_h.x <= 1.0f && point_clip_space_h.y >= -1.0f &&
         point_clip_space_h.y <= 1.0f && point_clip_space_h.z >= -1.0f && point_clip_space_h.z <= 1.0f;
}

namespace nv3dvc {
namespace core {
namespace util {
namespace filter {
static glm::vec3 extrapolate(const glm::vec3& a, const glm::vec3& b, float alpha) { return glm::mix(a, b, alpha + 1); }
}  // namespace filter
}  // namespace util
}  // namespace core
}  // namespace nv3dvc

namespace nv3dvc {
namespace modules {
namespace rendermodule {
namespace components {

StereoViewComponent::StereoViewComponent() {
  m_eyeData.resize(2);                  // Two eyes per default
  m_eyeData[0] = {-0.03f, 0.0f, 0.6f};  // Left eye, Meters
  m_eyeData[1] = {0.03f, 0.0f, 0.6f};   // Right eye, Meters
  ReAllocateViewPointFilters(m_eyeData.size());
  UpdateViewsForEyes({1920, 1080}, {1, 0.5}, 1.0f / 60.0f);
  auto reset_filter_fn = [this]() {
    for (auto& filt : m_viewPointFilters) {
      filt.Reset(one_euro_filter_params.min_cutoff_freq, one_euro_filter_params.cutoff_slope,
                 one_euro_filter_params.deriv_cutoff_freq);
    }
  };
  one_euro_filter_params.min_cutoff_freq.SetOnChangeFunction(reset_filter_fn);
  one_euro_filter_params.cutoff_slope.SetOnChangeFunction(reset_filter_fn);
  one_euro_filter_params.deriv_cutoff_freq.SetOnChangeFunction(reset_filter_fn);
}

core::Error StereoViewComponent::SetViews(const std::vector<glm::vec3>& view_points,
                                          const glm::vec2& screen_size_pixels, const glm::vec2& screen_size_meters) {
  if (view_points.size() < 2) return core::Error::ERR_GENERAL;
  m_viewMatrices.resize(view_points.size());
  m_cameraIntrinsics.resize(view_points.size());
  m_viewPoints.resize(view_points.size());
  for (int i = 0; i < view_points.size(); i++) {
    // Update stored view points and view matrices.
    m_viewPoints[i] = view_points[i] * *scale_factor.get();
    commonmodule::components::CameraComponent::EyePositionToCameraIntrinsics(
        screen_size_pixels, screen_size_meters, m_viewPoints[i], &m_cameraIntrinsics[i].fx, &m_cameraIntrinsics[i].fy,
        &m_cameraIntrinsics[i].cx, &m_cameraIntrinsics[i].cy);
    m_viewMatrices[i] = glm::inverse(glm::translate(glm::mat4(1.0f), m_viewPoints[i]));
  }
  return core::Error::SUCCESS;
}

void StereoViewComponent::SetEyes(const glm::vec3& left, const glm::vec3& right, const float dt) {
  glm::vec3 proposed_eye_left = {fix_view ? default_eye_location.get()->x : left.x,
                                 fix_view ? default_eye_location.get()->y : left.y,
                                 fix_focal_length || fix_view ? default_eye_location.get()->z : left.z};
  glm::vec3 proposed_eye_right = {fix_view ? default_eye_location.get()->x : right.x,
                                  fix_view ? default_eye_location.get()->y : right.y,
                                  fix_focal_length || fix_view ? default_eye_location.get()->z : right.z};
  {
    std::lock_guard lock(m_eyeDataAccessorMutex);

    const glm::vec3 filtered_eye_left = m_viewPointFilters[0].Filter(proposed_eye_left, dt);
    const glm::vec3 filtered_eye_right = m_viewPointFilters[1].Filter(proposed_eye_right, dt);

    if (filter_view && filter_at_observation) {
      proposed_eye_left = filtered_eye_left;
      proposed_eye_right = filtered_eye_right;
    }

    if (detect_static_eyes) {
      // Compute mean eye speed.
      using core::util::filter::Magnitude;  // ADL two-step.
      const float speed = 0.5f * (Magnitude(m_viewPointFilters[0].GetFilteredVelocity()) +
                                  Magnitude(m_viewPointFilters[1].GetFilteredVelocity()));
      // Compute mean eye distance from the last stored eye positions.
      const float distance_since_last = 0.5f * (glm::length(proposed_eye_left - m_eyeData[0]) +  //
                                                glm::length(proposed_eye_right - m_eyeData[1]));
      // Decide whether the head is currently moving, based on speeed and distance from last update.
      // Speed threshold has hysteresis -- if eyes are static, threshold is high; if eyes are moving, threshold is low.
      const float motion_threshold = m_eyesStatic ? eyes_speed_thresholds.get()->y : eyes_speed_thresholds.get()->x;
      const float distance_threshold = eyes_distance_threshold;
      m_eyesStatic = speed < motion_threshold && distance_since_last < distance_threshold;
    } else {
      m_eyesStatic = false;
    }

    if (m_eyesStatic) {
      // If viewer is not moving, only update the eye positions very slowly.
      constexpr float kStaticEyesUpdateHalfLife = 1.0f;  // seconds
      // Compute alpha based on half-life.
      const float alpha = 1.0f - std::pow(0.5f, dt / kStaticEyesUpdateHalfLife);
      proposed_eye_left = glm::mix(m_eyeData[0], proposed_eye_left, alpha);
      proposed_eye_right = glm::mix(m_eyeData[1], proposed_eye_right, alpha);
    }

    m_eyeData = {proposed_eye_left, proposed_eye_right};

    m_hasNewObservation = true;
  }
}

void StereoViewComponent::SetIsConfident(const bool confident) { m_isConfident.store(confident); }

const std::vector<glm::mat4>& StereoViewComponent::ViewMatrices() const { return m_viewMatrices; }

const std::vector<NvAR_RenderCameraIntrinsicParams>& StereoViewComponent::CameraIntrinsics() const {
  return m_cameraIntrinsics;
}

glm::vec3 StereoViewComponent::MeanViewPoint() {
  std::lock_guard lock(m_eyeDataAccessorMutex);
  return std::accumulate(m_viewPoints.begin(), m_viewPoints.end(), glm::vec3(0.0f)) /
         static_cast<float>(m_viewPoints.size());
}

bool StereoViewComponent::IsConfident() { return m_isConfident.load(); }

void StereoViewComponent::UpdateViewsForEyes(const glm::vec2& screen_size_pixels, const glm::vec2& screen_size_meters,
                                             float dt) {
  size_t num_viewpoints = 0;
  {
    std::lock_guard lock(m_eyeDataAccessorMutex);
    num_viewpoints = m_eyeData.size();
    m_viewPoints.resize(num_viewpoints);
    ReAllocateViewPointFilters(num_viewpoints);
    if (m_hasNewObservation) {
      for (int i = 0; i < m_eyeData.size(); i++) {
        m_viewPointExtrapolators[i].Observe(m_eyeData[i]);
      }
      m_hasNewObservation = false;
    }
  }
  m_viewMatrices.resize(num_viewpoints);
  m_cameraIntrinsics.resize(num_viewpoints);

  float diff = parallax_falloff_rate * dt;
  if (!IsConfident()) {
    // If not confident, decrease parallax factor instead.
    // Speed of decrease should be slower than the speed of increase, to avoid oscillations.
    constexpr float kDecreaseSpeedFactor = 4.0f;
    diff *= -1.0f / kDecreaseSpeedFactor;
  }
  parallax_interpolation_factor += diff;
  parallax_interpolation_factor = glm::clamp(*parallax_interpolation_factor.get(), 0.0f, 1.0f);

  for (int i = 0; i < m_viewPoints.size(); i++) {
    glm::vec3 viewpoint_filtered = m_viewPointExtrapolators[i].GetLastVal();
    if (extrapolate_view) {
      viewpoint_filtered = m_viewPointExtrapolators[i].Extrapolate(dt);
    }
    if (filter_view && !filter_at_observation) {
      viewpoint_filtered = m_viewPointFilters[i].Filter(viewpoint_filtered, dt);
    }
    // Smoothly fall back to default eye position if confidence is low.
    viewpoint_filtered =
        glm::mix(*default_eye_location.get(), viewpoint_filtered, *parallax_interpolation_factor.get());
    // Update stored view points and view matrices.
    m_viewPoints[i] = viewpoint_filtered * *scale_factor.get();
    commonmodule::components::CameraComponent::EyePositionToCameraIntrinsics(
        screen_size_pixels, screen_size_meters, m_viewPoints[i], &m_cameraIntrinsics[i].fx, &m_cameraIntrinsics[i].fy,
        &m_cameraIntrinsics[i].cx, &m_cameraIntrinsics[i].cy);
    if (flip_y) {
      m_cameraIntrinsics[i].fy *= -1.0f;
    }
    m_viewMatrices[i] = glm::inverse(glm::translate(glm::mat4(1.0f), m_viewPoints[i]));
  }
}

void StereoViewComponent::ReAllocateViewPointFilters(size_t num_viewpoints) {
  if (m_viewPointFilters.size() != num_viewpoints) {
    m_viewPointFilters.resize(num_viewpoints,
                              {one_euro_filter_params.min_cutoff_freq, one_euro_filter_params.cutoff_slope,
                               one_euro_filter_params.deriv_cutoff_freq});
    m_viewPointExtrapolators.resize(
        num_viewpoints, core::util::filter::TemporalLinearExtrapolator<glm::vec3>{glm::vec3(0.0f, 0.0f, 0.6f)});
  }
}

}  // namespace components
}  // namespace rendermodule
}  // namespace modules
}  // namespace nv3dvc
