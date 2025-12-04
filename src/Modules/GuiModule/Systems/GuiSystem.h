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

#ifndef SRC_MODULES_GUIMODULE_SYSTEMS_GUISYSTEM_H_
#define SRC_MODULES_GUIMODULE_SYSTEMS_GUISYSTEM_H_

#include <string>

#include "Core/Engine/Engine.h"
#include "Core/EntityComponentSystem/Entity.h"
#include "Core/EntityComponentSystem/System.h"
#include "Core/Events/KeyEvent.h"
#include "Core/Events/MouseEvent.h"
#include "Core/Events/WindowEvent.h"
#include "Modules/GuiModule/Panels/ApplicationConfigPanel.h"
#include "Modules/GuiModule/Panels/EntityPropertiesPanel.h"
#include "Modules/GuiModule/Panels/MainMenuBar.h"
#include "Modules/GuiModule/Panels/SceneHierarchyPanel.h"
#include "Modules/GuiModule/Panels/UserPromptsPanel.h"
#include "Modules/GuiModule/Panels/VideoPanel.h"
#include "Modules/GuiModule/Panels/WindowDisplayOptions.h"

namespace nv3dvc {
namespace modules {
namespace guimodule {
namespace systems {

/// @defgroup GuiSystemProperties GuiSystem
/// @ingroup  SystemProperties
/// @brief    System for displaying and enabling the user to control properties in the application
///
/// The GUI displays the following panels:
/// * panels::MainMenuBar
/// * panels::ApplicationConfigPanel
/// * panels::SceneHierarchyPanel
/// * panels::EntityPropertiesPanel
/// * panels::VideoPanel
/// * panels::UserPromptsPanel

/// See @ref GuiSystemProperties
class GuiSystem : public core::ecs::System {
 public:
  constexpr static const char* NAME = "GuiSystem";
  std::string Name() const override { return NAME; }

  /// @brief Constructor
  /// @param[in,out] engine A reference to the engine, which properties can be modified using the GUI
  explicit GuiSystem(core::engine::Engine* engine);

  ~GuiSystem() override = default;

  core::Error Initialize() override;
  core::Error Uninitialize() override;

  core::Error OnLoadScene(core::ecs::registry::EntityRegistry* reg) override;
  core::Error OnUnloadScene(core::ecs::registry::EntityRegistry* reg) override;

  core::Error Run(core::ecs::registry::EntityRegistry* reg, float dt) override;
  core::Error OnEvent(core::ecs::registry::EntityRegistry* reg, core::events::Event* e) override;

 public:
  /// @defgroup GuiSystemProperties GuiSystem
  /// @ingroup SystemProperties
  /// @{
  core::properties::Property<float> ui_content_scale{
      this,
      "ui_content_scale",
      "Scaling factor for UI content. For stereo displays, with a lower perceived resolution, a larger content scaling "
      "of 2 may be desired.",
      1.0f,
  };
  /// @}

 private:
  bool OnMouseButtonPressed(core::events::MouseButtonPressEvent* e);
  bool OnMouseButtonReleased(core::events::MouseButtonReleaseEvent* e);
  bool OnMouseMoved(core::events::CursorPositionEvent* e);
  bool OnMouseScrolled(core::events::ScrollEvent* e);
  bool OnKeyPressed(core::events::KeyPressEvent* e);
  bool OnKeyReleased(core::events::KeyReleaseEvent* e);
  bool OnKeyTyped(core::events::CharEvent* e);
  bool OnWindowResized(core::events::WindowSizeEvent* e);
  bool OnFrameBufferResized(core::events::FramebufferSizeEvent* e);
  bool OnWindowContentScaleUpdate(core::events::WindowContentScaleEvent* e);

  /// @brief Update dependent values based on the current value of ui_content_scale
  void UpdateContentScale();

  core::engine::Engine* m_engine;

  // Panels
  panels::MainMenuBar m_mainMenuBar;
  panels::ApplicationConfigPanel m_appConfigPanel;
  panels::SceneHierarchyPanel m_sceneHierarchyPanel;
  panels::EntityPropertiesPanel m_entityPropertiesPanel;
  panels::VideoPanel m_videoPanel;
  panels::UserPromptsPanel m_userPromptsPanel;

  // Display options
  panels::WindowDisplayOptions m_windowDisplayOptions;
  glm::vec2 m_windowContentScale;
  glm::ivec2 m_windowSize;
  glm::ivec2 m_frameBufferSize;
  glm::vec2 m_contentScale;
};

}  // namespace systems
}  // namespace guimodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_GUIMODULE_SYSTEMS_GUISYSTEM_H_
