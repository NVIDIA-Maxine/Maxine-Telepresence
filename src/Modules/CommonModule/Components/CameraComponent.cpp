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

#include "CameraComponent.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace nv3dvc {
namespace modules {
namespace commonmodule {
namespace components {

CameraComponent* CameraComponent::s_mainCamera = nullptr;

CameraComponent::CameraComponent() {
  if (!s_mainCamera) {
    SetMainCamera();
  }
  SetIntrinsics(focal_length, principal_point.get()->x, principal_point.get()->y, principal_point.get()->x * 2,
                principal_point.get()->y * 2);
}

CameraComponent::~CameraComponent() {
  if (s_mainCamera == this) s_mainCamera = nullptr;
}

CameraComponent* CameraComponent::MainCamera() { return s_mainCamera; }

void CameraComponent::SetIntrinsics(float focal_length, float cx, float cy, const unsigned int width_pixels,
                                    const unsigned int height_pixels) {
  focal_length = focal_length;
  principal_point = {cx, cy};
  m_projectionMatrix = CameraIntrinsicsToOpenGlPerspectiveProjectionMatrix(
      focal_length, focal_length, principal_point.get()->x, principal_point.get()->y, width_pixels, height_pixels,
      clipping_plane_near, clipping_plane_far);
}

void CameraComponent::SetMainCamera(CameraComponent* main_camera) { s_mainCamera = main_camera; }

void CameraComponent::SetMainCamera() { CameraComponent::SetMainCamera(this); }

const glm::mat4& CameraComponent::GetViewMatrix() { return m_viewMatrix; }

const glm::mat4& CameraComponent::GetProjectionMatrix() { return m_projectionMatrix; }

const float CameraComponent::GetFocalLength() const { return focal_length; }

glm::fvec2 CameraComponent::GetPrincipalPoint() const { return principal_point; }

void CameraComponent::SetViewMatrix(const glm::mat4& view_matrix) { m_viewMatrix = view_matrix; }

void CameraComponent::UpdateIntrinsicsFromEyePosition(const glm::vec2& screen_size_pixels, const glm::vec2& screen_size,
                                                      const glm::vec3& eye_position) {
  float fx, fy, cx, cy;
  EyePositionToCameraIntrinsics(screen_size_pixels, screen_size, eye_position, &fx, &fy, &cx, &cy);
  if (cx != principal_point.get()->x || cy != principal_point.get()->y) {
    principal_point = {cx, cy};
  }
  focal_length = fy;
  m_projectionMatrix = CameraIntrinsicsToOpenGlPerspectiveProjectionMatrix(
      fx, fy, principal_point.get()->x, principal_point.get()->y, screen_size_pixels.x, screen_size_pixels.y,
      clipping_plane_near, clipping_plane_far);
}

void CameraComponent::EyePositionToCameraIntrinsics(const glm::vec2& screen_size_pixels, const glm::vec2& screen_size,
                                                    const glm::vec3& eye_position, float* fx, float* fy, float* cx,
                                                    float* cy) {
  const float scale_y = screen_size_pixels.y / screen_size.y;  // To convert from metric to pixel units
  const float scale_x = screen_size_pixels.x / screen_size.x;  // To convert from metric to pixel units
  const float focal_length_pixels_y = eye_position.z * scale_y;
  const float focal_length_pixels_x = eye_position.z * scale_x;
  const float offset_x_pixels = eye_position.x * scale_x;
  const float offset_y_pixels = -eye_position.y * scale_y;  // From Y down to Y up
  // Intrinsics use pixel size as unit
  if (fx) *fx = focal_length_pixels_x;                          // Horizontal focal length
  if (fy) *fy = focal_length_pixels_y;                          // Vertical focal length
  if (cx) *cx = screen_size_pixels.x * 0.5f + offset_x_pixels;  // Principal point x, offset by eye location
  if (cy) *cy = screen_size_pixels.y * 0.5f + offset_y_pixels;  // Principal point y, offset by eye location
}

glm::fmat4 CameraComponent::CameraIntrinsicsToOpenGlPerspectiveProjectionMatrix(float fx, float fy, float cx, float cy,
                                                                                const unsigned int width_pixels,
                                                                                const unsigned int height_pixels,
                                                                                const float hither, const float yon) {
  const float one_over_fx = 1.0f / fx;
  const float one_over_fy = 1.0f / fy;
  // Compute frustum coordinates using symmetric triangles
  const float left = -hither * cx * one_over_fx;
  const float right = hither * (width_pixels - cx) * one_over_fx;
  const float top = hither * cy * one_over_fy;
  const float bottom = -hither * (height_pixels - cy) * one_over_fy;
  const glm::fmat4 res = glm::frustum(left, right, bottom, top, hither, yon);
  return res;
}

}  // namespace components
}  // namespace commonmodule
}  // namespace modules
}  // namespace nv3dvc
