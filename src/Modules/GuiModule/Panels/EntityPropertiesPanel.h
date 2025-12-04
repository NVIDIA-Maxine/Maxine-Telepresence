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

#ifndef SRC_MODULES_GUIMODULE_PANELS_ENTITYPROPERTIESPANEL_H_
#define SRC_MODULES_GUIMODULE_PANELS_ENTITYPROPERTIESPANEL_H_
#include "Core/EntityComponentSystem/Entity.h"

namespace nv3dvc {
namespace modules {
namespace guimodule {
namespace panels {

/// @brief Panel for displaying all properties of an entity
class EntityPropertiesPanel {
 public:
  /// @brief Render all the properties and subowners of an entity
  /// @param entity The entity. It's properties are allowed to change based on user input
  void Render(core::ecs::Entity entity);
};

}  // namespace panels
}  // namespace guimodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_GUIMODULE_PANELS_ENTITYPROPERTIESPANEL_H_
