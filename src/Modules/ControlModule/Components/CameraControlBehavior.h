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

#ifndef SRC_MODULES_CONTROLMODULE_COMPONENTS_CAMERACONTROLBEHAVIOR_H_
#define SRC_MODULES_CONTROLMODULE_COMPONENTS_CAMERACONTROLBEHAVIOR_H_

#include <string>

#include "Core/Application/Inputs.h"
#include "Modules/BehaviorModule/Components/BehaviorComponent.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

namespace nv3dvc {
namespace modules {
namespace controlmodule {
namespace components {

/// @defgroup CameraControlBehaviorProperties CameraControlBehavior
/// @ingroup  ComponentProperties
/// @brief    Behavior component to control camera movement using mouse and keyboard input
///
/// The entity can be controlled using WASD + QE keyboard inputs for translation and mouse drag for rotation
///
/// Acts on entities having a TransformComponent and a CameraComponent attached

/// See @ref CameraControlBehaviorProperties
class CameraControlBehavior : public behaviormodule::components::BehaviorComponent {
 public:
  constexpr static const char* NAME = "CameraControlBehavior";
  std::string Name() const override { return NAME; };

  CameraControlBehavior();

  nv3dvc::core::Error OnEvent(core::events::Event* e) override;
  nv3dvc::core::Error OnUpdate(float dt) override;
  nv3dvc::core::Error OnInitialize() override;

 public:
  /// @ingroup CameraControlBehaviorProperties
  /// @{
  core::properties::Property<bool> enable_mouse_control = {
      this,
      "enable_mouse_control",
      "Whether to enable keyboard and mouse control",
      true,
  };
  core::properties::Property<float> translation_speed = {
      this,
      "translation_speed",
      "Multiplying factor applied to translation when Translation keys are pressed",
      1.0f,
  };
  /// @}

 private:
  glm::vec2 m_lastMousePosition;
  glm::vec2 m_mousePosition;
  glm::vec3 m_localVelocity;
  glm::vec2 m_windowSizePixels;

  // State
  bool m_forwardPressed;
  bool m_leftPressed;
  bool m_rightPressed;
  bool m_backPressed;
  bool m_upPressed;
  bool m_downPressed;
  bool m_grabPressed;

  // Controller
  static const core::application::inputs::Key kKeyForward = core::application::inputs::Key::W;
  static const core::application::inputs::Key kKeyLeft = core::application::inputs::Key::A;
  static const core::application::inputs::Key kKeyRight = core::application::inputs::Key::D;
  static const core::application::inputs::Key kKeyBack = core::application::inputs::Key::S;
  static const core::application::inputs::Key kKeyUp = core::application::inputs::Key::Q;
  static const core::application::inputs::Key kKeyDown = core::application::inputs::Key::E;
  static const core::application::inputs::Mouse kMouseButtonGrab = core::application::inputs::Mouse::BUTTON_1;
};

}  // namespace components
}  // namespace controlmodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_CONTROLMODULE_COMPONENTS_CAMERACONTROLBEHAVIOR_H_
