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

#ifndef SRC_MODULES_CAPTUREMODULE_CAMERADESCRIPTOR_H_
#define SRC_MODULES_CAPTUREMODULE_CAMERADESCRIPTOR_H_

#include <string>

namespace nv3dvc {
namespace modules {
namespace capturemodule {

/// @brief See @ref CameraDescriptorPropertiesProperties
struct CameraDescriptor {
  int camera_device_index;
  std::string camera_device_path;
  int cuda_device_index;
  bool flip_horizontal;
  bool auto_exposure;
  float exposure;
  float gain;
  float fps;
};

}  // namespace capturemodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_CAPTUREMODULE_CAMERADESCRIPTOR_H_
