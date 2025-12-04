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

#include "Inputs.h"

#include <string>
#include <unordered_map>

namespace nv3dvc {
namespace core {
namespace application {
namespace inputs {

static const std::unordered_map<JoystickHatState, const std::string> kJoystickHatStateStrMap = {
    {JoystickHatState::CENTERED, "CENTERED"},
    {JoystickHatState::UP, "UP"},
    {JoystickHatState::RIGHT, "RIGHT"},
    {JoystickHatState::DOWN, "DOWN"},
    {JoystickHatState::LEFT, "LEFT"},
    {JoystickHatState::RIGHT_UP, "RIGHT_UP"},
    {JoystickHatState::RIGHT_DOWN, "RIGHT_DOWN"},
    {JoystickHatState::LEFT_UP, "LEFT_UP"},
    {JoystickHatState::LEFT_DOWN, "LEFT_DOWN"},
};

static const std::unordered_map<Key, const std::string> kKeyStrMap = {
    {Key::UNKNOWN, "UNKNOWN"},
    {Key::SPACE, "SPACE"},
    {Key::APOSTROPHE, "APOSTROPHE"},
    {Key::COMMA, "COMMA"},
    {Key::MINUS, "MINUS"},
    {Key::PERIOD, "PERIOD"},
    {Key::SLASH, "SLASH"},
    {Key::KEY_0, "KEY_0"},
    {Key::KEY_1, "KEY_1"},
    {Key::KEY_2, "KEY_2"},
    {Key::KEY_3, "KEY_3"},
    {Key::KEY_4, "KEY_4"},
    {Key::KEY_5, "KEY_5"},
    {Key::KEY_6, "KEY_6"},
    {Key::KEY_7, "KEY_7"},
    {Key::KEY_8, "KEY_8"},
    {Key::KEY_9, "KEY_9"},
    {Key::SEMICOLON, "SEMICOLON"},
    {Key::EQUAL, "EQUAL"},
    {Key::A, "A"},
    {Key::B, "B"},
    {Key::C, "C"},
    {Key::D, "D"},
    {Key::E, "E"},
    {Key::F, "F"},
    {Key::G, "G"},
    {Key::H, "H"},
    {Key::I, "I"},
    {Key::J, "J"},
    {Key::K, "K"},
    {Key::L, "L"},
    {Key::M, "M"},
    {Key::N, "N"},
    {Key::O, "O"},
    {Key::P, "P"},
    {Key::Q, "Q"},
    {Key::R, "R"},
    {Key::S, "S"},
    {Key::T, "T"},
    {Key::U, "U"},
    {Key::V, "V"},
    {Key::W, "W"},
    {Key::X, "X"},
    {Key::Y, "Y"},
    {Key::Z, "Z"},
    {Key::LEFT_BRACKET, "LEFT_BRACKET"},
    {Key::BACKSLASH, "BACKSLASH"},
    {Key::RIGHT_BRACKET, "RIGHT_BRACKET"},
    {Key::GRAVE_ACCENT, "GRAVE_ACCENT"},
    {Key::WORLD_1, "WORLD_1"},
    {Key::WORLD_2, "WORLD_2"},
    {Key::ESCAPE, "ESCAPE"},
    {Key::ENTER, "ENTER"},
    {Key::TAB, "TAB"},
    {Key::BACKSPACE, "BACKSPACE"},
    {Key::INSRT, "INSRT"},
    {Key::DEL, "DEL"},
    {Key::RIGHT, "RIGHT"},
    {Key::LEFT, "LEFT"},
    {Key::DOWN, "DOWN"},
    {Key::UP, "UP"},
    {Key::PAGE_UP, "PAGE_UP"},
    {Key::PAGE_DOWN, "PAGE_DOWN"},
    {Key::HOME, "HOME"},
    {Key::END, "END"},
    {Key::CAPS_LOCK, "CAPS_LOCK"},
    {Key::SCROLL_LOCK, "SCROLL_LOCK"},
    {Key::NUM_LOCK, "NUM_LOCK"},
    {Key::PRINT_SCREEN, "PRINT_SCREEN"},
    {Key::PAUSE, "PAUSE"},
    {Key::F1, "F1"},
    {Key::F2, "F2"},
    {Key::F3, "F3"},
    {Key::F4, "F4"},
    {Key::F5, "F5"},
    {Key::F6, "F6"},
    {Key::F7, "F7"},
    {Key::F8, "F8"},
    {Key::F9, "F9"},
    {Key::F10, "F10"},
    {Key::F11, "F11"},
    {Key::F12, "F12"},
    {Key::F13, "F13"},
    {Key::F14, "F14"},
    {Key::F15, "F15"},
    {Key::F16, "F16"},
    {Key::F17, "F17"},
    {Key::F18, "F18"},
    {Key::F19, "F19"},
    {Key::F20, "F20"},
    {Key::F21, "F21"},
    {Key::F22, "F22"},
    {Key::F23, "F23"},
    {Key::F24, "F24"},
    {Key::F25, "F25"},
    {Key::KP_0, "KP_0"},
    {Key::KP_1, "KP_1"},
    {Key::KP_2, "KP_2"},
    {Key::KP_3, "KP_3"},
    {Key::KP_4, "KP_4"},
    {Key::KP_5, "KP_5"},
    {Key::KP_6, "KP_6"},
    {Key::KP_7, "KP_7"},
    {Key::KP_8, "KP_8"},
    {Key::KP_9, "KP_9"},
    {Key::KP_DECIMAL, "KP_DECIMAL"},
    {Key::KP_DIVIDE, "KP_DIVIDE"},
    {Key::KP_MULTIPLY, "KP_MULTIPLY"},
    {Key::KP_SUBTRACT, "KP_SUBTRACT"},
    {Key::KP_ADD, "KP_ADD"},
    {Key::KP_ENTER, "KP_ENTER"},
    {Key::KP_EQUAL, "KP_EQUAL"},
    {Key::LEFT_SHIFT, "LEFT_SHIFT"},
    {Key::LEFT_CONTROL, "LEFT_CONTROL"},
    {Key::LEFT_ALT, "LEFT_ALT"},
    {Key::LEFT_SUPER, "LEFT_SUPER"},
    {Key::RIGHT_SHIFT, "RIGHT_SHIFT"},
    {Key::RIGHT_CONTROL, "RIGHT_CONTROL"},
    {Key::RIGHT_ALT, "RIGHT_ALT"},
    {Key::RIGHT_SUPER, "RIGHT_SUPER"},
    {Key::MENU, "MENU"},
};

static const std::unordered_map<Modifier, const std::string> kModifierStrMap = {
    {Modifier::NONE, "NONE"},         {Modifier::SHIFT, "SHIFT"}, {Modifier::CONTROL, "CONTROL"},
    {Modifier::ALT, "ALT"},           {Modifier::SUPER, "SUPER"}, {Modifier::CAPS_LOCK, "CAPS_LOCK"},
    {Modifier::NUM_LOCK, "NUM_LOCK"},
};

static const std::unordered_map<Mouse, const std::string> kMouseStrMap = {
    {Mouse::BUTTON_1, "BUTTON_1"}, {Mouse::BUTTON_2, "BUTTON_2"}, {Mouse::BUTTON_3, "BUTTON_3"},
    {Mouse::BUTTON_4, "BUTTON_4"}, {Mouse::BUTTON_5, "BUTTON_5"}, {Mouse::BUTTON_6, "BUTTON_6"},
    {Mouse::BUTTON_7, "BUTTON_7"}, {Mouse::BUTTON_8, "BUTTON_8"},
};

static const std::unordered_map<Joystick, const std::string> kJoystickStrMap = {
    {Joystick::JOYSTICK_1, "JOYSTICK_1"},   {Joystick::JOYSTICK_2, "JOYSTICK_2"},
    {Joystick::JOYSTICK_3, "JOYSTICK_3"},   {Joystick::JOYSTICK_4, "JOYSTICK_4"},
    {Joystick::JOYSTICK_5, "JOYSTICK_5"},   {Joystick::JOYSTICK_6, "JOYSTICK_6"},
    {Joystick::JOYSTICK_7, "JOYSTICK_7"},   {Joystick::JOYSTICK_8, "JOYSTICK_8"},
    {Joystick::JOYSTICK_9, "JOYSTICK_9"},   {Joystick::JOYSTICK_10, "JOYSTICK_10"},
    {Joystick::JOYSTICK_11, "JOYSTICK_11"}, {Joystick::JOYSTICK_12, "JOYSTICK_12"},
    {Joystick::JOYSTICK_13, "JOYSTICK_13"}, {Joystick::JOYSTICK_14, "JOYSTICK_14"},
    {Joystick::JOYSTICK_15, "JOYSTICK_15"}, {Joystick::JOYSTICK_16, "JOYSTICK_16"},
};

static const std::unordered_map<GamepadButton, const std::string> kGamepadButtonStrMap = {
    {GamepadButton::A, "A"},
    {GamepadButton::B, "B"},
    {GamepadButton::X, "X"},
    {GamepadButton::Y, "Y"},
    {GamepadButton::LEFT_BUMPER, "LEFT_BUMPER"},
    {GamepadButton::RIGHT_BUMPER, "RIGHT_BUMPER"},
    {GamepadButton::BACK, "BACK"},
    {GamepadButton::START, "START"},
    {GamepadButton::GUIDE, "GUIDE"},
    {GamepadButton::LEFT_THUMB, "LEFT_THUMB"},
    {GamepadButton::RIGHT_THUMB, "RIGHT_THUMB"},
    {GamepadButton::DPAD_UP, "DPAD_UP"},
    {GamepadButton::DPAD_RIGHT, "DPAD_RIGHT"},
    {GamepadButton::DPAD_DOWN, "DPAD_DOWN"},
    {GamepadButton::DPAD_LEFT, "DPAD_LEFT"},
};

static const std::unordered_map<GamepadAxis, const std::string> kGamepadAxisStrMap = {
    {GamepadAxis::LEFT_X, "LEFT_X"},
    {GamepadAxis::LEFT_Y, "LEFT_Y"},
    {GamepadAxis::RIGHT_X, "RIGHT_X"},
    {GamepadAxis::RIGHT_Y, "RIGHT_Y"},
    {GamepadAxis::LEFT_TRIGGER, "LEFT_TRIGGER"},
    {GamepadAxis::RIGHT_TRIGGER, "RIGHT_TRIGGER"},
};

const std::string& ToString(JoystickHatState const& k) { return kJoystickHatStateStrMap.at(k); }

const std::string& ToString(Key const& k) { return kKeyStrMap.at(k); }

std::string ToString(Modifier const& m) {
  std::string res;
  if (m & Modifier::SHIFT) {
    res += kModifierStrMap.at(Modifier::SHIFT) + ", ";
  }
  if (m & Modifier::CONTROL) {
    res += kModifierStrMap.at(Modifier::CONTROL) + ", ";
  }
  if (m & Modifier::ALT) {
    res += kModifierStrMap.at(Modifier::ALT) + ", ";
  }
  if (m & Modifier::SUPER) {
    res += kModifierStrMap.at(Modifier::SUPER) + ", ";
  }
  if (m & Modifier::CAPS_LOCK) {
    res += kModifierStrMap.at(Modifier::CAPS_LOCK) + ", ";
  }
  if (m & Modifier::NUM_LOCK) {
    res += kModifierStrMap.at(Modifier::NUM_LOCK) + ", ";
  }
  if (res.empty()) {
    res = kModifierStrMap.at(Modifier::NONE);
  } else {
    res = res.substr(0, res.size() - 2);
  }
  return res;
}

const std::string& ToString(Mouse const& k) { return kMouseStrMap.at(k); }

const std::string& ToString(Joystick const& k) { return kJoystickStrMap.at(k); }

const std::string& ToString(GamepadButton const& k) { return kGamepadButtonStrMap.at(k); }

const std::string& ToString(GamepadAxis const& k) { return kGamepadAxisStrMap.at(k); }

void Input::AddKeyPressInputListener(const std::function<bool(Key)>& key_input_function) {
  m_keyInputFunctions.push_back(key_input_function);
}

void Input::AddMousePressInputListener(const std::function<bool(Mouse)>& mouse_input_function) {
  m_mouseInputFuncions.push_back(mouse_input_function);
}

void Input::AddCursorPositionInputListener(const std::function<glm::dvec2()>& cursor_input_function) {
  m_cursorInputFunctions.push_back(cursor_input_function);
}

bool Input::IsKeyPressed(const Key& key) const {
  bool res = false;
  for (const auto& fun : m_keyInputFunctions) {
    res = res || fun(key);
  }
  return res;
}

bool Input::IsMouseButtonPressed(const Mouse& mouse_button) const {
  bool res = false;
  for (const auto& fun : m_mouseInputFuncions) {
    res = res || fun(mouse_button);
  }
  return res;
}

glm::dvec2 Input::GetCursorPosition() const {
  glm::dvec2 res = {0.0, 0.0};
  if (!m_cursorInputFunctions.empty()) {
    // Just use the last one as multiple callbacks is not well defined
    res = m_cursorInputFunctions.back()();
  }
  return res;
}

}  // namespace inputs
}  // namespace application
}  // namespace core
}  // namespace nv3dvc
