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

#include "Module.h"

#include <string>

#include "core/Util/Logger.h"

namespace nv3dvc {
namespace core {
namespace engine {

bool Module::ComponentIsRegisteredInModule(const std::string& component_name) const {
  return m_onAttachFunctions.find(component_name) != m_onAttachFunctions.end();
}

bool Module::EntityHasComponent(const std::string& component_name, const ecs::Entity* entity) const {
  return ComponentIsRegisteredInModule(component_name) && m_onCheckHasComponentFunctions.at(component_name)(entity);
}

bool Module::TryAttachComponentInModule(const std::string& component_name, ecs::Entity* entity) const {
  if (ComponentIsRegisteredInModule(component_name)) {
    try {
      m_onAttachFunctions.at(component_name)(entity);
    } catch (const std::exception& e) {
      LOG_ERROR("Failed to attach component {} in module with error {}", component_name, e.what());
      return false;
    }
    return true;
  }
  return false;
}

const util::TypeMap<std::shared_ptr<ecs::System>>& Module::SystemTypeMap() const { return m_systemsTypeMap; }

}  // namespace engine
}  // namespace core
}  // namespace nv3dvc
