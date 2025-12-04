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

#ifndef SRC_CORE_SERIALIZATION_CUSTOMJSONTYPES_H_
#define SRC_CORE_SERIALIZATION_CUSTOMJSONTYPES_H_

#include "Core/Application/Inputs.h"
#include "Core/Util/Types.h"
#include "nlohmann/json.hpp"

namespace nv3dvc {
namespace core {
namespace application {
namespace inputs {

NLOHMANN_JSON_SERIALIZE_ENUM(Key, {{Key::UNKNOWN, "NONE"},
                                   {Key::SPACE, " "},
                                   {Key::APOSTROPHE, "'"},
                                   {Key::COMMA, ","},
                                   {Key::MINUS, "-"},
                                   {Key::PERIOD, "."},
                                   {Key::SLASH, "/"},
                                   {Key::KEY_0, "0"},
                                   {Key::KEY_1, "1"},
                                   {Key::KEY_2, "2"},
                                   {Key::KEY_3, "3"},
                                   {Key::KEY_4, "4"},
                                   {Key::KEY_5, "5"},
                                   {Key::KEY_6, "6"},
                                   {Key::KEY_7, "7"},
                                   {Key::KEY_8, "8"},
                                   {Key::KEY_9, "9"},
                                   {Key::SEMICOLON, ";"},
                                   {Key::EQUAL, "="},
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
                                   {Key::LEFT_BRACKET, "["},
                                   {Key::BACKSLASH, "\\"},
                                   {Key::RIGHT_BRACKET, "]"},
                                   {Key::GRAVE_ACCENT, "`"},
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
                                   {Key::LAST, "LAST"}});

NLOHMANN_JSON_SERIALIZE_ENUM(Modifier, {
                                           {Modifier::NONE, "NONE"},
                                           {Modifier::SHIFT, "SHIFT"},
                                           {Modifier::CONTROL, "CONTROL"},
                                           {Modifier::ALT, "ALT"},
                                           {Modifier::SUPER, "SUPER"},
                                           {Modifier::CAPS_LOCK, "CAPS_LOCK"},
                                           {Modifier::NUM_LOCK, "NUM_LOCK"},
                                       });

}  // namespace inputs
}  // namespace application
}  // namespace core
}  // namespace nv3dvc

namespace nv3dvc {
namespace core {
namespace util {

/// @brief Serialization of Trigger
/// @param j The destination json object
/// @param p The source Trigger
void to_json(nlohmann::json& j,  // NOLINT(runtime/references) (owned by nlohmann::json)
             const Trigger& p) {
  j = {{"Key", p.key}, {"Modifier", p.modifier}};
}

/// @brief Deserialization of Trigger
/// @param j The source json object
/// @param p The destination Trigger
void from_json(const nlohmann::json& j,  //
               Trigger& p) {             // NOLINT(runtime/references) (owned by nlohmann::json)
  core::application::inputs::Key key = core::application::inputs::Key::UNKNOWN;
  core::application::inputs::Modifier modifier = core::application::inputs::Modifier::NONE;
  if (j.contains("Key")) {
    key = j.at("Key");
  }
  if (j.contains("Modifier")) {
    modifier = j.at("Modifier");
  }
  p = {key, modifier};
}

}  // namespace util
}  // namespace core
}  // namespace nv3dvc

#endif  // SRC_CORE_SERIALIZATION_CUSTOMJSONTYPES_H_
