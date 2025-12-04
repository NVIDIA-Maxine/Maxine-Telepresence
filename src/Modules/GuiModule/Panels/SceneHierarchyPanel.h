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

#ifndef SRC_MODULES_GUIMODULE_PANELS_SCENEHIERARCHYPANEL_H_
#define SRC_MODULES_GUIMODULE_PANELS_SCENEHIERARCHYPANEL_H_

#include "Core/EntityComponentSystem/Entity.h"

namespace nv3dvc {
namespace modules {
namespace guimodule {
namespace panels {

/// @brief Panel for displaying the scene hierarchy
///
/// Will display A tree structure of the scene hierarchy where an entity can be selected. The selected entity's
/// properties will be shown in the EntityPropertiesPanel
class SceneHierarchyPanel {
 public:
  SceneHierarchyPanel();
  ~SceneHierarchyPanel();

  /// @brief Render the scene hierarchy
  /// @param[in,out] reg The registry of entitities defining the scene hierarchy
  void Render(core::ecs::registry::EntityRegistry* reg);

  /// @brief Get the currently user selected entity
  /// @return The selected entity
  ///         An invalid entity, if no entity is selected
  core::ecs::Entity GetSelectedEntity() const;

  /// @brief Removes reference to the currently selected entity
  void ClearSelectedEntity();

 private:
  /// @brief Helper function for rendering the entity node recursively
  /// @param[in] entity The base entity to render
  void RenderEntityNode(core::ecs::Entity entity);

  core::ecs::Entity m_selectedEntity;
};

}  // namespace panels
}  // namespace guimodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_GUIMODULE_PANELS_SCENEHIERARCHYPANEL_H_
