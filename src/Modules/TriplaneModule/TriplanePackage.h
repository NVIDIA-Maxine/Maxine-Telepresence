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

#ifndef SRC_MODULES_TRIPLANEMODULE_TRIPLANEPACKAGE_H_
#define SRC_MODULES_TRIPLANEMODULE_TRIPLANEPACKAGE_H_

#include <vector>

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "nlohmann/json.hpp"
#include "nvCVTriplaneVolume.h"

namespace nv3dvc {
namespace modules {
namespace triplanemodule {

/// @brief Triplane metadata package
struct TriplanePackage {
  int client_id;
  /// @brief (1 + focalScale) is a corrective factor with which to scale focal length for the size of the head in
  /// view to remain constant.
  ///
  /// Alternatively, apply (1 + focalScale) to the object and keep camera intrinsic constant.
  float focal_scale;
  float confidence;
  int64_t timestamp;
  std::vector<float> maxs;
  std::vector<float> mins;
  float head_scale;
  glm::vec2 gaze_angle;
  glm::quat head_pose_quaternion;   ///< Stores face_box_to_display_rotation;
  glm::vec3 head_pose_translation;  ///< Stores face_box_to_display_translation;
  glm::vec3 raw_tracking_pose_euler;
  glm::quat triplane_quaternion;
  glm::vec3 triplane_translation;
};

struct TriplaneFrame {
  TriplanePackage triplane_package;
  NvCVTriplaneVolume triplane_volume;
};

/// @brief Convert a triplane package to a json object
///
/// Keys are camelCase for backward compatibility
/// @param[in,out] j                The resulting json object
/// @param[in]     triplane_package The triplane package
void to_json(nlohmann::json& j,  // NOLINT(runtime/references) (owned by nlohmann::json)
             const TriplanePackage& triplane_package);

/// @brief Convert a json object to a triplane package
///
/// Keys are camelCase for backward compatibility
/// @param[in,out] j                The json object
/// @param[in]     triplane_package The resulting triplane package
void from_json(const nlohmann::json& j,
               TriplanePackage& triplane_package);  // NOLINT(runtime/references) (owned by nlohmann::json)

}  // namespace triplanemodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_TRIPLANEMODULE_TRIPLANEPACKAGE_H_
