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

#include "BehaviorComponent.h"

#include "Core/Engine/Engine.h"
#include "Core/Events/KeyEvent.h"
#include "Core/Util/Types.h"
#include "Modules/CommonModule/CommonModule.h"

namespace nv3dvc {
namespace modules {
namespace behaviormodule {
namespace components {

core::Error BehaviorComponent::Initialize(const core::ecs::Entity& entity, core::application::inputs::Input* input) {
  m_entity = entity;
  m_input = input;
  if (m_entity.IsValid()) {
    m_isInitialized = true;
  } else {
    m_isInitialized = false;
  }
  return m_isInitialized ? core::Error::SUCCESS : core::Error::ERR_INITIALIZATION;
}

bool BehaviorComponent::IsInitialized() const { return m_isInitialized; }

core::application::inputs::Input* BehaviorComponent::Input() { return m_input; }

glm::mat4 BehaviorComponent::GetGlobalTransform() const { return commonmodule::GetGlobalTransform(m_entity); }

core::Error BehaviorComponent::HandleTriggers(core::events::Event* e) {
  core::Error err = core::Error::SUCCESS;
  CHECK_NONNULL(e, core::Error::ERR_NULL_POINTER);
  if (e->Type() == core::events::EventType::KEY_PRESS_EVENT) {
    auto& evt = e->As<core::events::KeyPressEvent>();
    for (auto* trigger : GetProperties<core::util::Trigger>()) {
      if (trigger->get()->key == evt.GetKey() && trigger->get()->modifier == evt.GetModifier()) {
        trigger->OnChange();
      }
    }
  }
bail:
  return err;
}

}  // namespace components
}  // namespace behaviormodule
}  // namespace modules
}  // namespace nv3dvc
