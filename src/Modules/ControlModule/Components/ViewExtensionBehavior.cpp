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

#include "ViewExtensionBehavior.h"

#include "Modules/CommonModule/Components/TransformComponent.h"
#include "Modules/RenderModule/Components/StereoViewComponent.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Static function declarations                                                                                     ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Extract horizontal and vertical angles to the rotation of a direction vector
/// @param[in] dir The direction to look at for achieving the rotation angles. The z component is assumed to be negative
/// @return        A vector representing the horizontal (x) and vertical (y) components of the rotation angles [radians]
static glm::vec2 DirectionToHorizontalVerticalAngles(const glm::vec3& dir);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Static function definitions                                                                                      ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static glm::vec2 DirectionToHorizontalVerticalAngles(const glm::vec3& dir) {
  return {std::atan(dir.x / dir.z), -std::atan(dir.y / dir.z)};
}

namespace nv3dvc {
namespace modules {
namespace controlmodule {
namespace components {

ViewExtensionBehavior::ViewExtensionBehavior() : m_lastRotation(glm::quat(0.0f, 0.0f, 0.0f, 1.0f)) {}

nv3dvc::core::Error ViewExtensionBehavior::OnInitialize() { return nv3dvc::core::SUCCESS; }

nv3dvc::core::Error ViewExtensionBehavior::OnEvent(core::events::Event* e) { return nv3dvc::core::SUCCESS; }

nv3dvc::core::Error ViewExtensionBehavior::OnUpdate(const float dt) {
  if (!(HasComponent<commonmodule::components::TransformComponent>() &&
        HasComponent<rendermodule::components::StereoViewComponent>())) {
    return core::SUCCESS;
  }
  auto& transform = GetComponent<commonmodule::components::TransformComponent>();
  auto& stereo_view = GetComponent<rendermodule::components::StereoViewComponent>();

  const glm::vec3 eye = stereo_view.MeanViewPoint();
  glm::vec2 angles =
      DirectionToHorizontalVerticalAngles(glm::normalize(-eye));  // -eye since we look in the direction of its negation
  angles.x *= horizontal_rotation_factor;
  angles.y *= vertical_rotation_factor;

  // Compose rotation
  glm::quat h_rot = glm::angleAxis(angles.x, glm::vec3(0.0f, 1.0f, 0.0f));
  glm::quat v_rot = glm::angleAxis(angles.y, glm::vec3(1.0f, 0.0f, 0.0f));
  glm::quat rotation = h_rot * v_rot;

  // Since we're overwriting the rotation until next frame, we save the last rotation and apply the difference instead
  // of accumulating
  glm::quat rotation_diff = rotation * glm::inverse(m_lastRotation);
  transform.rotation = *transform.rotation.get() * rotation_diff;
  m_lastRotation = rotation;

  // Remove roll using lookat
  glm::vec3 target_pos = *transform.rotation.get() * glm::vec3(0.0f, 0.0f, -1.0f);
  transform.rotation = glm::quatLookAt(glm::normalize(target_pos), glm::vec3(0.0f, 1.0f, 0.0f));

  return nv3dvc::core::SUCCESS;
}

}  // namespace components
}  // namespace controlmodule
}  // namespace modules
}  // namespace nv3dvc
