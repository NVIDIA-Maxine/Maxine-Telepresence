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

#ifndef SRC_CORE_ENGINE_MODULE_H_
#define SRC_CORE_ENGINE_MODULE_H_

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "Core/EntityComponentSystem/Entity.h"
#include "Core/EntityComponentSystem/System.h"
#include "Core/Error.h"
#include "nlohmann/json.hpp"

namespace nv3dvc {
namespace core {
namespace engine {

/// @brief Module base class
///
/// Extend this class for registering new component types to be attached to entities.
/// The module class allows for attaching or detaching entity components given their names, to be used during
/// deserialization
///
/// Sample extension of module:
/// \snippet src/Samples/MyModule/MyModule.h Simple module sample
class Module : public properties::PropertyOwner {
 public:
  /// @brief Register all the module's components in the constructor
  Module() = default;
  ~Module() override {}

  /// @brief Initialize the module
  ///
  /// Load any internal dependencies during initialization. The module's properties will have been loaded before
  /// initialization. Therefore properties can be used to initialize the module based on how it was configured through
  /// properties
  /// @return true if successful
  virtual nv3dvc::core::Error Initialize() = 0;

  /// @brief Uninitialize the module
  ///
  /// Unload any internal dependencies during uninitialization.
  /// @return true if successful
  virtual nv3dvc::core::Error Uninitialize() = 0;

  /// @brief Startup of the module called once per application run
  ///
  /// Gets called after initialization, and after the scene has been loaded, but before Update
  /// Typically used to initialize module specific components
  /// @param[in,out] reg The registry of entities existing in the scene
  virtual nv3dvc::core::Error OnLoadScene(ecs::registry::EntityRegistry* reg) { return nv3dvc::core::SUCCESS; }

  virtual nv3dvc::core::Error OnUnloadScene(ecs::registry::EntityRegistry* reg) { return nv3dvc::core::SUCCESS; }

  /// @brief Update the state of the module
  ///
  /// Typically, only used for static module functionality. Not all modules need to update.
  /// @param dt Deltatime. Time spent since last update
  /// @return ture if successful
  virtual nv3dvc::core::Error Update(float dt) { return nv3dvc::core::SUCCESS; }

  /// @brief Encode any special property types that are not defined in the list of supported types for serialization
  ///
  /// Usage:
  /// @code{.cpp}
  /// nv3dvc::core::Error MyModule::EncodeProperties(nlohmann::json* json_description, const PropertyOwner*
  /// property_owner) const override {
  ///   core::serialization::EncodeProperties<MyTypeCustomType1>(json_description, property_owner);
  ///   core::serialization::EncodeProperties<MyTypeCustomType2>(json_description, property_owner);
  ///   ...
  ///   return nv3dvc::core::SUCCESS;
  /// }
  /// @endcode
  /// When using the function `core::serialization::EncodeProperties<T>`, ensure that the `to_json` serialization
  /// function is defined for that type `T`.
  /// @param[in,out] json_description Serialized description of component that may have special types added as property
  /// @param[in]     property_owner   The property owner corresponding to the component which may have special types
  ///                                 added as property
  /// @return        SUCCESS          If successful
  virtual nv3dvc::core::Error EncodeProperties(nlohmann::json* json_description,
                                               const PropertyOwner* property_owner) const {
    return nv3dvc::core::SUCCESS;
  }

  /// @brief Decode any special property types that are not defined in the list of supported types for serialization
  ///
  /// Usage:
  /// @code{.cpp}
  /// nv3dvc::core::Error MyModule::DecodeProperties(const nlohmann::json& json_description, PropertyOwner*
  /// property_owner) const override {
  ///   core::serialization::DecodeProperties<MyTypeCustomType1>(json_description, property_owner);
  ///   core::serialization::DecodeProperties<MyTypeCustomType2>(json_description, property_owner);
  ///   ...
  ///   return nv3dvc::core::SUCCESS;
  /// }
  /// @endcode
  /// When using the function `core::serialization::DecodeProperties<T>`, ensure that the `from_json` serialization
  /// function is defined for that type `T`.
  /// @param[in]     json_description Serialized description of component that may have special types added as property
  /// @param[in,out] property_owner   The property owner corresponding to the component which may have special types
  ///                                 added as property
  /// @return        SUCCESS          If successful
  virtual nv3dvc::core::Error DecodeProperties(const nlohmann::json& json_description,
                                               PropertyOwner* property_owner) const {
    return nv3dvc::core::SUCCESS;
  }

