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

#include "SceneHierarchyPanel.h"

#include <string>

#include "Core/EntityComponentSystem/Entity.h"
#include "Core/Util/Logger.h"
#include "imgui.h"

namespace nv3dvc {
namespace modules {
namespace guimodule {
namespace panels {

SceneHierarchyPanel::SceneHierarchyPanel() {}
SceneHierarchyPanel::~SceneHierarchyPanel() {}

void SceneHierarchyPanel::Render(core::ecs::registry::EntityRegistry* reg) {
  ImGui::Begin("Scene");

  // Render all root entities. Children will be rendered recursively
  auto view = reg->view<std::string>();
  for (auto entity : view) {
    if (!entity.GetParent().IsValid()) {
      RenderEntityNode(entity);
    }
  }

  ImGui::End();
}

void SceneHierarchyPanel::RenderEntityNode(core::ecs::Entity entity) {
  ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
  if (entity.GetChildren().size() == 0) flags |= ImGuiTreeNodeFlags_Leaf;
  if (m_selectedEntity == entity) flags |= ImGuiTreeNodeFlags_Selected;
  std::string& name = entity.GetComponent<std::string>();
  bool opened = ImGui::TreeNodeEx(reinterpret_cast<void*>((uint64_t)(uint32_t)entity), flags, "%s", name.c_str());
  if (ImGui::IsItemClicked()) {
    m_selectedEntity = entity;
  }
  if (opened) {
    for (core::ecs::Entity child : entity.GetChildren()) {
      RenderEntityNode(child);
    }
    ImGui::TreePop();
  }
}

void SceneHierarchyPanel::ClearSelectedEntity() { m_selectedEntity = core::ecs::Entity(); }

core::ecs::Entity SceneHierarchyPanel::GetSelectedEntity() const { return m_selectedEntity; }

}  // namespace panels
}  // namespace guimodule
}  // namespace modules
}  // namespace nv3dvc
