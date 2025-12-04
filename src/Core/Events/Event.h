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

#ifndef SRC_CORE_EVENTS_EVENT_H_
#define SRC_CORE_EVENTS_EVENT_H_

#include <string>

namespace nv3dvc {
namespace core {
/// @brief Namespace for events
namespace events {

/// @brief Event types
///
/// Every event type requires an implementation in the form of an extension of the base class Event
enum class EventType {
  WINDOW_POSITION_EVENT,       ///< Event triggered when the window position changes -- WindowPositionEvent
  WINDOW_SIZE_EVENT,           ///< Event triggered when the window size changes -- WindowSizeEvent
  WINDOW_CLOSE_EVENT,          ///< Event triggered when the window is closed -- WindowCloseEvent
  WINDOW_REFRESH_EVENT,        ///< Event triggered when the window needs to be refreshed -- WindowRefreshEvent
  WINDOW_FOCUS_GAIN_EVENT,     ///< Event triggered when the window gains focus -- WindowFocusGainEvent
  WINDOW_FOCUS_LOSE_EVENT,     ///< Event triggered when the window loses focus -- WindowFocusLoseEvent
  WINDOW_MINIMIZE_EVENT,       ///< Event triggered when the window is minimized -- WindowMinimizeEvent
  WINDOW_MAXIMIZE_EVENT,       ///< Event triggered when the window is maximized -- WindowMaximizeEvent
  WINDOW_RESTORE_EVENT,        ///< Event triggered when the window is restored -- WindowRestoreEvent
  FRAMEBUFFER_SIZE_EVENT,      ///< Event triggered when the framebuffer size changes -- FramebufferSizeEvent
  WINDOW_CONTENT_SCALE_EVENT,  ///< Event triggered when the window content scale changes -- WindowContentScaleEvent
  KEY_PRESS_EVENT,             ///< Event triggered when a key is pressed -- KeyPressEvent
  KEY_RELEASE_EVENT,           ///< Event triggered when a key is released -- KeyReleaseEvent
  KEY_REPEAT_EVENT,            ///< Event triggered when a key is repeated -- KeyRepeatEvent
  CHAR_EVENT,                  ///< Event triggered when a character is entered -- CharEvent
  CURSOR_POSITION_EVENT,       ///< Event triggered when the cursor position changes -- CursorPositionEvent
  CURSOR_ENTER_EVENT,          ///< Event triggered when the cursor enters the window -- CursorEnterEvent
  CURSOR_EXIT_EVENT,           ///< Event triggered when the cursor exits the window -- CursorExitEvent
  MOUSE_BUTTON_PRESS_EVENT,    ///< Event triggered when a mouse button is pressed -- MouseButtonPressEvent
  MOUSE_BUTTON_RELEASE_EVENT,  ///< Event triggered when a mouse button is released -- MouseButtonReleaseEvent
  SCROLL_EVENT,                ///< Event triggered when the mouse is scrolled -- ScrollEvent
  FILE_EVENT,                  ///< Event triggered when a file should be loaded -- FileEvent
};

/// @brief Event base class. Extend this class for all events in EventType
class Event {
 public:
  /// @brief Destructor
  virtual ~Event() = default;

  /// @brief Get the type of the event
  /// @return The type of the event
  virtual EventType Type() const = 0;

  /// @brief Convert the event to a string representation
  /// @return A string representation of the event
  virtual std::string ToString() const = 0;

  /// @brief Consume the event to prevent it from being passed to the next system
  void Consume();

  /// @brief Check if the event has already been consumed
  /// @return True if the event has been consumed, false otherwise
  bool IsConsumed() const;

  /// @brief Downcast the event to its original type
  ///
  /// Example usage:
  /// @code
  /// if (event.Type() == core::events::EventType::KeyPressEvent) {
  ///   auto& key_event = e.As<core::events::KeyPressEvent>();
  ///   // Use key_event
  /// }
  /// @endcode
  /// @tparam EventClass One of the types defined in EventType
  /// @return The event as EventType
  template <typename EventClass>
  EventClass& As() {
#ifdef NDEBUG
    return *static_cast<EventClass*>(this);
#else   // DEBUG
    return *dynamic_cast<EventClass*>(this);
#endif  // *DEBUG
  }

 private:
  bool m_consumed = false;
};

/// @brief Output the event to an ostream
/// @param out The ostream to output to
/// @param e The event to output
/// @return The ostream
std::ostream& operator<<(std::ostream& out, Event const& e);

}  // namespace events
}  // namespace core
}  // namespace nv3dvc

#endif  // SRC_CORE_EVENTS_EVENT_H_
