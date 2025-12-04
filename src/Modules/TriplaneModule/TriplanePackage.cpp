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

#include "TriplanePackage.h"

#include "Core/Serialization/CustomJsonTypesGlm.h"

namespace nv3dvc {
namespace modules {
namespace triplanemodule {

void to_json(nlohmann::json& j,  // NOLINT: [runtime/references] (Owned by nlohmann::json)
             const TriplanePackage& triplane_package) {
  j = nlohmann::json{{"clientId", triplane_package.client_id},
                     {"gazeAngle", triplane_package.gaze_angle},
                     {"focalScale", triplane_package.focal_scale},
                     {"confidence", triplane_package.confidence},
                     {"headPoseQuaternion", triplane_package.head_pose_quaternion},
                     {"headPoseTranslation", triplane_package.head_pose_translation},
                     {"headScale", triplane_package.head_scale},
                     {"mins", triplane_package.mins},
                     {"maxs", triplane_package.maxs},
                     {"rawTrackingPoseEuler", triplane_package.raw_tracking_pose_euler},
                     {"triplaneQuaternion", triplane_package.triplane_quaternion},
                     {"triplaneTranslation", triplane_package.triplane_translation},
                     {"timestamp", triplane_package.timestamp}};
}

void from_json(const nlohmann::json& j,
               TriplanePackage& triplane_package) {  // NOLINT: [runtime/references] (Owned by nlohmann::json)
  if (j.contains("clientId")) j.at("clientId").get_to(triplane_package.client_id);
  if (j.contains("gazeAngle")) j.at("gazeAngle").get_to(triplane_package.gaze_angle);
  if (j.contains("focalScale")) j.at("focalScale").get_to(triplane_package.focal_scale);
  if (j.contains("confidence")) j.at("confidence").get_to(triplane_package.confidence);
  if (j.contains("headPoseQuaternion")) j.at("headPoseQuaternion").get_to(triplane_package.head_pose_quaternion);
  if (j.contains("headPoseTranslation")) j.at("headPoseTranslation").get_to(triplane_package.head_pose_translation);
  if (j.contains("headScale")) j.at("headScale").get_to(triplane_package.head_scale);
  if (j.contains("mins")) j.at("mins").get_to(triplane_package.mins);
  if (j.contains("maxs")) j.at("maxs").get_to(triplane_package.maxs);
  if (j.contains("rawTrackingPoseEuler")) j.at("rawTrackingPoseEuler").get_to(triplane_package.raw_tracking_pose_euler);
  if (j.contains("triplaneQuaternion")) j.at("triplaneQuaternion").get_to(triplane_package.triplane_quaternion);
  if (j.contains("triplaneTranslation")) j.at("triplaneTranslation").get_to(triplane_package.triplane_translation);
  if (j.contains("timestamp")) j.at("timestamp").get_to(triplane_package.timestamp);
}

}  // namespace triplanemodule
}  // namespace modules
}  // namespace nv3dvc
