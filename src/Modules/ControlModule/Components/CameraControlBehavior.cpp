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

#include "CameraControlBehavior.h"

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Core/Events/KeyEvent.h"
#include "Core/Events/MouseEvent.h"
#include "Core/Events/WindowEvent.h"
#include "Modules/CommonModule/Components/CameraComponent.h"
#include "Modules/CommonModule/Components/TransformComponent.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Static function declarations                                                                                     ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Compute a ray based on the position of the mouse cursor in image space
/// @param[in] mouse_position_pixels The position of the cursor in image space, y down
/// @param[in] window_size_pixels    The size of the window, covering the full view of the camera, in pixels
/// @param[in] inverse_projection    The inverse of the camera projection matrix
/// @return                          A normalized vector representing the ray in the camera's local coordinates
static glm::vec3 RayFromMousePosition(const glm::vec2& mouse_position_pixels, const glm::vec2& window_size_pixels,
                                      const glm::mat4& inverse_projection);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Static function definitions                                                                                      ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static glm::vec3 RayFromMousePosition(const glm::vec2& mouse_position_pixels, const glm::vec2& window_size_pixels,
                                      const glm::mat4& inverse_projection) {
  glm::vec2 mouse_position_norm = glm::vec2(mouse_position_pixels) / window_size_pixels;  // [0, 1]
  mouse_position_norm.y = 1.0f - mouse_position_norm.y;                                   // Flip y
  const glm::vec4 mouse_position_clip_space =
      glm::vec4((mouse_position_norm - glm::vec2(0.5f, 0.5f)) * 2.0f, -1.0f, 1.0f);  // [-1, 1]
  glm::vec4 target_pos = inverse_projection * mouse_position_clip_space;
  return glm::normalize(glm::vec3(target_pos));
}

