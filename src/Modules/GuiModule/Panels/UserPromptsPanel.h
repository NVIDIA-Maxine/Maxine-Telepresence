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

#ifndef SRC_MODULES_GUIMODULE_PANELS_USERPROMPTSPANEL_H_
#define SRC_MODULES_GUIMODULE_PANELS_USERPROMPTSPANEL_H_

#include "Core/Engine/Engine.h"
#include "imgui.h"

namespace nv3dvc {
namespace modules {
namespace guimodule {
namespace panels {

/// @brief Panel for displaying user prompts
///
/// Will display user prompts overlayed as text
class UserPromptsPanel {
 public:
  UserPromptsPanel() = default;

  /// @brief Render user prompts with enlarged text size
  /// Prompts are rendered with an undecorated transparent window with the effect of only showing text on the render
  /// buffer
  /// @return core::Error::SUCCESS if successful
  core::Error Render(core::engine::Engine* engine);

 private:
  ImVec2 m_panelPos = {0, 50};
  ImVec2 m_panelSize = {500, 500};
  float m_fontScale = 1.8f;
};

}  // namespace panels
}  // namespace guimodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_GUIMODULE_PANELS_USERPROMPTSPANEL_H_