  /// @brief Whether a component of a specific name is already registered in the module
  /// @param[in] component_name The name of the component
  /// @return    true           if the component of this name is already registered in the module
  bool ComponentIsRegisteredInModule(const std::string& component_name) const;

  /// @brief Whether an entity has a component of the given name already attached
  /// @param[in] component_name The name of the component
  /// @param[in] entity         The entity
  /// @return    true           if the entity already has a component of this name attached
  bool EntityHasComponent(const std::string& component_name, const ecs::Entity* entity) const;

  /// @brief Try to attach a component of a given name to the provided entity
  ///
  /// This function relies on the component of this name being previously registered in this module. This function can
  /// be used during deserialization, where a component name exists
  /// @param[in] component_name The name of the component type
  /// @param[in] entity         The entity to attach the given component to
  /// @return    true           if the component was attached
  bool TryAttachComponentInModule(const std::string& component_name, ecs::Entity* entity) const;

  /// @brief Get a constant reference to the typemap of all systems registered in the module
  /// @return A constant reference to the typemap of all systems registered in the module
  const util::TypeMap<std::shared_ptr<ecs::System>>& SystemTypeMap() const;

 protected:
  /// @brief Register a component type to this module
  ///
  /// After a component type has been registered, it can be serialized, and deserialized when reading and writing the
  /// scene containing entities with this component type attached
  /// @tparam     ComponentType The type of the component to register
  /// @tparam     ...ArgTypes   Forward args types for component constructor
  /// @param[in]  ...args       Forward args for component constructor
  template <class ComponentType, typename... ArgTypes>
  nv3dvc::core::Error RegisterComponent(ArgTypes&&... args);

  /// @brief Registere a system type to this module
  ///
  /// A system object will be created with this function call.
  /// After a systmem has been registered, it can be fetched using SystemTypeMap and executed from an engine
  /// @tparam     SystemType  The type of the system to register. Should be a subclass of core::ecs::System
  /// @tparam     ...ArgTypes Forward args types for system constructor
  /// @param[in]  ...args     Forward args for system constructor
  template <class SystemType, typename... ArgTypes>
  nv3dvc::core::Error RegisterSystem(ArgTypes&&... args);

 private:
  // Mapping from name to function specific to the component type using that name
  std::unordered_map<std::string, std::function<void(ecs::Entity* entity)>> m_onAttachFunctions;
  std::unordered_map<std::string, std::function<void(ecs::Entity* entity)>> m_onDetachFunctions;
  std::unordered_map<std::string, std::function<bool(const ecs::Entity* entity)>> m_onCheckHasComponentFunctions;
  // Internal storage of system objects
  util::TypeMap<std::shared_ptr<ecs::System>> m_systemsTypeMap;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Template function definitions                                                                                      //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <class ComponentType, typename... ArgTypes>
nv3dvc::core::Error Module::RegisterComponent(ArgTypes&&... args) {
  // Make sure this component is not already registered
  assert(m_onAttachFunctions.find(ComponentType::NAME) == m_onAttachFunctions.end());
  m_onAttachFunctions.insert({ComponentType::NAME, [&args...](ecs::Entity* entity) {
                                entity->AddComponent<ComponentType>(std::forward<ArgTypes>(args)...);
                              }});
  m_onDetachFunctions.insert(
      {ComponentType::NAME, [](ecs::Entity* entity) { entity->RemoveComponent<ComponentType>(); }});
  m_onCheckHasComponentFunctions.insert(
      {ComponentType::NAME, [](const ecs::Entity* entity) { return entity->HasComponent<ComponentType>(); }});
  return nv3dvc::core::SUCCESS;
}

template <class SystemType, typename... ArgTypes>
nv3dvc::core::Error Module::RegisterSystem(ArgTypes&&... args) {
  assert(m_systemsTypeMap.find<SystemType>() == m_systemsTypeMap.end());
  m_systemsTypeMap.put<SystemType>(std::make_shared<SystemType>(std::forward<ArgTypes>(args)...));
  return nv3dvc::core::SUCCESS;
}

}  // namespace engine
}  // namespace core
}  // namespace nv3dvc

#endif  // SRC_CORE_ENGINE_MODULE_H_