namespace nv3dvc {
namespace modules {
namespace controlmodule {
namespace components {

CameraControlBehavior::CameraControlBehavior()
    : m_lastMousePosition(glm::vec3(255, 255, 255)),
      m_mousePosition(glm::vec3(255, 255, 255)),
      m_localVelocity(glm::vec3(0.0f, 0.0f, 0.0f)),
      m_windowSizePixels(glm::vec2(1920, 1080)),
      m_forwardPressed(false),
      m_leftPressed(false),
      m_rightPressed(false),
      m_backPressed(false),
      m_upPressed(false),
      m_downPressed(false),
      m_grabPressed(false) {}

nv3dvc::core::Error CameraControlBehavior::OnInitialize() { return nv3dvc::core::SUCCESS; }

nv3dvc::core::Error CameraControlBehavior::OnEvent(core::events::Event* e) {
  switch (e->Type()) {
    case core::events::EventType::KEY_PRESS_EVENT: {
      auto& evt = e->As<core::events::KeyPressEvent>();
      if (evt.GetModifier() != core::application::inputs::Modifier::NONE) {
        // Don't move if there is a modifier key.
        break;
      } else if (evt.GetKey() == kKeyForward) {
        m_forwardPressed = true;
      } else if (evt.GetKey() == kKeyLeft) {
        m_leftPressed = true;
      } else if (evt.GetKey() == kKeyRight) {
        m_rightPressed = true;
      } else if (evt.GetKey() == kKeyBack) {
        m_backPressed = true;
      } else if (evt.GetKey() == kKeyUp) {
        m_upPressed = true;
      } else if (evt.GetKey() == kKeyDown) {
        m_downPressed = true;
      }
      break;
    }
    case core::events::EventType::KEY_RELEASE_EVENT: {
      auto& evt = e->As<core::events::KeyReleaseEvent>();
      if (evt.GetKey() == kKeyForward) {
        m_forwardPressed = false;
      } else if (evt.GetKey() == kKeyLeft) {
        m_leftPressed = false;
      } else if (evt.GetKey() == kKeyRight) {
        m_rightPressed = false;
      } else if (evt.GetKey() == kKeyBack) {
        m_backPressed = false;
      } else if (evt.GetKey() == kKeyUp) {
        m_upPressed = false;
      } else if (evt.GetKey() == kKeyDown) {
        m_downPressed = false;
      }
      break;
    }
    case core::events::EventType::MOUSE_BUTTON_PRESS_EVENT: {
      auto& evt = e->As<core::events::MouseButtonPressEvent>();
      if (evt.GetButton() == kMouseButtonGrab) {
        m_grabPressed = true;
      }
      break;
    }
    case core::events::EventType::MOUSE_BUTTON_RELEASE_EVENT: {
      auto& evt = e->As<core::events::MouseButtonReleaseEvent>();
      if (evt.GetButton() == kMouseButtonGrab) {
        m_grabPressed = false;
      }
      break;
    }
    case core::events::EventType::WINDOW_SIZE_EVENT: {
      auto& evt = e->As<core::events::WindowSizeEvent>();
      m_windowSizePixels = glm::vec2(evt.GetXsize(), evt.GetYsize());
      break;
    }
    default:
      break;
  }
  return nv3dvc::core::SUCCESS;
}

nv3dvc::core::Error CameraControlBehavior::OnUpdate(const float dt) {
  core::application::inputs::Input* input = Input();
  if (!(HasComponent<commonmodule::components::TransformComponent>() &&
        HasComponent<commonmodule::components::CameraComponent>() && enable_mouse_control && input)) {
    return nv3dvc::core::SUCCESS;
  }
  auto& transform = GetComponent<commonmodule::components::TransformComponent>();
  auto& camera = GetComponent<commonmodule::components::CameraComponent>();

  // WASD Keyboard input for translation
  m_localVelocity.x = m_leftPressed ? -1.0f : m_rightPressed ? 1.0f : 0.0f;
  m_localVelocity.y = m_upPressed ? -1.0f : m_downPressed ? 1.0f : 0.0f;
  m_localVelocity.z = m_forwardPressed ? -1.0f : m_backPressed ? 1.0f : 0.0f;
  m_localVelocity *= static_cast<float>(translation_speed);
  // Apply translation in local space
  glm::vec3 translation_diff_local = *transform.rotation.get() * m_localVelocity;
  transform.translation += translation_diff_local * dt;

  // Mouse cursor input for rotation
  m_mousePosition = input->GetCursorPosition();
  if (m_grabPressed) {
    const glm::mat4& proj = camera.GetProjectionMatrix();
    const glm::mat4 iproj = glm::inverse(proj);

    const glm::vec3 target_ray = RayFromMousePosition(m_mousePosition, m_windowSizePixels, iproj);
    const glm::vec3 last_target_ray = RayFromMousePosition(m_lastMousePosition, m_windowSizePixels, iproj);
    const glm::vec3 rotation_diff_aa = glm::cross(target_ray, last_target_ray);
    const float rotation_diff_angle = glm::length(rotation_diff_aa);
    const glm::quat rotation_diff = glm::abs(rotation_diff_angle) > 1e-3f
                                        ? glm::angleAxis(rotation_diff_angle, rotation_diff_aa / rotation_diff_angle)
                                        : glm::identity<glm::quat>();
    transform.rotation = *transform.rotation.get() * rotation_diff;
    transform.rotation = glm::normalize(*transform.rotation.get());

    // Remove roll using lookat
    glm::vec3 target_pos = *transform.rotation.get() * glm::vec3(0.0f, 0.0f, -1.0f);
    transform.rotation = glm::quatLookAt(glm::normalize(target_pos), glm::vec3(0.0f, 1.0f, 0.0f));
  }
  m_lastMousePosition = m_mousePosition;

  // The camera view matrix is the inverse of the camera's transform
  camera.SetViewMatrix(glm::inverse(GetGlobalTransform()));
  return nv3dvc::core::SUCCESS;
}

}  // namespace components
}  // namespace controlmodule
}  // namespace modules
}  // namespace nv3dvc
