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

#ifndef SRC_CORE_EVENTS_WINDOWEVENT_H_
#define SRC_CORE_EVENTS_WINDOWEVENT_H_

#include <string>

#include "Event.h"

namespace nv3dvc {
namespace core {
namespace events {

/// @brief Event triggered when the window position changes
class WindowPositionEvent : public Event {
 public:
  WindowPositionEvent(const int x_position, const int y_position) : m_xPosition(x_position), m_yPosition(y_position) {}
  EventType Type() const override { return EventType::WINDOW_POSITION_EVENT; }
  std::string ToString() const override {
    return "WindowPositionEvent : { x_position : " + std::to_string(m_xPosition) +
           ", y_position : " + std::to_string(m_yPosition) + " }";
  }
  int GetXposition() const { return m_xPosition; }
  int GetYposition() const { return m_yPosition; }

 private:
  int m_xPosition;
  int m_yPosition;
};

/// @brief Event triggered when the window size changes
class WindowSizeEvent : public Event {
 public:
  WindowSizeEvent(const int x_size, const int y_size) : m_xSize(x_size), m_ySize(y_size) {}
  EventType Type() const override { return EventType::WINDOW_SIZE_EVENT; }
  std::string ToString() const override {
    return "WindowSizeEvent : { x_size : " + std::to_string(m_xSize) + ", y_size : " + std::to_string(m_ySize) + "}";
  }

  int GetXsize() const { return m_xSize; }
  int GetYsize() const { return m_ySize; }

 private:
  int m_xSize;
  int m_ySize;
};

/// @brief Event triggered when the window is closed
class WindowCloseEvent : public Event {
 public:
  EventType Type() const override { return EventType::WINDOW_CLOSE_EVENT; }
  std::string ToString() const override { return "WindowCloseEvent"; }
};

/// @brief Event triggered when the window needs to be refreshed
class WindowRefreshEvent : public Event {
 public:
  EventType Type() const override { return EventType::WINDOW_REFRESH_EVENT; }
  std::string ToString() const override { return "WindowRefreshEvent"; }
};

/// @brief Event triggered when the window gains focus
class WindowFocusGainEvent : public Event {
 public:
  EventType Type() const override { return EventType::WINDOW_FOCUS_GAIN_EVENT; }
  std::string ToString() const override { return "WindowFocusGainEvent"; }
};

/// @brief Event triggered when the window loses focus
class WindowFocusLoseEvent : public Event {
 public:
  EventType Type() const override { return EventType::WINDOW_FOCUS_LOSE_EVENT; }
  std::string ToString() const override { return "WindowFocusLoseEvent"; }
};

/// @brief Event triggered when the window is minimized
class WindowMinimizeEvent : public Event {
 public:
  EventType Type() const override { return EventType::WINDOW_MINIMIZE_EVENT; }
  std::string ToString() const override { return "WindowMinimizeEvent"; }
};

/// @brief Event triggered when the window is maximized
class WindowMaximizeEvent : public Event {
 public:
  EventType Type() const override { return EventType::WINDOW_MAXIMIZE_EVENT; }
  std::string ToString() const override { return "WindowMaximizeEvent"; }
};

/// @brief Event triggered when the window is restored
class WindowRestoreEvent : public Event {
 public:
  EventType Type() const override { return EventType::WINDOW_RESTORE_EVENT; }
  std::string ToString() const override { return "WindowRestoreEvent"; }
};

/// @brief Event triggered when the framebuffer size changes
class FramebufferSizeEvent : public Event {
 public:
  FramebufferSizeEvent(const int width, const int height) : m_width(width), m_height(height) {}
  EventType Type() const override { return EventType::FRAMEBUFFER_SIZE_EVENT; }
  std::string ToString() const override {
    return "FramebufferSizeEvent : { width : " + std::to_string(m_width) + ", height : " + std::to_string(m_height) +
           " }";
  }

  int GetWidth() const { return m_width; }
  int GetHeight() const { return m_height; }

 private:
  int m_width;
  int m_height;
};

/// @brief Event triggered when the window content scale changes
class WindowContentScaleEvent : public Event {
 public:
  WindowContentScaleEvent(const float x_scale, const float y_scale) : m_xScale(x_scale), m_yScale(y_scale) {}
  EventType Type() const override { return EventType::WINDOW_CONTENT_SCALE_EVENT; }
  std::string ToString() const override {
    return "WindowContentScaleEvent : { x_scale : " + std::to_string(m_xScale) +
           ", y_scale : " + std::to_string(m_yScale) + " }";
  }

  float GetXscale() const { return m_xScale; }
  float GetYscale() const { return m_yScale; }

 private:
  float m_xScale;
  float m_yScale;
};

}  // namespace events
}  // namespace core
}  // namespace nv3dvc

#endif  // SRC_CORE_EVENTS_WINDOWEVENT_H_
