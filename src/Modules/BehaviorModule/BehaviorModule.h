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

#ifndef SRC_MODULES_BEHAVIORMODULE_BEHAVIORMODULE_H_
#define SRC_MODULES_BEHAVIORMODULE_BEHAVIORMODULE_H_

#include <functional>
#include <string>
#include <unordered_map>

#include "Core/Application/Inputs.h"
#include "Core/Engine/Module.h"
#include "Core/EntityComponentSystem/Entity.h"
#include "Core/Properties/Property.h"

// Export components and systems
#include "Components/BehaviorComponent.h"
#include "Systems/BehaviorSystem.h"

namespace nv3dvc {
namespace modules {
namespace behaviormodule {

/// @defgroup BehaviorModuleProperties BehaviorModule
/// @ingroup  ModuleProperties
/// @brief    Module for handling all behavior components
///
/// Registers the system: systems::BehaviorSystem
/// Any module which registers their own behavior components (extensions of components::BehaviorComponent) needs to
/// register the behavior system with the module pointer as constructor input. see systems::BehaviorSystem.

/// See @ref BehaviorModuleProperties
class BehaviorModule : public core::engine::Module {
 public:
  constexpr static const char* NAME = "BehaviorModule";
  std::string Name() const override { return NAME; };

  /// @brief Constructor
  /// @param input Application input
  explicit BehaviorModule(core::application::inputs::Input* input);

  nv3dvc::core::Error Initialize() override;
  nv3dvc::core::Error Uninitialize() override;
  nv3dvc::core::Error OnUnloadScene(core::ecs::registry::EntityRegistry* reg) override;

  /// @brief Broadcast an event to all behavior components registered in this module
  /// This will call the OnEvent functions for each of the behavior components previously registered to this module
  /// @param[in]     reg The registry where the registered behavior components are attached to entities
  /// @param[in,out] e   The event to broadcast
  void BroadcastEvent(core::ecs::registry::EntityRegistry* reg, core::events::Event* e) const;

  /// @brief Update all entities with any of the registered behavior components attached
  /// @param[in] reg  The registry where the registered behavior components are attached to entities
  /// @param[in] dt   Delta time. Time tick since last invocation
  /// @return    true If successful
  nv3dvc::core::Error UpdateEntities(core::ecs::registry::EntityRegistry* reg, float dt);

  /// @brief Register a given behavior component type to the module
  /// A component type must be registered to the behavior module before being added to objects in the scene for update
  /// functions and event broadcasting to be called for the behavior component. This is also required for the scene
  /// serialization to take the behavior component into account.
  /// @tparam BehaviorComponentType Should be a type extending the class BehaviorComponent
  template <class BehaviorComponentType>
  nv3dvc::core::Error RegisterBehaviorComponent();

 private:
  core::application::inputs::Input* m_input;
  // Update functions allows for calling the update function on all registered behavior components
  std::unordered_map<std::string, std::function<core::Error(core::ecs::registry::EntityRegistry* reg, float dt)>>
      m_onUpdateFunctions;
  // On event functions allows for broadcasting events to all registered behavior components
  std::unordered_map<std::string,
                     std::function<core::Error(core::ecs::registry::EntityRegistry* reg, core::events::Event* e)>>
      m_onEventFunctions;

  std::unordered_map<std::string, std::function<core::Error(core::ecs::registry::EntityRegistry* reg)>>
      m_onUnloadSceneFunctions;
};

template <class BehaviorComponentType>
nv3dvc::core::Error BehaviorModule::RegisterBehaviorComponent() {
  // First register to set up the standard attachment functions
  RegisterComponent<BehaviorComponentType>();

  // Then set up the update attachment function
  m_onUpdateFunctions.insert(  //
      {BehaviorComponentType::NAME, [this](core::ecs::registry::EntityRegistry* reg, float dt) {
         core::Error err = core::Error::SUCCESS;
         auto view = reg->view<BehaviorComponentType>();
         for (auto& entity : view) {
           auto& component = entity.GetComponent<BehaviorComponentType>();
           core::Error err_tmp = core::Error::SUCCESS;
           if (!component.IsInitialized()) {
             err_tmp = component.Initialize(entity, m_input);
             if (err == core::Error::SUCCESS) err = err_tmp;  // Save first err
             err = component.OnInitialize();
             if (err == core::Error::SUCCESS) err = err_tmp;  // Save first err
           }
           err = component.OnUpdate(dt);
           if (err == core::Error::SUCCESS) err = err_tmp;  // Save first err
         }
         return err;
       }});

  // And event functions
  m_onEventFunctions.insert(  //
      {BehaviorComponentType::NAME, [](core::ecs::registry::EntityRegistry* reg, core::events::Event* e) {
         core::Error err = core::Error::SUCCESS;
         auto view = reg->view<BehaviorComponentType>();
         for (auto& entity : view) {
           core::Error err_tmp = entity.GetComponent<BehaviorComponentType>().OnEvent(e);
           if (err == core::Error::SUCCESS) err = err_tmp;  // Save first err
         }
         return err;
       }});

  // And shutdown functions
  m_onUnloadSceneFunctions.insert(  //
      {BehaviorComponentType::NAME, [this](core::ecs::registry::EntityRegistry* reg) {
         core::Error err = core::Error::SUCCESS;
         auto view = reg->view<BehaviorComponentType>();
         for (auto& entity : view) {
           core::Error err_tmp = entity.GetComponent<BehaviorComponentType>().OnUnloadScene();
           if (err == core::Error::SUCCESS) err = err_tmp;  // Save first err
         }
         return err;
       }});

  return nv3dvc::core::SUCCESS;
}

}  // namespace behaviormodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_BEHAVIORMODULE_BEHAVIORMODULE_H_
