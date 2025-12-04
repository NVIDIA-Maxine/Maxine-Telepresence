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

#include "Callbacks.h"

#include <GLFW/glfw3.h>

#include "Core/Application/EventBroadcaster.h"
#include "Core/Application/Inputs.h"
#include "Core/Events/FileEvent.h"
#include "Core/Events/KeyEvent.h"
#include "Core/Events/MouseEvent.h"
#include "Core/Events/WindowEvent.h"
#include "Core/Util/Logger.h"
#include "InputMapping.h"

namespace nv3dvc {
namespace core {
namespace application {

void GlfwErrorCallback(int error_code, const char* description) { LOG_ERROR("GLFW Error: %s", description); }

void GlfwWindowPosCallback(GLFWwindow* window, int xpos, int ypos) {
  EventBroadcaster& broadcaster = *static_cast<EventBroadcaster*>(glfwGetWindowUserPointer(window));
  events::WindowPositionEvent e(xpos, ypos);
  broadcaster.event_callback(&e);
}

void GlfwWindowSizeCallback(GLFWwindow* window, int width, int height) {
  EventBroadcaster& broadcaster = *static_cast<EventBroadcaster*>(glfwGetWindowUserPointer(window));
  events::WindowSizeEvent e(width, height);
  broadcaster.event_callback(&e);
}

void GlfwWindowCloseCallback(GLFWwindow* window) {
  EventBroadcaster& broadcaster = *static_cast<EventBroadcaster*>(glfwGetWindowUserPointer(window));
  events::WindowCloseEvent e;
  broadcaster.event_callback(&e);
}

void GlfwWindowRefreshCallback(GLFWwindow* window) {
  EventBroadcaster& broadcaster = *static_cast<EventBroadcaster*>(glfwGetWindowUserPointer(window));
  events::WindowRefreshEvent e;
  broadcaster.event_callback(&e);
}

void GlfwWindowFocusCallback(GLFWwindow* window, int focused) {
  EventBroadcaster& broadcaster = *static_cast<EventBroadcaster*>(glfwGetWindowUserPointer(window));
  if (focused == GLFW_TRUE) {
    events::WindowFocusGainEvent e;
    broadcaster.event_callback(&e);
  } else {
    events::WindowFocusLoseEvent e;
    broadcaster.event_callback(&e);
  }
}

void GlfwWindowIconifyCallback(GLFWwindow* window, int iconified) {
  EventBroadcaster& broadcaster = *static_cast<EventBroadcaster*>(glfwGetWindowUserPointer(window));
  if (iconified == GLFW_TRUE) {
    events::WindowMinimizeEvent e;
    broadcaster.event_callback(&e);
  } else {
    events::WindowRestoreEvent e;
    broadcaster.event_callback(&e);
  }
}

void GlfwWindowMaximizeCallback(GLFWwindow* window, int maximized) {
  EventBroadcaster& broadcaster = *static_cast<EventBroadcaster*>(glfwGetWindowUserPointer(window));
  if (maximized == GLFW_TRUE) {
    events::WindowMaximizeEvent e;
    broadcaster.event_callback(&e);
  } else {
    events::WindowRestoreEvent e;
    broadcaster.event_callback(&e);
  }
}

void GlfwFramebufferSizeCallback(GLFWwindow* window, int width, int height) {
  EventBroadcaster& broadcaster = *static_cast<EventBroadcaster*>(glfwGetWindowUserPointer(window));
  events::FramebufferSizeEvent e(width, height);
  broadcaster.event_callback(&e);
}

void GlfwWindowContentScaleCallback(GLFWwindow* window, float xscale, float yscale) {
  EventBroadcaster& broadcaster = *static_cast<EventBroadcaster*>(glfwGetWindowUserPointer(window));
  events::WindowContentScaleEvent e(xscale, yscale);
  broadcaster.event_callback(&e);
}

void GlfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods_glfw) {
  EventBroadcaster& broadcaster = *static_cast<EventBroadcaster*>(glfwGetWindowUserPointer(window));
  inputs::Modifier mods = inputs::Modifier::NONE;
  mods |= inputs::MODIFIER_GLFW_TO_NV3DVC.at(mods_glfw & GLFW_MOD_SHIFT);
  mods |= inputs::MODIFIER_GLFW_TO_NV3DVC.at(mods_glfw & GLFW_MOD_CONTROL);
  mods |= inputs::MODIFIER_GLFW_TO_NV3DVC.at(mods_glfw & GLFW_MOD_ALT);
  mods |= inputs::MODIFIER_GLFW_TO_NV3DVC.at(mods_glfw & GLFW_MOD_SUPER);
  mods |= inputs::MODIFIER_GLFW_TO_NV3DVC.at(mods_glfw & GLFW_MOD_CAPS_LOCK);
  mods |= inputs::MODIFIER_GLFW_TO_NV3DVC.at(mods_glfw & GLFW_MOD_NUM_LOCK);

  switch (action) {
    case GLFW_PRESS: {
      events::KeyPressEvent e(inputs::KEY_GLFW_TO_NV3DVC.at(key), mods);
      broadcaster.event_callback(&e);
      break;
    };
    case GLFW_RELEASE: {
      events::KeyReleaseEvent e(inputs::KEY_GLFW_TO_NV3DVC.at(key), mods);
      broadcaster.event_callback(&e);
      break;
    };
    case GLFW_REPEAT: {
      events::KeyRepeatEvent e(inputs::KEY_GLFW_TO_NV3DVC.at(key), mods);
      broadcaster.event_callback(&e);
      break;
    }
    default: {
      printf("Unknown GLFW key action: %i", action);
    }
  }
}

void GlfwCharCallback(GLFWwindow* window, unsigned int codepoint) {
  EventBroadcaster& broadcaster = *static_cast<EventBroadcaster*>(glfwGetWindowUserPointer(window));
  events::CharEvent e(codepoint);
  broadcaster.event_callback(&e);
}

void GlfwCursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
  EventBroadcaster& broadcaster = *static_cast<EventBroadcaster*>(glfwGetWindowUserPointer(window));
  events::CursorPositionEvent e(xpos, ypos);
  broadcaster.event_callback(&e);
}

void GlfwCursorEnterCallback(GLFWwindow* window, int entered) {
  EventBroadcaster& broadcaster = *static_cast<EventBroadcaster*>(glfwGetWindowUserPointer(window));
  if (entered == GLFW_TRUE) {
    events::CursorEnterEvent e;
    broadcaster.event_callback(&e);
  } else {
    events::CursorExitEvent e;
    broadcaster.event_callback(&e);
  }
}

void GlfwMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
  EventBroadcaster& broadcaster = *static_cast<EventBroadcaster*>(glfwGetWindowUserPointer(window));
  switch (action) {
    case GLFW_PRESS: {
      events::MouseButtonPressEvent e(inputs::MOUSE_GLFW_TO_NV3DVC.at(button),
                                      inputs::MODIFIER_GLFW_TO_NV3DVC.at(mods));
      broadcaster.event_callback(&e);
      break;
    };
    case GLFW_RELEASE: {
      events::MouseButtonReleaseEvent e(inputs::MOUSE_GLFW_TO_NV3DVC.at(button),
                                        inputs::MODIFIER_GLFW_TO_NV3DVC.at(mods));
      broadcaster.event_callback(&e);
      break;
    };
    default: {
      printf("Unknown GLFW key action: %i", action);
    }
  }
}

void GlfwScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
  EventBroadcaster& broadcaster = *static_cast<EventBroadcaster*>(glfwGetWindowUserPointer(window));
  events::ScrollEvent e(xoffset, yoffset);
  broadcaster.event_callback(&e);
}

void GlfwDropCallback(GLFWwindow* window, int path_count, const char* paths[]) {
  EventBroadcaster& broadcaster = *static_cast<EventBroadcaster*>(glfwGetWindowUserPointer(window));
  for (size_t i = 0; i < path_count; i++) {
    events::FileEvent e(paths[i]);
    broadcaster.event_callback(&e);
  }
}

}  // namespace application
}  // namespace core
}  // namespace nv3dvc
