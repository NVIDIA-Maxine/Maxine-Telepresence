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

#ifndef SRC_MODULES_RENDERMODULE_RENDERMODULE_H_
#define SRC_MODULES_RENDERMODULE_RENDERMODULE_H_

#include <string>
#include <vector>

#include "Core/Engine/Module.h"

// Export components and systems
#include "Components/CubeMapComponent.h"
#include "Components/DisplayComponent.h"
#include "Components/RenderableTriplaneComponent.h"
#include "Components/StereoViewComponent.h"
#include "Components/TexturedQuadComponent.h"
#include "Core/Rendering/Display/VirtualDisplay.h"
#include "Systems/RenderSystem.h"

namespace nv3dvc {
namespace modules {
namespace rendermodule {

/// @defgroup RenderModuleProperties RenderModule
/// @ingroup  ModuleProperties
/// @brief    Module responsible for rendering and handling renderable objects
///
/// Registers the following system:
/// - systems::RenderSystem
///
/// Registers the following components:
/// - components::CubeMapComponent
/// - components::DisplayComponent
/// - components::RenderableTriplaneComponent
/// - components::StereoViewComponent

/// @see RenderModuleProperties
class RenderModule : public core::engine::Module {
 public:
  constexpr static const char* NAME = "RenderModule";
  std::string Name() const override { return NAME; };

  explicit RenderModule(core::engine::Engine* engine);
  ~RenderModule();

  nv3dvc::core::Error Initialize() override;
  nv3dvc::core::Error Uninitialize() override;
  nv3dvc::core::Error OnLoadScene(core::ecs::registry::EntityRegistry* reg) override;
  nv3dvc::core::Error Update(float dt) override;
  nv3dvc::core::Error EncodeProperties(nlohmann::json* json_description,
                                       const PropertyOwner* property_owner) const override;
  nv3dvc::core::Error DecodeProperties(const nlohmann::json& json_description,
                                       PropertyOwner* property_owner) const override;

 private:
  core::engine::Engine* m_engine;
};

}  // namespace rendermodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_RENDERMODULE_RENDERMODULE_H_
