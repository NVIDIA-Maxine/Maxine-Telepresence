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

#include "BehaviorSystem.h"

#include "Core/EntityComponentSystem/Entity.h"
#include "Modules/BehaviorModule/BehaviorModule.h"
#include "Modules/BehaviorModule/Components/BehaviorComponent.h"

namespace nv3dvc {
namespace modules {
namespace behaviormodule {
namespace systems {

BehaviorSystem::BehaviorSystem(BehaviorModule* behavior_module) : m_behaviorModule(behavior_module) {}

nv3dvc::core::Error BehaviorSystem::Initialize() { return nv3dvc::core::SUCCESS; }

nv3dvc::core::Error BehaviorSystem::OnEvent(core::ecs::registry::EntityRegistry* reg, core::events::Event* e) {
  m_behaviorModule->BroadcastEvent(reg, e);
  return nv3dvc::core::SUCCESS;
}

nv3dvc::core::Error BehaviorSystem::OnLoadScene(core::ecs::registry::EntityRegistry* reg) {
  return nv3dvc::core::SUCCESS;
}

nv3dvc::core::Error BehaviorSystem::OnUnloadScene(core::ecs::registry::EntityRegistry* reg) {
  return nv3dvc::core::SUCCESS;
}

nv3dvc::core::Error BehaviorSystem::Uninitialize() { return nv3dvc::core::SUCCESS; }

nv3dvc::core::Error BehaviorSystem::Run(core::ecs::registry::EntityRegistry* reg, const float dt) {
  nv3dvc::core::Error err = nv3dvc::core::ERR_REGISTRY;
  if (reg) err = m_behaviorModule->UpdateEntities(reg, dt);
  return err;
}

}  // namespace systems
}  // namespace behaviormodule
}  // namespace modules
}  // namespace nv3dvc
