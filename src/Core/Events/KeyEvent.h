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

#ifndef SRC_CORE_EVENTS_KEYEVENT_H_
#define SRC_CORE_EVENTS_KEYEVENT_H_

#include <string>

#include "Core/Application/Inputs.h"
#include "Event.h"

namespace nv3dvc {
namespace core {
namespace events {

/// @brief Base class for all key event types
///
/// All key events have a Key and a potential modifier
class KeyEvent : public Event {
 public:
  KeyEvent(const application::inputs::Key& key, const application::inputs::Modifier modifier)
      : m_key(key), m_modifier(modifier) {}
  application::inputs::Key GetKey() const { return m_key; }
  application::inputs::Modifier GetModifier() const { return m_modifier; }

 private:
  application::inputs::Key m_key;
  application::inputs::Modifier m_modifier;

 protected:
  std::string StringInner() const {
    return "key : " + application::inputs::ToString(m_key) +
           ", modifier : " + application::inputs::ToString(m_modifier);
  }
};

/// @brief Event triggered when a key is pressed
class KeyPressEvent : public KeyEvent {
 public:
  KeyPressEvent(const application::inputs::Key& key, const application::inputs::Modifier modifier)
      : KeyEvent(key, modifier) {}
  EventType Type() const override { return EventType::KEY_PRESS_EVENT; }
  std::string ToString() const override { return "KeyPressEvent : { " + StringInner() + " }"; }
};

/// @brief Event triggered when a key is released
class KeyReleaseEvent : public KeyEvent {
 public:
  KeyReleaseEvent(const application::inputs::Key& key, const application::inputs::Modifier modifier)
      : KeyEvent(key, modifier) {}
  EventType Type() const override { return EventType::KEY_RELEASE_EVENT; }
  std::string ToString() const override { return "KeyReleaseEvent : { " + StringInner() + " }"; }
};

/// @brief Event triggered when a key is repeated
class KeyRepeatEvent : public KeyEvent {
 public:
  KeyRepeatEvent(const application::inputs::Key& key, const application::inputs::Modifier modifier)
      : KeyEvent(key, modifier) {}
  EventType Type() const override { return EventType::KEY_REPEAT_EVENT; }
  std::string ToString() const override { return "KeyRepeatEvent : { " + StringInner() + " }"; }
};

/// @brief Event triggered when a character is entered
class CharEvent : public Event {
 public:
  explicit CharEvent(const unsigned int codepoint) : m_codepoint(codepoint) {}
  EventType Type() const override { return EventType::CHAR_EVENT; }
  std::string ToString() const override { return "CharEvent : { codepoint : " + std::to_string(m_codepoint) + " }"; }
  unsigned int GetCodepoint() const { return m_codepoint; }

 private:
  unsigned int m_codepoint;
};

}  // namespace events
}  // namespace core
}  // namespace nv3dvc

#endif  // SRC_CORE_EVENTS_KEYEVENT_H_
