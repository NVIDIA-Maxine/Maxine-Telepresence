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

#include "EntityPropertiesPanel.h"

#include <imgui.h>

#include <string>

#include "Core/EntityComponentSystem/Entity.h"
#include "RenderProperties.h"

namespace nv3dvc {
namespace modules {
namespace guimodule {
namespace panels {

void EntityPropertiesPanel::Render(core::ecs::Entity entity) {
  if (!entity.HasComponent<std::string>() || !entity.HasComponent<core::properties::PropertyOwner>()) return;
  const auto& name = entity.GetComponent<std::string>();
  auto& property_owner = entity.GetComponent<core::properties::PropertyOwner>();
  // Window label will be `name`, but window ID will be `EntityProperties`, so it retains size / position.
  ImGui::Begin((name + "###EntityProperties").c_str());
  RenderProperties("Properties", &property_owner);
  ImGui::End();
}

}  // namespace panels
}  // namespace guimodule
}  // namespace modules
}  // namespace nv3dvc
