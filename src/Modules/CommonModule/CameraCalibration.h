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

#ifndef SRC_MODULES_COMMONMODULE_CAMERACALIBRATION_H_
#define SRC_MODULES_COMMONMODULE_CAMERACALIBRATION_H_

namespace nv3dvc {
namespace modules {
namespace commonmodule {

/// @brief See @ref CameraCalibrationPropertiesProperties
struct CameraCalibration {
  int image_width;
  int image_height;
  float fx;
  float fy;
  float cx;
  float cy;
};

}  // namespace commonmodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_COMMONMODULE_CAMERACALIBRATION_H_
