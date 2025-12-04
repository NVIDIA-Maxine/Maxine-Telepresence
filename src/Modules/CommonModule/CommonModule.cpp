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

#include "CommonModule.h"

namespace nv3dvc {
namespace modules {
namespace commonmodule {

glm::mat4 GetGlobalTransform(const core::ecs::Entity& entity) {
  core::ecs::Entity parent = entity;
  glm::mat4 full_transform = entity.HasComponent<components::TransformComponent>()
                                 ? entity.GetComponent<components::TransformComponent>().GetLocalMatrix()
                                 : glm::mat4(1.0f);
  if (!entity.IsValid()) return full_transform;
  while (parent.GetParent().IsValid() && (!parent.HasComponent<components::TransformComponent>() ||
                                          parent.GetComponent<components::TransformComponent>().relative)) {
    parent = parent.GetParent();
    if (!parent.HasComponent<components::TransformComponent>()) continue;
    full_transform = parent.GetComponent<components::TransformComponent>().GetRt() * full_transform;
  }
  return full_transform;
}

glm::mat4 GetGlobalTransformFiltered(const core::ecs::Entity& entity) {
  core::ecs::Entity parent = entity;
  glm::mat4 full_transform = entity.HasComponent<components::TransformComponent>()
                                 ? entity.GetComponent<components::TransformComponent>().GetLocalMatrixFiltered()
                                 : glm::mat4(1.0f);
  while (parent.GetParent().IsValid() && (!parent.HasComponent<components::TransformComponent>() ||
                                          parent.GetComponent<components::TransformComponent>().relative)) {
    parent = parent.GetParent();
    if (!parent.HasComponent<components::TransformComponent>()) continue;
    full_transform = parent.GetComponent<components::TransformComponent>().GetRtFiltered() * full_transform;
  }
  return full_transform;
}

CommonModule::CommonModule() {
  RegisterComponent<components::CameraComponent>();
  RegisterComponent<components::CameraCalibrationComponent>();
  RegisterComponent<components::DataBufferComponent>();
  RegisterComponent<components::TransformComponent>();
  RegisterComponent<components::VideoFrameComponent>();
  RegisterComponent<components::EncodedVideoBufferComponent>();
  RegisterComponent<components::EncodedVideoCallbackComponent>();
  RegisterComponent<components::RecordingCallbackComponent>();
  RegisterComponent<components::WebCameraAudioCallbackComponent>();
}

CommonModule::~CommonModule() {}

core::Error CommonModule::Initialize() { return core::SUCCESS; }

core::Error CommonModule::Uninitialize() { return core::Error::SUCCESS; }

}  // namespace commonmodule
}  // namespace modules
}  // namespace nv3dvc
