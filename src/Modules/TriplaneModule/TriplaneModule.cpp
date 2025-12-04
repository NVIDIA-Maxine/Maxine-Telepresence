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

#include "TriplaneModule.h"

#include <iostream>

#include "Modules/BehaviorModule/Systems/BehaviorSystem.h"

namespace nv3dvc {
namespace modules {
namespace triplanemodule {

TriplaneModule::TriplaneModule(behaviormodule::BehaviorModule* behavior_module) {
  RegisterComponent<components::TriplaneVolumeComponent>();
  RegisterComponent<components::TriplaneBufferComponent>();
  behavior_module->RegisterBehaviorComponent<components::TriplaneLoaderBehaviorComponent>();
}

nv3dvc::core::Error TriplaneModule::Initialize() { return nv3dvc::core::SUCCESS; }

nv3dvc::core::Error TriplaneModule::Uninitialize() { return nv3dvc::core::SUCCESS; }

nv3dvc::core::Error TriplaneModule::OnLoadScene(core::ecs::registry::EntityRegistry* reg) {
  for (auto& entity : reg->view<components::TriplaneBufferComponent>()) {
    auto& triplane_buffer = entity.GetComponent<components::TriplaneBufferComponent>();
    triplane_buffer.Initialize();
  }
  return core::Error::SUCCESS;
}

}  // namespace triplanemodule
}  // namespace modules
}  // namespace nv3dvc
