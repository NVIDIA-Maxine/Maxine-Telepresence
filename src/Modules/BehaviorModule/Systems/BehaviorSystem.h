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

#ifndef SRC_MODULES_BEHAVIORMODULE_SYSTEMS_BEHAVIORSYSTEM_H_
#define SRC_MODULES_BEHAVIORMODULE_SYSTEMS_BEHAVIORSYSTEM_H_

#include <string>

#include "Core/EntityComponentSystem/System.h"
#include "Core/Properties/Property.h"

namespace nv3dvc {

namespace core {
namespace ecs {
namespace registry {
// Forward declaration
class EntityRegistry;
}  // namespace registry
}  // namespace ecs
}  // namespace core

namespace modules {
namespace behaviormodule {

// Forward declaration
class BehaviorModule;

namespace systems {

/// @defgroup BehaviorSystemProperties BehaviorSystem
/// @ingroup  SystemProperties
/// @brief    System for handling behavior components.
///
/// Behavior components have update functions which get called when running the system
/// Update functions for behavior components are executed within the BehaviorSystem's Run function as opposed to the
/// BehaviorModule's update function to enable control over when the components are updated, i.e. on the main thread or
/// other threads. See core::Application::RegisterMainThreadSystems, core::Application::RegisterThreadSystems
/// Typically, the behavior system is executed on the main thread.

/// See @ref BehaviorSystemProperties
class BehaviorSystem : public core::ecs::System {
 public:
  constexpr static const char* NAME = "BehaviorSystem";
  std::string Name() const override { return NAME; };

  /// @brief Create a behavior system instance given its owning module
  /// The behavior system requires a reference to the behavior module for updating all entities with attached behavior
  /// components in a registry using their update function.
  ///
  /// Usage within extension of BehaviorModule's constructor
  /// @code
  /// ...
  /// RegisterSystem<systems::BehaviorSystem>(this);
  /// ...
  /// @endcode
  /// @param behavior_module
  explicit BehaviorSystem(BehaviorModule* behavior_module);

  nv3dvc::core::Error Initialize() override;
  nv3dvc::core::Error OnEvent(core::ecs::registry::EntityRegistry* reg, core::events::Event* e) override;
  nv3dvc::core::Error OnLoadScene(core::ecs::registry::EntityRegistry* reg) override;
  nv3dvc::core::Error OnUnloadScene(core::ecs::registry::EntityRegistry* reg) override;

  nv3dvc::core::Error Run(core::ecs::registry::EntityRegistry* reg, float dt) override;

  nv3dvc::core::Error Uninitialize() override;

 private:
  BehaviorModule* m_behaviorModule;
};

}  // namespace systems
}  // namespace behaviormodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_BEHAVIORMODULE_SYSTEMS_BEHAVIORSYSTEM_H_
