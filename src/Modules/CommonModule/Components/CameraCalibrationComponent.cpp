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

#include "CameraCalibrationComponent.h"

namespace nv3dvc {
namespace modules {
namespace commonmodule {
namespace components {

CameraCalibrationComponent::CameraCalibrationComponent() {
  vfov.SetOnChangeFunction([this]() {
    if (use_vfov) {
      fy = image_height / 2.0f / glm::tan(glm::radians(static_cast<float>(vfov)) / 2.0f);
      fx = *fy.get();
      cx = image_width / 2.0f;
      cy = image_height / 2.0f;
    }
  });
  use_vfov.SetOnChangeFunction([this]() { vfov.OnChange(); });
  Update();
}

void CameraCalibrationComponent::Update() {
  if (use_vfov) {
    vfov.OnChange();
  } else {
    vfov = glm::degrees(2.0f * atan(image_height / 2.0f / fy));
  }
}

CameraCalibration CameraCalibrationComponent::GetCameraCalibration() const {
  return {image_width, image_height, fx, fy, cx, cy};
}

}  // namespace components
}  // namespace commonmodule
}  // namespace modules
}  // namespace nv3dvc
