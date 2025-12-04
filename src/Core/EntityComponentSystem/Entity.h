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

#ifndef SRC_CORE_ENTITYCOMPONENTSYSTEM_ENTITY_H_
#define SRC_CORE_ENTITYCOMPONENTSYSTEM_ENTITY_H_

#include <mutex>  // NOLINT(build/c++11) (cpplint outside Chromium: https://github.com/google/styleguide/issues/483)
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "Core/Properties/Property.h"
#include "entt/entt.hpp"

namespace nv3dvc {
namespace core {
/// @brief Entity-Component-System namespace
namespace ecs {

namespace registry {
// Forward declaration
class EntityRegistry;
}  // namespace registry

/// @brief Entity used by the entity component system design pattern
class Entity {
 public:
  /// @brief Default constructor
  ///
  /// Creates an invalid entity that does not belong to any registry. An invalid entity can not be used, but a default
  /// constructor is handy
  Entity() = default;

  /// @brief Copy constructor
  /// @param other Other entity
  Entity(const Entity& other) = default;

  /// @brief Attach a component to the entity. The component's constructor will be called with ...ArgTypes
  /// @tparam ComponentType Component type
  /// @tparam ...ArgTypes   Template arg types
  /// @param  ...args       Forward args
  /// @return               Reference to the created component
  template <typename ComponentType, typename... ArgTypes>
  ComponentType& AddComponent(ArgTypes&&... args);

  /// @brief Get component of type ComponentType
  /// @tparam ComponentType Component type
  /// @return               The component
  template <typename ComponentType>
  ComponentType& GetComponent() const;

  /// @brief Remove component of type ComponentType
  /// @tparam ComponentType Component type
  template <typename ComponentType>
  void RemoveComponent();

  /// @brief Check if the entity has a component of type ComponentType
  /// @tparam T Component type
  /// @return True if the entity has a component of type ComponentType
  template <typename ComponentType>
  bool HasComponent() const;

  /// @brief Check if the entity is valid. I.e. if it exists in the registry
  /// @return
  bool IsValid() const;

  /// @brief True if valid
  explicit operator bool() const;

  /// @brief The ID of the entity
  explicit operator const uint32_t() const;

  /// @brief The Internal entt handle
  explicit operator const entt::entity() const;

  /// @brief Equality operator
  /// @param other Other entity
  /// @return True if equal, otherwise false
  bool operator==(const Entity& other) const;

  /// @brief Inverse equality operator
  /// @param other Other entity
  /// @return False if equal, otherwise true
  bool operator!=(const Entity& other) const;

  /// @brief Get the registry this entity exists in
  /// A valid entity can not exist on its own.
  /// @return The registry associated with this entity
  registry::EntityRegistry* Registry() const;

  /// @brief Returns true if the entity has a valid parent
  /// @return true if the entity has a valid parent
  bool HasParent();

  /// @brief Get the parent of the entity
  ///
  /// IsValid() of the parent will be true if this entity has a parent
  /// @return An entity which is the parent of this entity.
  Entity GetParent();

  /// @brief Get a list of all the child entities of this entity
  /// @return An std::vector of entities. Empty if entity does not have any children
  std::vector<Entity> GetChildren();

  /// @brief Set the parent of this entity
  ///
  /// An entity does not need to have a parent
  /// @param[in] parent The parent entity
  /// @return    true   If successful
  ///            false  If the parent is invalid
  bool SetParent(const Entity& parent);

  /// @brief Remove the reference to the parent if there is any
  void UnsetParent();

  /// @brief Destory the entity and all of its components
  void Destroy();

 private:
  /// @brief Constructor. Typically only called internally in the EntityRegistry when creating an entity
  /// @param[in] handle   entt handle to the entity
  /// @param[in] registry The registry where the entity will be added
  Entity(entt::entity handle, registry::EntityRegistry* registry);

  entt::entity m_entity_handle{0};
  registry::EntityRegistry* m_registry = nullptr;
  friend class registry::EntityRegistry;  // Only EntityRegistry is allowed to call the entity constructor
  friend class HierarchyComponent;        // HierarchyComponent can call private constructor with existing registry
};

/// @brief The entity as an ID
struct EntityHash {
  uint32_t operator()(const Entity& entity) const { return static_cast<uint32_t>(entity); }
};

/// @brief Component that enables entities to have parents and children
class HierarchyComponent {
 public:
  /// @brief Constructor
  /// @param entity The entity which is the owner of this component
  explicit HierarchyComponent(const Entity& entity);
  ~HierarchyComponent() = default;

  /// @brief Set the parent of this entity
  ///
  /// An entity does not need to have a parent
  /// @param[in] parent The parent entity
  /// @return    true   If successful
  ///            false  If the parent is invalid
  bool SetParent(const Entity& parent);

  /// @brief Remove the reference to the parent if there is any
  void UnsetParent();

  /// @brief Get a list of all the child entities of this entity
  /// @return An std::vector of entities. Empty if entity does not have any children
  std::vector<Entity> GetChildren() const;

