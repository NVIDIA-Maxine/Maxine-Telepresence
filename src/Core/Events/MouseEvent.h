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

#ifndef SRC_CORE_EVENTS_MOUSEEVENT_H_
#define SRC_CORE_EVENTS_MOUSEEVENT_H_

#include <string>

#include "Core/Application/Inputs.h"
#include "Event.h"

namespace nv3dvc {
namespace core {
namespace events {

/// @brief Base class for all mouse button event types
///
/// All mouse button events have a mouse button and a potential modifier
class MouseButtonEvent : public Event {
 public:
  MouseButtonEvent(const application::inputs::Mouse& button, const application::inputs::Modifier& modifier)
      : m_button(button), m_modifier(modifier) {}
  application::inputs::Mouse GetButton() const { return m_button; }
  application::inputs::Modifier GetModifier() const { return m_modifier; }

 private:
  application::inputs::Mouse m_button;
  application::inputs::Modifier m_modifier;

 protected:
  std::string StringInner() const {
    return "button : " + application::inputs::ToString(m_button) +
           ", modifier : " + application::inputs::ToString(m_modifier);
  }
};

/// @brief Event triggered when a mouse button is pressed
class MouseButtonPressEvent : public MouseButtonEvent {
 public:
  MouseButtonPressEvent(const application::inputs::Mouse& button, const application::inputs::Modifier& modifier)
      : MouseButtonEvent(button, modifier) {}
  EventType Type() const override { return EventType::MOUSE_BUTTON_PRESS_EVENT; }
  std::string ToString() const override { return "MouseButtonPressEvent : { " + StringInner() + " }"; }
};

/// @brief Event triggered when a mouse button is released
class MouseButtonReleaseEvent : public MouseButtonEvent {
 public:
  MouseButtonReleaseEvent(const application::inputs::Mouse& button, const application::inputs::Modifier& modifier)
      : MouseButtonEvent(button, modifier) {}
  EventType Type() const override { return EventType::MOUSE_BUTTON_RELEASE_EVENT; }
  std::string ToString() const override { return "MouseButtonReleaseEvent : { " + StringInner() + " }"; }
};

/// @brief Event triggered when the mouse is scrolled
class ScrollEvent : public Event {
 public:
  ScrollEvent(const double x_offset, const double y_offset) : m_xOffset(x_offset), m_yOffset(y_offset) {}
  EventType Type() const override { return EventType::SCROLL_EVENT; }
  std::string ToString() const override {
    return "ScrollEvent : { x_offset : " + std::to_string(m_xOffset) + ", y_offset : " + std::to_string(m_yOffset) +
           " }";
  }
  double GetXoffset() const { return m_xOffset; }
  double GetYoffset() const { return m_yOffset; }

 private:
  double m_xOffset;
  double m_yOffset;
};

/// @brief Event triggered when the cursor position changes
class CursorPositionEvent : public Event {
 public:
  CursorPositionEvent(const double x_position, const double y_position)
      : m_xPosition(x_position), m_yPosition(y_position) {}
  EventType Type() const override { return EventType::CURSOR_POSITION_EVENT; }
  std::string ToString() const override {
    return "CursorPositionEvent : { x_position : " + std::to_string(m_xPosition) +
           ", y_position : " + std::to_string(m_yPosition) + " }";
  }
  double GetXposition() const { return m_xPosition; }
  double GetYposition() const { return m_yPosition; }

 private:
  double m_xPosition;
  double m_yPosition;
};

/// @brief Event triggered when the cursor enters the window
class CursorEnterEvent : public Event {
 public:
  EventType Type() const override { return EventType::CURSOR_ENTER_EVENT; }
  std::string ToString() const override { return "CursorEnterEvent"; }
};

/// @brief Event triggered when the cursor exits the window
class CursorExitEvent : public Event {
 public:
  EventType Type() const override { return EventType::CURSOR_EXIT_EVENT; }
  std::string ToString() const override { return "CursorExitEvent"; }
};

}  // namespace events
}  // namespace core
}  // namespace nv3dvc

#endif  // SRC_CORE_EVENTS_MOUSEEVENT_H_
