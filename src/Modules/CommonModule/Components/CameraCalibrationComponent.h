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

#ifndef SRC_MODULES_COMMONMODULE_COMPONENTS_CAMERACALIBRATIONCOMPONENT_H_
#define SRC_MODULES_COMMONMODULE_COMPONENTS_CAMERACALIBRATIONCOMPONENT_H_

#include <string>

#include "Core/EntityComponentSystem/Component.h"
#include "Modules/CommonModule/CameraCalibration.h"
#include "glm/glm.hpp"

namespace nv3dvc {
namespace modules {
namespace commonmodule {
namespace components {

/// @defgroup CameraCalibrationComponentProperties CameraCalibrationComponent
/// @ingroup  ComponentProperties
/// @brief A camera calibration specification for physical camera devices
///
/// Supports the pinhole camera model, without distortion parameters

/// See @ref CameraCalibrationComponentProperties
class CameraCalibrationComponent : public core::ecs::Component {
 public:
  constexpr static const char* NAME = "CameraCalibrationComponent";
  std::string Name() const override { return "CameraCalibrationComponent"; };

  CameraCalibrationComponent();
  ~CameraCalibrationComponent() override = default;

  void Update();
  CameraCalibration GetCameraCalibration() const;

 public:
  /// @ingroup CameraCalibrationComponentProperties
  /// @{
  core::properties::Property<float> vfov = {
      this,
      "vfov",
      "Vertical field of view, implicitly set using fy if use_vfov is false",
      kDefaultvFov,
  };
  core::properties::Property<bool> use_vfov = {
      this,
      "use_vfov",
      "If true, fx, fy, cx, and cy will be set according to vfov",
      true,
  };
  core::properties::Property<int> image_width = {
      this,
      "image_width",
      "Desired image width",
      1280,
  };
  core::properties::Property<int> image_height = {
      this,
      "image_height",
      "Desired image height",
      720,
  };
  core::properties::Property<float> fx = {
      this,
      "fx",
      "Horizontal focal length, same as fy if pixels are uniform",
      (image_height / 2.0f) / glm::tan(glm::radians(kDefaultvFov / 2.0f)),
  };
  core::properties::Property<float> fy = {
      this,
      "fy",
      "Vertical focal length in pixel units",
      (image_height / 2.0f) / glm::tan(glm::radians(kDefaultvFov / 2.0f)),
  };
  core::properties::Property<float> cx = {
      this,
      "cx",
      "Principal point's horizontal coordinate in pixel units",
      image_width / 2.0f,
  };
  core::properties::Property<float> cy = {
      this,
      "cy",
      "Principal point's vertical coordinate in pixel units",
      image_height / 2.0f,
  };
  /// @}

 public:
  static constexpr float kDefaultvFov = 43.3067;  /// Corresponds to diagonal fov of 78 degrees at 16:9
};

}  // namespace components
}  // namespace commonmodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_COMMONMODULE_COMPONENTS_CAMERACALIBRATIONCOMPONENT_H_
