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

#include "BehaviorModule.h"

#include <iostream>

namespace nv3dvc {
namespace modules {
namespace behaviormodule {

BehaviorModule::BehaviorModule(core::application::inputs::Input* input) : m_input(input) {
  RegisterSystem<systems::BehaviorSystem>(this);
}

nv3dvc::core::Error BehaviorModule::Initialize() { return nv3dvc::core::SUCCESS; }

nv3dvc::core::Error BehaviorModule::Uninitialize() { return nv3dvc::core::SUCCESS; }

nv3dvc::core::Error BehaviorModule::OnUnloadScene(core::ecs::registry::EntityRegistry* registry) {
  for (const auto& [behavior_component_name, event_fun] : m_onUnloadSceneFunctions) {
    event_fun(registry);
  }
  return nv3dvc::core::SUCCESS;
}

void BehaviorModule::BroadcastEvent(core::ecs::registry::EntityRegistry* registry, core::events::Event* e) const {
  for (const auto& [behavior_component_name, event_fun] : m_onEventFunctions) {
    event_fun(registry, e);
  }
}

nv3dvc::core::Error BehaviorModule::UpdateEntities(core::ecs::registry::EntityRegistry* registry, const float dt) {
  core::Error err = nv3dvc::core::SUCCESS;
  for (const auto& [behavior_component_name, update_fun] : m_onUpdateFunctions) {
    core::Error err_tmp = update_fun(registry, dt);
    if (err == core::Error::SUCCESS) err = err_tmp;  // Save first error
  }
  return err;
}

}  // namespace behaviormodule
}  // namespace modules
}  // namespace nv3dvc
