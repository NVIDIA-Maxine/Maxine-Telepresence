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

#include "WindowModule.h"

#include "Core/Application/Inputs.h"
#include "Core/Engine/Engine.h"
#include "Core/EntityComponentSystem/Entity.h"
#include "Core/Events/Event.h"
#include "Core/Events/WindowEvent.h"
#include "Core/Serialization/Serialization.h"

namespace nv3dvc {
namespace core {
namespace application {
NLOHMANN_JSON_SERIALIZE_ENUM(Window::WindowMode, {{Window::WindowMode::FULL_SCREEN, "FULL_SCREEN"},
                                                  {Window::WindowMode::HIDDEN, "HIDDEN"},
                                                  {Window::WindowMode::UNDECORATED, "UNDECORATED"},
                                                  {Window::WindowMode::WINDOWED, "WINDOWED"}});
}  // namespace application
}  // namespace core
}  // namespace nv3dvc

namespace nv3dvc {
namespace modules {
namespace windowmodule {

WindowModule::WindowModule(core::engine::Engine* engine) : m_engine(engine) {}

core::Error WindowModule::Initialize() {
  if (!m_engine) {
    LOG_WARNING("Engine not provided to WindowModule. No input listener or event callback to set.");
  }
  const std::function event_callback = [&](core::events::Event* e) {
    if (m_engine) m_engine->EventCallback(e);
  };
  core::Error res = m_window.Initialize(window_width, window_height, "Window", window_mode, enforce_window_resolution,
                                        event_callback);
  // May have been resized if m_enforceWindowResolution was set to false
  glm::uvec2 window_size = m_window.WindowSize();
  window_width = window_size.x;
  window_height = window_size.y;
  window_width.SetOnChangeFunction([this]() {
    if (enforce_window_resolution) {
      m_window.SetWindowSize({window_width, window_height});
    } else {
      *window_width.get() = m_window.WindowSize().x;  // Bypass the on-change-function to avoid infinite recursion
    }
  });
  window_height.SetOnChangeFunction([this]() {
    if (enforce_window_resolution) {
      m_window.SetWindowSize({window_width, window_height});
    } else {
      *window_height.get() = m_window.WindowSize().y;  // Bypass the on-change-function to avoid infinite recursion
    }
  });
  const std::function key_input_function = [&](core::application::inputs::Key key) {
    return m_window.IsKeyPressed(key);
  };
  const std::function mouse_input_function = [&](core::application::inputs::Mouse mouse_button) {
    return m_window.IsMouseButtonPressed(mouse_button);
  };
  const std::function cursor_input_function = [&] { return m_window.GetCursorPosition(); };

  if (m_engine) {
    m_engine->Input().AddKeyPressInputListener(key_input_function);
    m_engine->Input().AddMousePressInputListener(mouse_input_function);
    m_engine->Input().AddCursorPositionInputListener(cursor_input_function);
  }
  if (enable_vsync) {
    m_window.EnableVsync();
  } else {
    m_window.DisableVsync();
  }
  enable_vsync.SetOnChangeFunction([this]() { enable_vsync ? m_window.EnableVsync() : m_window.DisableVsync(); });

  return res;
}

core::Error WindowModule::Uninitialize() { return m_window.Uninitialize(); }

core::Error WindowModule::OnLoadScene(core::ecs::registry::EntityRegistry* reg) {
  core::Error err = core::Error::SUCCESS;
  glm::vec2 content_scale;
  glm::ivec2 framebuffer_size;
  for (auto entity : reg->view<rendermodule::components::DisplayComponent>()) {
    auto* display = entity.GetComponent<rendermodule::components::DisplayComponent>().GetDisplayPtr();
    int pos_x = 0;
    int pos_y = 0;
    int width = 0;
    int height = 0;
    if (display && display->GetDesiredWindowPositionAndSize(&pos_x, &pos_y, &width, &height)) {
      err = m_window.SetWindowPosition({pos_x, pos_y});
      BAIL_IF_ERR(err);
      err = m_window.SetWindowSize({width, height});
      BAIL_IF_ERR(err);
      window_width = width;
      window_height = height;
    }
  }

  // Signal to anyone who may need the window size after initialization
  if (m_engine) {
    m_engine->QueueEvent<core::events::WindowSizeEvent>(window_width, window_height);
    content_scale = m_window.WindowContentScale();
    m_engine->QueueEvent<core::events::WindowContentScaleEvent>(content_scale.x, content_scale.y);
    framebuffer_size = m_window.FramebufferSize();
    m_engine->QueueEvent<core::events::FramebufferSizeEvent>(framebuffer_size.x, framebuffer_size.y);
  }
bail:
  return err;
}

core::Error WindowModule::Update(float dt) {
  m_window.Update();
  // Update window size values without triggering OnChange callback.
  glm::uvec2 window_size = m_window.WindowSize();
  *window_width.get() = window_size.x;
  *window_height.get() = window_size.y;
  if (m_window.ShouldClose() && m_engine) {
    m_engine->Control().Queue<core::engine::command::Close>();
  }

  return core::SUCCESS;
}

core::Error WindowModule::EncodeProperties(nlohmann::json* json_description,
                                           const PropertyOwner* property_owner) const {
  core::Error err = core::Error::SUCCESS;
  CHECK_SUCCESS(
      core::serialization::EncodeProperties<core::application::Window::WindowMode>(json_description, property_owner));
bail:
  return err;
}

core::Error WindowModule::DecodeProperties(const nlohmann::json& json_description,
                                           PropertyOwner* property_owner) const {
  core::Error err = core::Error::SUCCESS;
  CHECK_SUCCESS(
      core::serialization::DecodeProperties<core::application::Window::WindowMode>(json_description, property_owner));
bail:
  return err;
}

}  // namespace windowmodule
}  // namespace modules
}  // namespace nv3dvc
