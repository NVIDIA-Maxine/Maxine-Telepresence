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

#include "ApplicationControlSystem.h"

#include <filesystem>

#include "Core/Events/FileEvent.h"
#include "Core/Events/KeyEvent.h"

namespace nv3dvc {
namespace modules {
namespace controlmodule {
namespace systems {

ApplicationControlSystem::ApplicationControlSystem(core::engine::EngineControl* engine_control)
    : m_engineControl(engine_control) {}

core::Error ApplicationControlSystem::Initialize() {
  core::Error err = core::Error::SUCCESS;
  CHECK_NONNULL(m_engineControl, core::Error::ERR_NULL_POINTER, "ApplicationControlSystem requires EngineControl");
bail:
  return err;
}

core::Error ApplicationControlSystem::OnEvent(core::ecs::registry::EntityRegistry* reg, core::events::Event* e) {
  switch (e->Type()) {
    case core::events::EventType::KEY_PRESS_EVENT: {
      auto& key_press_event = e->As<core::events::KeyPressEvent>();
      if (key_press_event.GetKey() == core::application::inputs::Key::ESCAPE) {
        m_engineControl->Queue<core::engine::command::Close>();
      }
      break;
    }
    case core::events::EventType::FILE_EVENT: {
      auto& file_event = e->As<core::events::FileEvent>();
      const std::string file_path = file_event.GetFilePath();
      const auto extension = std::filesystem::path(file_path).extension();
      if (extension == ".json" || extension == ".scene") {
        m_engineControl->Queue<core::engine::command::LoadScene>(file_path);
      }
      break;
    }
    default:
      break;
  }
  return core::Error::SUCCESS;
}

core::Error ApplicationControlSystem::Run(core::ecs::registry::EntityRegistry* reg, float dt) {
  return core::Error::SUCCESS;
}

}  // namespace systems
}  // namespace controlmodule
}  // namespace modules
}  // namespace nv3dvc
