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

#ifndef SRC_MODULES_BEHAVIORMODULE_COMPONENTS_BEHAVIORCOMPONENT_H_
#define SRC_MODULES_BEHAVIORMODULE_COMPONENTS_BEHAVIORCOMPONENT_H_

#include <string>
#include <utility>

#include "Core/Application/Inputs.h"
#include "Core/Engine/Engine.h"
#include "Core/EntityComponentSystem/Component.h"
#include "Core/EntityComponentSystem/Entity.h"

namespace nv3dvc {

namespace core {
namespace events {
class Event;
}  // namespace events
}  // namespace core

namespace modules {
namespace behaviormodule {
namespace components {

/// @brief Base class for behavior components
/// A behavior component has all the functionality of a regular component extension, but with additional behavior
/// implemented with update functions and events. A behavior component has a reference to the entity to which it is
/// attached, making it possible to reference other components already attached to the entity, or to add new components
/// to it programmatically. See BehaviorComponent::HasComponent(), BehaviorComponent::GetComponent,
/// BehaviorComponent::AddComponent. An extension to the behavior component also has callbacks for events, and a
/// reference to user input, which can be used to control the component in its update function. Behavior component's
/// update function is generally called from the BehaviorSystems Run function
///
/// Sample extension of behavior:
/// \snippet src/Samples/MyModule/MyModule.h Simple behavior sample
class BehaviorComponent : public core::ecs::Component {
 public:
  constexpr static const char* NAME = "BehaviorComponent";
  std::string Name() const override { return NAME; };

  /// @brief Create an invalid behavior component.
  /// The behavior component needs to be initialized to become valid
  BehaviorComponent() = default;
  BehaviorComponent(const BehaviorComponent& other) = default;
  BehaviorComponent(BehaviorComponent&& other) noexcept = default;
  ~BehaviorComponent() override = default;
  BehaviorComponent& operator=(const BehaviorComponent& other) = default;
  BehaviorComponent& operator=(BehaviorComponent&& other) noexcept = default;

  /// @brief Initialize the behavior component
  /// As long as the behavior component is registered to a behavior module, it will get initialized implicitly,
  /// whereupon OnInitialize will get called.
  /// @param[in] entity A reference to the entity owning this component
  /// @param[in] input  A reference to general user input. Must not be nullptr
  /// @return core::Error::SUCCESS If successful
  core::Error Initialize(const core::ecs::Entity& entity, core::application::inputs::Input* input);

  /// @brief True if the component has been successfully initialized
  /// @return true If the component has been successfully initialized
  bool IsInitialized() const;

  /// @brief A pointer to the user input
  /// @return A pointer to the user input
  core::application::inputs::Input* Input();

  /// @brief Check whether the entity owning this behavior component has a component of type ComponentType attached
  /// @tparam ComponentType The type of the component to check
  /// @return true If the owning entity has this component type attached
  template <typename ComponentType>
  bool HasComponent() const;

  /// @brief Get an attached component of type ComponentType
  /// Assumes HasComponent<ComponentType>() == true
  /// @tparam ComponentType The type of the component to fetch
  /// @return The attached component
  template <typename ComponentType>
  ComponentType& GetComponent();

  /// @brief Attach a component to the entity. The component's constructor will be called with ...ArgTypes
  /// @tparam ComponentType Component type
  /// @tparam ArgTypes      Template argument types
  /// @param  args          Forward args
  /// @return Reference to the created component
  template <typename ComponentType, typename... ArgTypes>
  ComponentType& AddComponent(ArgTypes&&... args);

  /// @brief Utility function for getting the global transform if there is any
  /// @return The global transform
  ///         A unit matrix if no transform component exists on this entity or its parent
  glm::mat4 GetGlobalTransform() const;

  /// @brief Calls trigger's onChange function if it is to be triggered using the event
  /// If any Trigger properties are attached to this component and the event matches their condition for triggering,
  /// onChange functions will be called
  /// @param e The event to trigger the onChange function
  /// @return core::Error:SUCCESS if successful
  core::Error HandleTriggers(core::events::Event* e);

  /// @brief Callback for when an event has been dispatched
  /// @param[in,out] e The event
  /// @return core::Error:SUCCESS if successful
  virtual core::Error OnEvent(core::events::Event* e) { return core::SUCCESS; }

  /// @brief Callback for updating the behavior component
  /// @param[in] dt Delta time. Time since last invocation.
  /// @return core::Error:SUCCESS if successful
  virtual core::Error OnUpdate(float dt) { return core::SUCCESS; }

  /// @brief Callback for when the behavior component has been successfully initialized
  /// @return core::Error:SUCCESS if successful
  virtual core::Error OnInitialize() { return core::SUCCESS; }

  /// @brief Callback for when the scene is unloaded
  /// @return core::Error::SUCCESS if successful
  virtual core::Error OnUnloadScene() { return core::Error::SUCCESS; }

 private:
  bool m_isInitialized = false;
  core::ecs::Entity m_entity = {};
  core::application::inputs::Input* m_input = nullptr;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Template function definitions                                                                                      //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename ComponentType>
bool BehaviorComponent::HasComponent() const {
  return m_entity.HasComponent<ComponentType>();
}

template <typename ComponentType>
ComponentType& BehaviorComponent::GetComponent() {
  return m_entity.GetComponent<ComponentType>();
}

template <typename ComponentType, typename... ArgTypes>
ComponentType& BehaviorComponent::AddComponent(ArgTypes&&... args) {
  return m_entity.AddComponent<ComponentType>(std::forward<ArgTypes>(args)...);
}

}  // namespace components
}  // namespace behaviormodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_BEHAVIORMODULE_COMPONENTS_BEHAVIORCOMPONENT_H_
