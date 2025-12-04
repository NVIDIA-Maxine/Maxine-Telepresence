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

#include "Core/Application/Window.h"

#include <GLFW/glfw3.h>

#include "Callbacks.h"
#include "Core/Events/Event.h"
#include "InputMapping.h"

#ifdef _WIN32
#include <Windows.h>
#include <dwmapi.h>
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "dwmapi.lib")
#endif

namespace nv3dvc {
namespace core {
namespace application {

Window::Window() : m_shouldClose(false), m_swapInterval(0), m_nativeWindow(nullptr) {}

Window::~Window() { Uninitialize(); }

nv3dvc::core::Error Window::Initialize(int width, int height, const std::string& name, WindowMode window_mode,
                                       bool enforce_size, const std::function<void(events::Event*)>& event_callback) {
  glfwSetErrorCallback(GlfwErrorCallback);

  if (!glfwInit()) {
    return nv3dvc::core::ERR_WINDOW;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);
  glfwWindowHint(GLFW_DECORATED, window_mode != WindowMode::UNDECORATED);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_VISIBLE, window_mode != WindowMode::HIDDEN);

  GLFWmonitor* monitor = glfwGetPrimaryMonitor();
  GLFWmonitor* init_monitor = window_mode == WindowMode::FULL_SCREEN ? monitor : nullptr;
  const GLFWvidmode* mode = glfwGetVideoMode(monitor);
  const int init_width = enforce_size ? width : mode->width;
  const int init_height = enforce_size ? height : mode->height;

  auto* glfw_window = glfwCreateWindow(init_width, init_height, name.c_str(), init_monitor, nullptr);
  if (!glfw_window) {
    glfwTerminate();
    return nv3dvc::core::ERR_WINDOW;
  }
  m_nativeWindow = reinterpret_cast<NativeWindow*>(glfw_window);

  m_shouldClose = glfwWindowShouldClose(glfw_window);

  if (event_callback) {
    m_eventBroadcaster.event_callback = event_callback;
    glfwSetWindowUserPointer(glfw_window, &m_eventBroadcaster);
    // Window callbacks
    glfwSetWindowPosCallback(glfw_window, GlfwWindowPosCallback);
    glfwSetWindowSizeCallback(glfw_window, GlfwWindowSizeCallback);
    glfwSetWindowCloseCallback(glfw_window, GlfwWindowCloseCallback);
    glfwSetWindowRefreshCallback(glfw_window, GlfwWindowRefreshCallback);
    glfwSetWindowFocusCallback(glfw_window, GlfwWindowFocusCallback);
    glfwSetWindowIconifyCallback(glfw_window, GlfwWindowIconifyCallback);
    glfwSetWindowMaximizeCallback(glfw_window, GlfwWindowMaximizeCallback);
    glfwSetFramebufferSizeCallback(glfw_window, GlfwFramebufferSizeCallback);
    glfwSetWindowContentScaleCallback(glfw_window, GlfwWindowContentScaleCallback);

    // Key callbacks
    glfwSetKeyCallback(glfw_window, GlfwKeyCallback);
    glfwSetCharCallback(glfw_window, GlfwCharCallback);

    // Mouse callbacks
    glfwSetCursorPosCallback(glfw_window, GlfwCursorPosCallback);
    glfwSetCursorEnterCallback(glfw_window, GlfwCursorEnterCallback);
    glfwSetMouseButtonCallback(glfw_window, GlfwMouseButtonCallback);
    glfwSetScrollCallback(glfw_window, GlfwScrollCallback);

    // Other window callbacks
    glfwSetDropCallback(glfw_window, GlfwDropCallback);
  }

