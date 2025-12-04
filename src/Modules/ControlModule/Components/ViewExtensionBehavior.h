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

#ifndef SRC_MODULES_CONTROLMODULE_COMPONENTS_VIEWEXTENSIONBEHAVIOR_H_
#define SRC_MODULES_CONTROLMODULE_COMPONENTS_VIEWEXTENSIONBEHAVIOR_H_

#include <string>

#include "Core/Application/Inputs.h"
#include "Modules/BehaviorModule/Components/BehaviorComponent.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

namespace nv3dvc {
namespace modules {
namespace controlmodule {
namespace components {

/// @defgroup ViewExtensionBehaviorProperties ViewExtensionBehavior
/// @ingroup  ComponentProperties
/// @brief    Behavior component used to extend the view of the scene by applying an additional rotation to the
/// transform
///
/// Acts on entities having a TransformComponent and a StereoViewComponent attached.
/// The rotational component of the transform is modified based on the viewpoint of the StereoViewComponent. This
/// enables a wider view range of the scene.

/// See @ref ViewExtensionBehaviorProperties
class ViewExtensionBehavior : public behaviormodule::components::BehaviorComponent {
 public:
  constexpr static const char* NAME = "ViewExtensionBehavior";
  std::string Name() const override { return NAME; };

  ViewExtensionBehavior();

  nv3dvc::core::Error OnEvent(core::events::Event* e) override;
  nv3dvc::core::Error OnUpdate(float dt) override;
  nv3dvc::core::Error OnInitialize() override;

 public:
  /// @ingroup ViewExtensionBehaviorProperties
  /// @{
  core::properties::Property<float> horizontal_rotation_factor = {
      this,
      "horizontal_rotation_factor",
      "Scale factor applied to the horizontal rotation component",
      1.0f,
  };
  core::properties::Property<float> vertical_rotation_factor = {
      this,
      "vertical_rotation_factor",
      "Scale factor applied to the vertical rotation component",
      0.1f,
  };
  /// @}
 private:
  glm::quat m_lastRotation;
};

}  // namespace components
}  // namespace controlmodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_CONTROLMODULE_COMPONENTS_VIEWEXTENSIONBEHAVIOR_H_
