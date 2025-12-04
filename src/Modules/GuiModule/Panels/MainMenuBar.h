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

#ifndef SRC_MODULES_GUIMODULE_PANELS_MAINMENUBAR_H_
#define SRC_MODULES_GUIMODULE_PANELS_MAINMENUBAR_H_

#include "Core/Engine/Engine.h"
#include "Core/EntityComponentSystem/Entity.h"
#include "Core/Events/KeyEvent.h"
#include "WindowDisplayOptions.h"

namespace nv3dvc {
namespace modules {
namespace guimodule {
namespace panels {

/// @brief Main menu bar GUI element
///
/// Will render a main menu bar at the top of the window enabling file options and display options
class MainMenuBar {
 public:
  MainMenuBar();
  ~MainMenuBar();

  /// @brief Render the main menu bar
  /// @param[in]     selected_entity        The currently selected entity
  /// @param[in,out] window_display_options Display options. Allowed to change based on user input
  /// @param[in,out] engine                 The engine. Allowed to read or write files based on user input
  void Render(core::ecs::Entity selected_entity, WindowDisplayOptions* window_display_options,
              core::engine::Engine* engine);

  /// @brief Callback function for when a key is pressed
  ///
  /// Allows Ctrl+S, Ctrl+O etc, for file I/O
  /// @param[in]     e      The key press event
  /// @param[in,out] engine The engine. Allowed to read or write files based on user input
  void OnKeyPressed(const core::events::KeyPressEvent& e, core::engine::Engine* engine);

 private:
  void ActionSaveApplicationConfig(core::engine::Engine* engine);
  void ActionSaveApplicationConfigAs(core::engine::Engine* engine);
  void ActionEmptyScene(core::engine::Engine* engine);
  void ActionLoadDefaultScene(core::engine::Engine* engine);
  void ActionLoadScene(core::engine::Engine* engine);
  void ActionSaveScene(core::engine::Engine* engine);
  void ActionSaveSceneAs(core::engine::Engine* engine);
  void ActionQuit(core::engine::Engine* engine);

  void ShowFileMenu(core::engine::Engine* engine);
  void ShowWindowMenu(WindowDisplayOptions* window_display_options, core::ecs::Entity selected_entity);
};

}  // namespace panels
}  // namespace guimodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_GUIMODULE_PANELS_MAINMENUBAR_H_
