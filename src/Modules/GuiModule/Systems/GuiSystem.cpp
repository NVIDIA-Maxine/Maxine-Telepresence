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

#include "GuiSystem.h"

#include <imgui.h>

#include "Modules/GuiModule/ImGuiBackend.h"

namespace nv3dvc {
namespace modules {
namespace guimodule {
namespace systems {

GuiSystem::GuiSystem(core::engine::Engine* engine)
    : m_engine(engine),
      m_windowContentScale(1.0f, 1.0f),
      m_windowSize(0, 0),
      m_frameBufferSize(0, 0),
      m_contentScale(1.0f, 1.0f) {}

nv3dvc::core::Error GuiSystem::Initialize() {
  core::Error err = core::Error::SUCCESS;
  ImGuiContext* ctx = ImGui::CreateContext();
  ImGui::SetCurrentContext(ctx);
  ImGuiIO& io = ImGui::GetIO();
  CHECK_NONNULL(m_engine, core::Error::ERR_NULL_POINTER, "GuiSystem requires Engine");
  ImGui::StyleColorsDark();
  io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
  io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
  io.DisplayFramebufferScale = {1.0f, 1.0f};
  ui_content_scale.SetOnChangeFunction([this]() { UpdateContentScale(); });
  imgui_backend::ImGuiInitialize();
bail:
  return err;
}

nv3dvc::core::Error GuiSystem::Uninitialize() {
  imgui_backend::ImGuiUninitialize();
  return nv3dvc::core::SUCCESS;
}

core::Error GuiSystem::OnLoadScene(core::ecs::registry::EntityRegistry* reg) {
  return m_videoPanel.Initialize(reg, GetStream());
}

core::Error GuiSystem::OnUnloadScene(core::ecs::registry::EntityRegistry* reg) {
  return m_videoPanel.UnInitialize(reg, GetStream());
}

nv3dvc::core::Error GuiSystem::Run(core::ecs::registry::EntityRegistry* reg, const float dt) {
  core::Error err = core::Error::SUCCESS;
  ImGuiIO& io = ImGui::GetIO();
  io.DeltaTime = dt;
  imgui_backend::ImGuiNewFrame();
  ImGui::NewFrame();

  core::ecs::Entity selected_entity = m_sceneHierarchyPanel.GetSelectedEntity();
  m_mainMenuBar.Render(selected_entity, &m_windowDisplayOptions, m_engine);
  if (m_windowDisplayOptions.show_user_prompts) {
    err = m_userPromptsPanel.Render(m_engine);
    if (err != core::Error::SUCCESS) {
      LOG_ERROR("Error rendering user prompts: %s", core::ErrorStringFromCode(err));
    }
  }
  if (m_windowDisplayOptions.show_video_panel) {
    err = m_videoPanel.Render(reg, GetStream());
    if (err != core::Error::SUCCESS) {
      LOG_ERROR("Error rendering user prompts: %s", core::ErrorStringFromCode(err));
    }
  } else {
    m_videoPanel.ConfigDoNotUpdate(reg);
  }
  if (m_windowDisplayOptions.show_app_config_panel) {
    m_appConfigPanel.Render(m_engine, dt);
  }
  if (m_windowDisplayOptions.show_scene_hierarchy_panel) {
    m_sceneHierarchyPanel.Render(reg);
  }
  if (m_windowDisplayOptions.show_entity_properties_panel && selected_entity.IsValid() &&
      selected_entity.HasComponent<core::properties::PropertyOwner>()) {
    m_entityPropertiesPanel.Render(selected_entity);
  }

  ImGui::Render();
  imgui_backend::ImGuiRenderDrawData(ImGui::GetDrawData());
bail:
  return err;
}

nv3dvc::core::Error GuiSystem::OnEvent(core::ecs::registry::EntityRegistry* reg, core::events::Event* e) {
  switch (e->Type()) {
    case core::events::EventType::WINDOW_SIZE_EVENT: {
      OnWindowResized(&e->As<core::events::WindowSizeEvent>());
      break;
    }
    case core::events::EventType::FRAMEBUFFER_SIZE_EVENT: {
      OnFrameBufferResized(&e->As<core::events::FramebufferSizeEvent>());
      break;
    }
    case core::events::EventType::WINDOW_CONTENT_SCALE_EVENT: {
      OnWindowContentScaleUpdate(&e->As<core::events::WindowContentScaleEvent>());
      break;
    }
    case core::events::EventType::KEY_PRESS_EVENT: {
      OnKeyPressed(&e->As<core::events::KeyPressEvent>());
      break;
    }
    case core::events::EventType::KEY_RELEASE_EVENT: {
      OnKeyReleased(&e->As<core::events::KeyReleaseEvent>());
      break;
    }
    case core::events::EventType::CHAR_EVENT: {
      OnKeyTyped(&e->As<core::events::CharEvent>());
      break;
    }
    case core::events::EventType::CURSOR_POSITION_EVENT: {
      OnMouseMoved(&e->As<core::events::CursorPositionEvent>());
      break;
    }
    case core::events::EventType::SCROLL_EVENT: {
      OnMouseScrolled(&e->As<core::events::ScrollEvent>());
      break;
    }
    case core::events::EventType::MOUSE_BUTTON_PRESS_EVENT: {
      OnMouseButtonPressed(&e->As<core::events::MouseButtonPressEvent>());
      break;
    }
    case core::events::EventType::MOUSE_BUTTON_RELEASE_EVENT: {
      OnMouseButtonReleased(&e->As<core::events::MouseButtonReleaseEvent>());
      break;
    }
    default:
      break;
  }
  return nv3dvc::core::SUCCESS;
}

bool GuiSystem::OnMouseButtonPressed(core::events::MouseButtonPressEvent* e) {
  if (IsPaused()) return false;
  ImGuiIO& io = ImGui::GetIO();
  io.MouseDown[imgui_backend::ImGuiMapMouseButton(e->GetButton())] = true;
  if (io.WantCaptureMouse) e->Consume();
  return false;
}

bool GuiSystem::OnMouseButtonReleased(core::events::MouseButtonReleaseEvent* e) {
  if (IsPaused()) return false;
  ImGuiIO& io = ImGui::GetIO();
  io.MouseDown[imgui_backend::ImGuiMapMouseButton(e->GetButton())] = false;
  if (io.WantCaptureMouse) e->Consume();
  return false;
}

bool GuiSystem::OnMouseMoved(core::events::CursorPositionEvent* e) {
  if (IsPaused()) return false;
  ImGuiIO& io = ImGui::GetIO();
  io.MousePos = ImVec2(e->GetXposition() / m_contentScale.x, e->GetYposition() / m_contentScale.y);
  if (io.WantCaptureMouse) e->Consume();
  return false;
}

bool GuiSystem::OnMouseScrolled(core::events::ScrollEvent* e) {
  if (IsPaused()) return false;
  ImGuiIO& io = ImGui::GetIO();
  io.MouseWheelH = e->GetXoffset();
  io.MouseWheel = e->GetYoffset();
  if (io.WantCaptureMouse) e->Consume();
  return false;
}

bool GuiSystem::OnKeyPressed(core::events::KeyPressEvent* e) {
  core::application::inputs::Key key = e->GetKey();
  if (key == core::application::inputs::Key::F11) {
    // Toggle GUI
    SetPaused(!IsPaused());
  }
  if (IsPaused()) return false;
  ImGuiIO& io = ImGui::GetIO();

  if (e->GetKey() != core::application::inputs::Key::UNKNOWN) {
    io.KeysDown[imgui_backend::ImGuiMapKey(key)] = true;
    if (io.WantCaptureKeyboard) e->Consume();
  }
  io.KeyCtrl = io.KeysDown[imgui_backend::ImGuiMapKey(core::application::inputs::Key::LEFT_CONTROL)] ||
               io.KeysDown[imgui_backend::ImGuiMapKey(core::application::inputs::Key::RIGHT_CONTROL)];
  io.KeyShift = io.KeysDown[imgui_backend::ImGuiMapKey(core::application::inputs::Key::LEFT_SHIFT)] ||
                io.KeysDown[imgui_backend::ImGuiMapKey(core::application::inputs::Key::RIGHT_SHIFT)];
  io.KeyAlt = io.KeysDown[imgui_backend::ImGuiMapKey(core::application::inputs::Key::LEFT_ALT)] ||
              io.KeysDown[imgui_backend::ImGuiMapKey(core::application::inputs::Key::RIGHT_ALT)];
  io.KeySuper = io.KeysDown[imgui_backend::ImGuiMapKey(core::application::inputs::Key::LEFT_SUPER)] ||
                io.KeysDown[imgui_backend::ImGuiMapKey(core::application::inputs::Key::RIGHT_SUPER)];
  m_mainMenuBar.OnKeyPressed(*e, m_engine);
  return false;
}

bool GuiSystem::OnKeyReleased(core::events::KeyReleaseEvent* e) {
  if (IsPaused()) return false;
  ImGuiIO& io = ImGui::GetIO();
  if (e->GetKey() != core::application::inputs::Key::UNKNOWN) {
    io.KeysDown[imgui_backend::ImGuiMapKey(e->GetKey())] = false;
    if (io.WantCaptureKeyboard) e->Consume();
  }
  return false;
}

bool GuiSystem::OnKeyTyped(core::events::CharEvent* e) {
  if (IsPaused()) return false;
  ImGuiIO& io = ImGui::GetIO();
  int keycode = e->GetCodepoint();
  if (keycode > 0 && keycode < 127) {
    io.AddInputCharacter(static_cast<uint16_t>(keycode));
  }
  if (io.WantCaptureKeyboard) e->Consume();
  return false;
}

bool GuiSystem::OnWindowResized(core::events::WindowSizeEvent* e) {
  m_windowSize = {e->GetXsize(), e->GetYsize()};
  ImGuiIO& io = ImGui::GetIO();
  io.DisplaySize = ImVec2(m_windowSize.x / m_contentScale.x, m_windowSize.y / m_contentScale.y);
  io.DisplayFramebufferScale = ImVec2(m_windowSize.x / static_cast<float>(m_frameBufferSize.x) * m_contentScale.x,
                                      m_windowSize.y / static_cast<float>(m_frameBufferSize.y) * m_contentScale.y);
  return false;
}

bool GuiSystem::OnFrameBufferResized(core::events::FramebufferSizeEvent* e) {
  m_frameBufferSize = {e->GetWidth(), e->GetHeight()};
  ImGuiIO& io = ImGui::GetIO();
  io.DisplayFramebufferScale = ImVec2(m_windowSize.x / static_cast<float>(m_frameBufferSize.x) * m_contentScale.x,
                                      m_windowSize.y / static_cast<float>(m_frameBufferSize.y) * m_contentScale.y);
  return false;
}

bool GuiSystem::OnWindowContentScaleUpdate(core::events::WindowContentScaleEvent* e) {
  m_windowContentScale = {e->GetXscale(), e->GetYscale()};
  UpdateContentScale();
  return true;
}

void GuiSystem::UpdateContentScale() {
  // Set reasonable limits on UI scale, without triggering OnChange.
  *ui_content_scale.get() = glm::clamp(*ui_content_scale.get(), 0.5f, 2.0f);
  // Combine UI content scale and window content scale.
  m_contentScale = float{ui_content_scale} * m_windowContentScale;
  ImGuiIO& io = ImGui::GetIO();
  io.DisplaySize = ImVec2(m_windowSize.x / m_contentScale.x, m_windowSize.y / m_contentScale.y);
  io.DisplayFramebufferScale = ImVec2(m_windowSize.x / static_cast<float>(m_frameBufferSize.x) * m_contentScale.x,
                                      m_windowSize.y / static_cast<float>(m_frameBufferSize.y) * m_contentScale.y);
}

}  // namespace systems
}  // namespace guimodule
}  // namespace modules
}  // namespace nv3dvc
