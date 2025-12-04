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

#ifndef SRC_CORE_APPLICATION_INPUTS_H_
#define SRC_CORE_APPLICATION_INPUTS_H_

#include <functional>
#include <string>
#include <vector>

#include "glm/glm.hpp"

namespace nv3dvc {
namespace core {
namespace application {
namespace inputs {

/// @brief Joystick hat states that are supported for input
enum class JoystickHatState {
  CENTERED = 0,
  UP = 1,
  RIGHT = 2,
  DOWN = 4,
  LEFT = 8,
  RIGHT_UP = 2 | 1,
  RIGHT_DOWN = 2 | 4,
  LEFT_UP = 8 | 1,
  LEFT_DOWN = 8 | 4,
};

/// @brief Keyboard keys that are supported for input
enum class Key {
  UNKNOWN = -1,
  // Printable keys (corresponding to ASCII codes)
  SPACE = 32,
  APOSTROPHE = 39,  // '
  COMMA = 44,       // ,
  MINUS = 45,       // -
  PERIOD = 46,      // .
  SLASH = 47,       // /
  KEY_0 = 48,
  KEY_1 = 49,
  KEY_2 = 50,
  KEY_3 = 51,
  KEY_4 = 52,
  KEY_5 = 53,
  KEY_6 = 54,
  KEY_7 = 55,
  KEY_8 = 56,
  KEY_9 = 57,
  SEMICOLON = 59,  // ;
  EQUAL = 61,      // =
  A = 65,
  B = 66,
  C = 67,
  D = 68,
  E = 69,
  F = 70,
  G = 71,
  H = 72,
  I = 73,
  J = 74,
  K = 75,
  L = 76,
  M = 77,
  N = 78,
  O = 79,
  P = 80,
  Q = 81,
  R = 82,
  S = 83,
  T = 84,
  U = 85,
  V = 86,
  W = 87,
  X = 88,
  Y = 89,
  Z = 90,
  LEFT_BRACKET = 91,  // [
  BACKSLASH = 92,
  RIGHT_BRACKET = 93,  // ]
  GRAVE_ACCENT = 96,   // `
  WORLD_1 = 161,       // non-US #1
  WORLD_2 = 162,       // non-US #2
  // Function keys
  ESCAPE = 256,
  ENTER = 257,
  TAB = 258,
  BACKSPACE = 259,
  INSRT = 260,
  DEL = 261,
  RIGHT = 262,
  LEFT = 263,
  DOWN = 264,
  UP = 265,
  PAGE_UP = 266,
  PAGE_DOWN = 267,
  HOME = 268,
  END = 269,
  CAPS_LOCK = 280,
  SCROLL_LOCK = 281,
  NUM_LOCK = 282,
  PRINT_SCREEN = 283,
  PAUSE = 284,
  F1 = 290,
  F2 = 291,
  F3 = 292,
  F4 = 293,
  F5 = 294,
  F6 = 295,
  F7 = 296,
  F8 = 297,
  F9 = 298,
  F10 = 299,
  F11 = 300,
  F12 = 301,
  F13 = 302,
  F14 = 303,
  F15 = 304,
  F16 = 305,
  F17 = 306,
  F18 = 307,
  F19 = 308,
  F20 = 309,
  F21 = 310,
  F22 = 311,
  F23 = 312,
  F24 = 313,
  F25 = 314,
  KP_0 = 320,
  KP_1 = 321,
  KP_2 = 322,
  KP_3 = 323,
  KP_4 = 324,
  KP_5 = 325,
  KP_6 = 326,
  KP_7 = 327,
  KP_8 = 328,
  KP_9 = 329,
  KP_DECIMAL = 330,
  KP_DIVIDE = 331,
  KP_MULTIPLY = 332,
  KP_SUBTRACT = 333,
  KP_ADD = 334,
  KP_ENTER = 335,
  KP_EQUAL = 336,
  LEFT_SHIFT = 340,
  LEFT_CONTROL = 341,
  LEFT_ALT = 342,
  LEFT_SUPER = 343,
  RIGHT_SHIFT = 344,
  RIGHT_CONTROL = 345,
  RIGHT_ALT = 346,
  RIGHT_SUPER = 347,
  MENU = 348,
  LAST = 348,  // MENU
};

/// @brief Keyboard key modifiers that are supported for input
///
/// This is a bitfield so values can be combined and compared with bitwise OR and AND operators
enum Modifier {
  NONE = 0x0000,
  SHIFT = 0x0001,
  CONTROL = 0x0002,
  ALT = 0x0004,
  SUPER = 0x0008,
  CAPS_LOCK = 0x0010,
  NUM_LOCK = 0x0020,
};

/// @brief Bitwise OR operator for the Modifier enum
/// @param[in] a The first Modifier
/// @param[in] b The second Modifier
/// @return    The Modifier value that represents the union of the two input modifiers
inline Modifier operator|(Modifier a, Modifier b) {
  return static_cast<Modifier>(static_cast<unsigned int>(a) | static_cast<unsigned int>(b));
}

/// @brief Bitwise AND operator for the Modifier enum
/// @param[in] a The first Modifier
/// @param[in] b The second Modifier
/// @return    The Modifier value that represents the intersection of the two input modifiers
inline Modifier operator&(Modifier a, Modifier b) {
  return static_cast<Modifier>(static_cast<unsigned int>(a) & static_cast<unsigned int>(b));
}

/// @brief Bitwise AND assignment operator for the Modifier enum
/// @param[in] a The first Modifier, which will be assigned
/// @param[in] b The second Modifier
/// @return    A reference to the first Modifier, which will be set to `a & b`
inline Modifier& operator&=(Modifier& a, Modifier b) {  // NOLINT(runtime/references)
  unsigned int a_int = static_cast<unsigned int>(a);
  a_int &= static_cast<unsigned int>(b);
  a = static_cast<Modifier>(a_int);
  return a;
}

/// @brief Bitwise OR assignment operator for the Modifier enum
/// @param[in] a The first Modifier, which will be assigned
/// @param[in] b The second Modifier
/// @return    A reference to the first Modifier, which will be set to `a | b`
inline Modifier& operator|=(Modifier& a, Modifier b) {  // NOLINT(runtime/references)
  unsigned int a_int = static_cast<unsigned int>(a);
  a_int |= static_cast<unsigned int>(b);
  a = static_cast<Modifier>(a_int);
  return a;
}

/// @brief Mouse buttons that are supported for input
enum class Mouse {
  BUTTON_1 = 0,
  BUTTON_2 = 1,
  BUTTON_3 = 2,
  BUTTON_4 = 3,
  BUTTON_5 = 4,
  BUTTON_6 = 5,
  BUTTON_7 = 6,
  BUTTON_8 = 7,
  LAST = 7,    // BUTTON_8
  LEFT = 0,    // BUTTON_1
  RIGHT = 1,   // BUTTON_2
  MIDDLE = 2,  // BUTTON_3
};

/// @brief Joystick buttons that are supported for input
enum class Joystick {
  JOYSTICK_1 = 0,
  JOYSTICK_2 = 1,
  JOYSTICK_3 = 2,
  JOYSTICK_4 = 3,
  JOYSTICK_5 = 4,
  JOYSTICK_6 = 5,
  JOYSTICK_7 = 6,
  JOYSTICK_8 = 7,
  JOYSTICK_9 = 8,
  JOYSTICK_10 = 9,
  JOYSTICK_11 = 10,
  JOYSTICK_12 = 11,
  JOYSTICK_13 = 12,
  JOYSTICK_14 = 13,
  JOYSTICK_15 = 14,
  JOYSTICK_16 = 15,
  LAST = 15,  // JOYSTICK_16
};

/// @brief Gamepad buttons that are supported for input
enum class GamepadButton {
  A = 0,
  B = 1,
  X = 2,
  Y = 3,
  LEFT_BUMPER = 4,
  RIGHT_BUMPER = 5,
  BACK = 6,
  START = 7,
  GUIDE = 8,
  LEFT_THUMB = 9,
  RIGHT_THUMB = 10,
  DPAD_UP = 11,
  DPAD_RIGHT = 12,
  DPAD_DOWN = 13,
  DPAD_LEFT = 14,
  LAST = 14,     // DPAD_LEFT
  CROSS = 0,     // A
  CIRCLE = 1,    // B
  SQUARE = 2,    // X
  TRIANGLE = 4,  // Y
};

/// @brief Gamepad axes that are supported for input
enum class GamepadAxis {
  LEFT_X = 0,
  LEFT_Y = 1,
  RIGHT_X = 2,
  RIGHT_Y = 3,
  LEFT_TRIGGER = 4,
  RIGHT_TRIGGER = 5,
  LAST = 5,  // RIGHT_TRIGGER
};

/// @brief Generate a string representation of a JoystickHatState value
const std::string& ToString(JoystickHatState const& k);
/// @brief Generate a string representation of a Key value
const std::string& ToString(Key const& k);
/// @brief Generate a string representation of a Modifier value
std::string ToString(Modifier const& m);
/// @brief Generate a string representation of a Mouse value
const std::string& ToString(Mouse const& k);
/// @brief Generate a string representation of a Joystick value
const std::string& ToString(Joystick const& k);
/// @brief Generate a string representation of a GamepadButton value
const std::string& ToString(GamepadButton const& k);
/// @brief Generate a string representation of a GamepadAxis value
const std::string& ToString(GamepadAxis const& k);

/// @brief Class for handling inputs from multiple sources
class Input {
 public:
  /// @brief Add a function to be called to check whether a key is pressed
  /// @param[in] key_input_function function to call to determine whether a key is pressed
  void AddKeyPressInputListener(const std::function<bool(Key)>& key_input_function);

