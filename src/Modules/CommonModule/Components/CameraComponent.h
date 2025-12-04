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

#ifndef SRC_MODULES_COMMONMODULE_COMPONENTS_CAMERACOMPONENT_H_
#define SRC_MODULES_COMMONMODULE_COMPONENTS_CAMERACOMPONENT_H_

#include <string>

#include "Core/EntityComponentSystem/Component.h"
#include "Core/Properties/Property.h"
#include "glm/glm.hpp"

namespace nv3dvc {
namespace modules {
namespace commonmodule {
namespace components {

/// @defgroup CameraComponentProperties CameraComponent
/// @ingroup  ComponentProperties
/// @brief    Component representing a virtual camera

/// See @ref CameraComponentProperties
class CameraComponent : public core::ecs::Component {
 public:
  constexpr static const char* NAME = "CameraComponent";
  std::string Name() const override { return NAME; };

  CameraComponent();
  ~CameraComponent() override;

  /// @brief Get the main camera
  /// If only one camera component exists in the scene, it will be the main camera.
  /// The main camera will be used for rendering the view
  /// @return The main camera of the scene
  static CameraComponent* MainCamera();

  /// @brief Set the camera intrinsics
  /// @param[in] focal_length  Vertical focal length
  /// @param[in] cx            Principal point's x coordinate. typically width_pixels / 2, unless the frustum is skewed
  /// @param[in] cy            Principal point's y coordinate. typically height_pixels / 2, unless the frustum is skewed
  /// @param[in] width_pixels  The image display width, in pixels
  /// @param[in] height_pixels The image display height, in pixels
  void SetIntrinsics(float focal_length, float cx, float cy, unsigned int width_pixels, unsigned int height_pixels);

  /// @brief Set the main camera of the scene
  /// @param[in] main_camera The camera to be the main camera
  static void SetMainCamera(CameraComponent* main_camera);

  /// @brief Makes this the main camera
  void SetMainCamera();

  /// @brief Get the view matrix, previously set with SetViewMatrix
  /// @return The GL view matrix
  const glm::mat4& GetViewMatrix();

  /// @brief Get the projection matrix corresponding to the camera intrinsics
  /// @return The GL projection matrix
  const glm::mat4& GetProjectionMatrix();

  /// @brief Get the focal length in pixel units
  /// @return The focal length [pixels]
  const float GetFocalLength() const;

  /// @brief Get the principal point, i.e. the center of projection on the image plane in pixel units
  /// @return The principal point [pixels]
  glm::fvec2 GetPrincipalPoint() const;

  /// @brief Set the view matrix
  /// The view matrix should be the inverse of the model transform matrix corresponding to the camera's translation and
  /// rotation in 3D space
  /// @param[in] view_matrix The GL view matrix.
  void SetViewMatrix(const glm::mat4& view_matrix);

  /// @brief Given a 3D eye position, update the camera's intrinsics, and projection matrix to correspond to a view from
  /// which the perspective projection ends up on the view plane
  /// @param[in] screen_size_pixels The (x,y) size of the image display, in pixels
  /// @param[in] screen_size        The physical screen size, measured in units of meters
  /// @param[in] eye_position       The 3D position of focus, in camera space
  void UpdateIntrinsicsFromEyePosition(const glm::vec2& screen_size_pixels, const glm::vec2& screen_size,
                                       const glm::vec3& eye_position);

  /// @brief Camera intrinsics for off center camera
  /// Produces intrinsics in units of pixels
  /// @param[in]  screen_size_pixels Screen size in pixels
  /// @param[in]  screen_size        Screen size in metric units. Unit needs to be the same as the unit of eye_position
  /// @param[in]  eye_position       Eye position in metric units. Unit needs to be the same as the unit of screen_size
  /// @param[out] fx                 Horizontal focal length
  /// @param[out] fy                 Vertical focal length
  /// @param[out] cx                 Horizontal coordinate for principal point
  /// @param[out] cy                 Vertical coordinate for principal point
  static void EyePositionToCameraIntrinsics(const glm::vec2& screen_size_pixels, const glm::vec2& screen_size,
                                            const glm::vec3& eye_position, float* fx, float* fy, float* cx, float* cy);

  /// @brief Converts an OpenCV style camera intrinsics matrix to an OpenGL style camera perspective projection matrix
  ///
  /// The Complete footprint of the camera intrinsics matrix should be as follows. This is a column major matrix:
  ///
  /// |fx, 0, cx|
  /// |0, fy, cy|
  /// |0,  0,  1|
  ///
  /// The output OpenGL, column major, projection matrix:
  ///
  /// | 2n/(r-l),        0,  (r+l)/(r-l),          0 |
  /// |        0, 2n/(t-b),  (t+b)/(t-b),          0 |
  /// |        0,        0, -(f+n)/(f-n), -2fn/(f-n) |
  /// |        0,        0,           -1,          0 |
  ///
  /// where n=near, r=right, l=left, t=top, b=bottom, f=far in the view frustum
  ///
  /// The OpenGL projection matrix converts 3D points (with homogeneous coordinate) to clip space. After perspective
  /// divide, points will be in normalized device coordinates.
  ///
  /// @brief Compute OpenGL perspective projection for off center camera
  /// @param[in] fx Vertical focal length in pixels
  /// @param[in] fy Horizontal focal length in pixels
  /// @param[in] cx Principal point's x coordinate in pixels
  /// @param[in] cy Principal point's y coordinate in pixels
  /// @param[in] width_pixels The horizontal display size in pixels
  /// @param[in] height_pixels The vertical display size in pixels
  /// @param[in] hither The distance to the resulting near plane
  /// @param[in] yon The distance to the resulting far plane
  /// @return The OpenGL projection matrix corresponding to the input
  static glm::fmat4 CameraIntrinsicsToOpenGlPerspectiveProjectionMatrix(float fx, float fy, float cx, float cy,
                                                                        unsigned int width_pixels,
                                                                        unsigned int height_pixels, float hither,
                                                                        float yon);

 public:
  /// @ingroup CameraComponentProperties
  /// @{
  core::properties::Property<float> focal_length = {
      this,
      "focal_length",
      "Focal length [pixels]",
      1080.0f,
  };
  core::properties::Property<glm::fvec2> principal_point = {
      this,
      "principal_point",
      "Principal point [pixels]",
      {960.0f, 540.0f},
  };
  core::properties::Property<float> clipping_plane_near = {
      this,
      "clipping_plane_near",
      "Near clipping plane distance [meters]",
      0.01f,
  };
  core::properties::Property<float> clipping_plane_far = {
      this,
      "clipping_plane_far",
      "Far clipping plane distance [meters]",
      10.0f,
  };
  /// @}

 private:
  glm::mat4 m_viewMatrix = glm::mat4(1.0f);
  glm::mat4 m_projectionMatrix = glm::mat4(1.0f);
  static CameraComponent* s_mainCamera;
};

}  // namespace components
}  // namespace commonmodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_COMMONMODULE_COMPONENTS_CAMERACOMPONENT_H_
