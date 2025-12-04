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

#ifndef SRC_MODULES_TRIPLANEMODULE_TRIPLANEMODULE_H_
#define SRC_MODULES_TRIPLANEMODULE_TRIPLANEMODULE_H_

#include <string>

#include "Modules/BehaviorModule/BehaviorModule.h"

// Export components and systems
#include "Components/TriplaneBufferComponent.h"
#include "Components/TriplaneLoaderBehaviorComponent.h"
#include "Components/TriplaneVolumeComponent.h"

namespace nv3dvc {
namespace modules {
namespace triplanemodule {

/// @defgroup TriplaneModuleProperties TriplaneModule
/// @ingroup ModuleProperties
/// @brief Behavior module defining triplane specific components
///
/// Registers the following components:
/// - nv3dvc::modules::triplanemodule::components::TriplaneBufferComponent
/// - nv3dvc::modules::triplanemodule::components::TriplaneLoaderBehaviorComponent
/// - nv3dvc::modules::triplanemodule::components::TriplaneVolumeComponent

/// See @ref TriplaneModuleProperties
class TriplaneModule : public core::engine::Module {
 public:
  constexpr static const char* NAME = "TriplaneModule";
  std::string Name() const override { return NAME; };

  explicit TriplaneModule(behaviormodule::BehaviorModule* behavior_module);

  nv3dvc::core::Error Initialize() override;

  nv3dvc::core::Error Uninitialize() override;

  nv3dvc::core::Error OnLoadScene(core::ecs::registry::EntityRegistry* reg) override;

  nv3dvc::core::Error Update(float dt) override { return nv3dvc::core::SUCCESS; }
};

}  // namespace triplanemodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_TRIPLANEMODULE_TRIPLANEMODULE_H_
