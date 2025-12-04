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

#ifndef SRC_CORE_APPLICATION_WINDOW_H_
#define SRC_CORE_APPLICATION_WINDOW_H_

#include <functional>
#include <string>

#include "Core/Application/Inputs.h"
#include "Core/Error.h"
#include "Core/Events/Event.h"
#include "EventBroadcaster.h"
#include "glm/glm.hpp"

namespace nv3dvc {
namespace core {
namespace application {

/// @brief Class for pointer to native window. Depends on implementation which is defined in Window.cpp
class NativeWindow {};

/// @brief Window class
class Window {
 public:
  /// @ingroup EnumProperties
  /// @defgroup WindowMode WindowMode
  /// @brief Window mode
  /// @{
  enum class WindowMode {
    WINDOWED,     ///< Windowed that can be moved or resized on the desktop
    FULL_SCREEN,  ///< Full screen window
    UNDECORATED,  ///< Undecorated window. Works as full screen if window size is not enforced
    HIDDEN,       ///< Hidden window. No window is shown on the desktop
  };
  /// @}

  Window();
  ~Window();

  /// @brief Initialize the window object
  /// @param width          The width of the window, in pixels
  /// @param height         The height of the window, in pixels
  /// @param name           The name displayed in the top bar of the window
  /// @param window_mode    Specification of the window mode
  /// @param enforce_size   Whether the width and the height of the window should be enforced by the resolution of the
  ///                       main monitor
  /// @param decorated      Whether to create a decorated window. An undecorated window can not be moved or resized
  /// @param event_callback Callback function for when an event is passed from the native window. No callback if nullptr
  /// @return true if successful
  nv3dvc::core::Error Initialize(int width, int height, const std::string& name, WindowMode window_mode,
                                 bool enforce_size, const std::function<void(events::Event*)>& event_callback);

  /// @brief Destroy the window
  /// @return true if successful
  nv3dvc::core::Error Uninitialize();

  /// @brief Get the native window pointer. Depends on the implementation defined in Window.cpp
  ///
  /// For example, will be the GLFWWindow* pointer if GLFW is used.
  /// @return the native Window pointer
  /// @return nullptr if uninitialized
  void* GetNativeWindowPtr() const;

  /// @brief Set the size of the window in units of pixels
  /// @param window_size The size of the window in units of pixels
  /// @return core::Error::SUCCESS if successful
  Error SetWindowSize(const glm::uvec2& window_size);

  /// @brief Set the position of the window in units of pixels
  /// @param window_position The position of the window in units of pixels
  /// @return core::Error::SUCCESS if successful
  Error SetWindowPosition(const glm::uvec2& window_position);

  /// @brief Enable window vertical sync with display
  void EnableVsync();

  /// @brief Disable window vertical sync with display
  void DisableVsync();

  /// @brief Get the size of the frame buffer
  /// @return The size of the frame buffer in pixels.
  ///         {0, 0} if uninitialized
  glm::uvec2 FramebufferSize() const;

  /// @brief Get the size of the window in pixels
  /// @return The size of the window in pixels
  ///         {0, 0} if uninitialized
  glm::uvec2 WindowSize() const;

  /// @brief Get the content scale of the window
  /// @return The content scale of the window
  ///         {0.0f, 0.0f} if uninitialized
  glm::fvec2 WindowContentScale() const;

  /// @brief Swap front / back buffer and synchronize. Will update the frame buffer
  nv3dvc::core::Error Update();

  /// @brief Close the window
  void Close();

  /// @brief True if the window has been requested to close
  /// @return true if the window has been requested to close
  ///         false otherwise, or if uninitialized
  bool ShouldClose() const;

  /// @brief Check whether a key is pressed
  /// @param key the key to test
  /// @return true if the key is pressed
  ///         false otherwise, or if uninitialized
  bool IsKeyPressed(const inputs::Key& key) const;

  /// @brief Check whether a mouse button is pressed
  /// @param mouse_button the mouse button to test
  /// @return true if the mouse button is pressed
  ///         false otherwise, or if uninitialized
  bool IsMouseButtonPressed(const inputs::Mouse& mouse_button) const;

  /// @brief Get the cursor position within the current window
  /// @return the coordinates {x, y} of the cursor
  ///         {-1.0, -1.0} if uninitialized
  glm::dvec2 GetCursorPosition() const;

 private:
  bool m_shouldClose;
  int m_swapInterval;
  NativeWindow* m_nativeWindow;
  EventBroadcaster m_eventBroadcaster;
};

}  // namespace application
}  // namespace core
}  // namespace nv3dvc

#endif  // SRC_CORE_APPLICATION_WINDOW_H_