  glfwMakeContextCurrent(glfw_window);
  return nv3dvc::core::SUCCESS;
}

nv3dvc::core::Error Window::Uninitialize() {
  if (m_nativeWindow) {
    auto* glfw_window = reinterpret_cast<GLFWwindow*>(m_nativeWindow);
    glfwDestroyWindow(glfw_window);
    m_nativeWindow = nullptr;
  }
  glfwTerminate();
  return nv3dvc::core::SUCCESS;
}

void* Window::GetNativeWindowPtr() const { return m_nativeWindow; }

nv3dvc::core::Error Window::SetWindowSize(const glm::uvec2& window_size) {
  if (m_nativeWindow) {
    auto* glfw_window = reinterpret_cast<GLFWwindow*>(m_nativeWindow);
    glfwSetWindowSize(glfw_window, window_size.x, window_size.y);
    return core::Error::SUCCESS;
  }
  return core::Error::ERR_GENERAL;
}

nv3dvc::core::Error Window::SetWindowPosition(const glm::uvec2& window_position) {
  if (m_nativeWindow) {
    auto* glfw_window = reinterpret_cast<GLFWwindow*>(m_nativeWindow);
    glfwSetWindowPos(glfw_window, window_position.x, window_position.y);
    return core::Error::SUCCESS;
  }
  return core::Error::ERR_GENERAL;
}

void Window::EnableVsync() {
  m_swapInterval = 1;
  glfwSwapInterval(m_swapInterval);
}

void Window::DisableVsync() {
  m_swapInterval = 0;
  glfwSwapInterval(m_swapInterval);
}

glm::uvec2 Window::FramebufferSize() const {
  int width = 0;
  int height = 0;
  if (m_nativeWindow) {
    auto* glfw_window = reinterpret_cast<GLFWwindow*>(m_nativeWindow);
    glfwGetFramebufferSize(glfw_window, &width, &height);
  }
  return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
}

glm::uvec2 Window::WindowSize() const {
  int width = 0;
  int height = 0;
  if (m_nativeWindow) {
    auto* glfw_window = reinterpret_cast<GLFWwindow*>(m_nativeWindow);
    glfwGetWindowSize(glfw_window, &width, &height);
  }
  return {width, height};
}

glm::fvec2 Window::WindowContentScale() const {
  float x_scale = 0.0f;
  float y_scale = 0.0f;
  if (m_nativeWindow) {
    auto* glfw_window = reinterpret_cast<GLFWwindow*>(m_nativeWindow);
    glfwGetWindowContentScale(glfw_window, &x_scale, &y_scale);
  }
  return {x_scale, y_scale};
}

nv3dvc::core::Error Window::Update() {
  if (m_nativeWindow) {
    auto* glfw_window = reinterpret_cast<GLFWwindow*>(m_nativeWindow);
    glfwSwapBuffers(glfw_window);
#ifdef _WIN32
    // This improves high CPU usage and poor performance on Windows with vsync.
    BOOL enabled = FALSE;
    if (SUCCEEDED(DwmIsCompositionEnabled(&enabled)) && enabled) {
      int interval = m_swapInterval;
      while (interval--) DwmFlush();
    }
#endif
    glfwPollEvents();
    m_shouldClose = glfwWindowShouldClose(glfw_window);
  }
  return nv3dvc::core::SUCCESS;
}

void Window::Close() {
  if (m_nativeWindow) {
    auto* glfw_window = reinterpret_cast<GLFWwindow*>(m_nativeWindow);
    glfwSetWindowShouldClose(glfw_window, true);
  }
}

bool Window::ShouldClose() const { return m_shouldClose; }

bool Window::IsKeyPressed(const inputs::Key& key) const {
  if (m_nativeWindow) {
    auto* glfw_window = reinterpret_cast<GLFWwindow*>(m_nativeWindow);
    const auto state = glfwGetKey(glfw_window, inputs::KEY_NV3DVC_TO_GLFW.at(key));
    return state == GLFW_PRESS || state == GLFW_REPEAT;
  }
  return false;
}

bool Window::IsMouseButtonPressed(const inputs::Mouse& mouse_button) const {
  if (m_nativeWindow) {
    auto* glfw_window = reinterpret_cast<GLFWwindow*>(m_nativeWindow);
    const auto state = glfwGetMouseButton(glfw_window, inputs::MOUSE_NV3DVC_TO_GLFW.at(mouse_button));
    return state == GLFW_PRESS;
  }
  return false;
}

glm::dvec2 Window::GetCursorPosition() const {
  glm::dvec2 cursor_pos = {-1.0, -1.0};
  if (m_nativeWindow) {
    auto* glfw_window = reinterpret_cast<GLFWwindow*>(m_nativeWindow);
    glfwGetCursorPos(glfw_window, &cursor_pos.x, &cursor_pos.y);
  }
  return cursor_pos;
}

}  // namespace application
}  // namespace core
}  // namespace nv3dvc