  /// @brief Add a function to be called to check whether a mouse button is pressed
  /// @param[in] mouse_input_function function to call to determine whether a mouse button is pressed
  void AddMousePressInputListener(const std::function<bool(Mouse)>& mouse_input_function);

  /// @brief Add a function to be called to check the mouse position
  /// @param[in] cursor_input_function function to call to determine the cursor position
  void AddCursorPositionInputListener(const std::function<glm::dvec2()>& cursor_input_function);

  /// @brief Check whether a key is pressed
  /// @param key the key to test
  /// @returns true if the key is pressed
  bool IsKeyPressed(const Key& key) const;

  /// @brief Check whether a mouse button is pressed
  /// @param mouse_button the mouse button to test
  /// @returns true if the mouse button is pressed
  bool IsMouseButtonPressed(const Mouse& mouse_button) const;

  /// @brief Get the cursor position within the current window
  /// @returns the coordinates {x, y} of the cursor
  glm::dvec2 GetCursorPosition() const;

 private:
  std::vector<std::function<bool(Key)>> m_keyInputFunctions;
  std::vector<std::function<bool(Mouse)>> m_mouseInputFuncions;
  std::vector<std::function<glm::dvec2()>> m_cursorInputFunctions;
};

}  // namespace inputs
}  // namespace application
}  // namespace core
}  // namespace nv3dvc

#endif  // SRC_CORE_APPLICATION_INPUTS_H_