  /// @brief Get the parent of the entity
  ///
  /// IsValid() of the parent will be true if this entity has a parent
  /// @return An entity which is the parent of this entity.
  Entity GetParent() const;

 private:
  Entity m_entity;
  Entity m_parent;
  // Store raw entt entities to avoid extra memory for duplicated registry pointers
  std::set<entt::entity> m_childHandles;
};

namespace registry {

/// @brief Registry of scene containing entities
class EntityRegistry {
 public:
  EntityRegistry() {}

  /// @brief Create an entity
  /// @return A valid entity added to the registry
  ///         An invalid Entity if failed
  Entity CreateEntity();

  /// @brief Create an entity and give it a name
  ///
  /// The name will be stored as a component of type std::string. The name can be used as identifier and should be
  /// unique to this entity
  /// @param name The name of the entity
  /// @return A valid entity added to the registry
  ///         An invalid Entity if failed
  Entity CreateEntity(const std::string& name);

  /// @brief Remove an entity from the scene and destroy all of its components
  /// @param entity The entity to remove
  void RemoveEntity(Entity* entity);

  /// @brief Checks if the entity handle refers to a valid entity that has been created and is owned by this registry
  /// @param entity The entity to test
  /// @return True if the entity is valid. False otherwise
  bool IsValid(const Entity& entity);

  /// @brief Get a view of all the entities holding components of the given type
  /// @tparam ...Types The types of components that have been previously added to entities within the registry
  /// @return A container of all the entities in the registry having all the requested components
  template <typename... Types>
  std::vector<Entity> view();

  /// @brief Attach a component to the entity. The component's constructor will be called with ...ArgTypes
  /// @tparam T            Component type
  /// @tparam ...ArgTypes  Template arg types
  /// @param  entity       The entity on which to attach the component
  /// @param  ...args      Forward args
  /// @return              Reference to the created component
  template <typename T, typename... ArgTypes>
  T& AddComponent(Entity* entity, ArgTypes&&... args);

  /// @brief Remove component of type T
  /// @tparam T      Component type
  /// @param  entity The entity from which to remove the component
  template <typename T>
  void RemoveComponent(Entity* entity);

  /// @brief Check if the entity has a component of type T
  /// @tparam T      Component type
  /// @param  entity The entity to test for the component
  /// @return True   If the entity has a component of type T
  template <typename T>
  bool HasComponent(const Entity& entity) const;

  void Clear() { m_registry.clear(); }

 private:
  std::mutex m_viewAccessMutex;
  entt::registry m_registry;               // The internal registry
  friend class nv3dvc::core::ecs::Entity;  // To avoid entt specific code to leak outside the EntityRegistry class
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Template function definitions                                                                                      //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename... Types>
std::vector<Entity> EntityRegistry::view() {
  std::vector<Entity> entity_view;
  // entt::registry::view is not thread safe. Lock to enable threaded access
  m_viewAccessMutex.lock();
  auto entities = m_registry.view<Types...>();
  m_viewAccessMutex.unlock();
  for (entt::entity handle : entities) {
    entity_view.push_back({handle, this});
  }
  return entity_view;
}

template <typename T, typename... ArgTypes>
T& EntityRegistry::AddComponent(Entity* entity, ArgTypes&&... args) {
  assert(entity != nullptr);
  assert(!HasComponent<T>(*entity));
  T& res = m_registry.emplace<T>(entity->m_entity_handle, std::forward<ArgTypes>(args)...);
  static_assert(std::is_convertible<T*, properties::PropertyOwner*>::value,
                "Component type must publicly inherit PropertyOwner");
  auto& property_owner = m_registry.get<properties::PropertyOwner>(entity->m_entity_handle);
  property_owner.AddSubOwner(static_cast<properties::PropertyOwner*>(&res));
  return res;
}

template <typename T>
void EntityRegistry::RemoveComponent(Entity* entity) {
  T* component = &m_registry.get<T>(entity->m_entity_handle);
  auto& property_owner = m_registry.get<properties::PropertyOwner>(entity->m_entity_handle);
  property_owner.RemoveSubOwner(component);
  m_registry.remove<T>(entity->m_entity_handle);
}

template <typename T>
bool EntityRegistry::HasComponent(const Entity& entity) const {
  return entity && m_registry.any_of<T>(entity.m_entity_handle);
}

}  // namespace registry

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Template function definitions                                                                                      //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T, typename... ArgTypes>
T& Entity::AddComponent(ArgTypes&&... args) {
  return m_registry->AddComponent<T>(this, std::forward<ArgTypes>(args)...);
}

template <typename T>
T& Entity::GetComponent() const {
  return m_registry->m_registry.get<T>(m_entity_handle);
}

template <typename T>
void Entity::RemoveComponent() {
  return m_registry->RemoveComponent<T>(this);
}

template <typename T>
bool Entity::HasComponent() const {
  return m_registry->HasComponent<T>(*this);
}

}  // namespace ecs
}  // namespace core
}  // namespace nv3dvc

#endif  // SRC_CORE_ENTITYCOMPONENTSYSTEM_ENTITY_H_
