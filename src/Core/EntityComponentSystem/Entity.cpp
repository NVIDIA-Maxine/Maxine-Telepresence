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

#include "Entity.h"

#include <string>

#include "uuid.h"

namespace nv3dvc {
namespace core {
/// @brief Entity-Component-System namespace
namespace ecs {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Member function definitions                                                                                        //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// Entity

Entity::Entity(entt::entity handle, registry::EntityRegistry* registry)
    : m_entity_handle(handle), m_registry(registry) {}

bool Entity::IsValid() const { return m_registry != nullptr && m_registry->IsValid(*this); }

Entity::operator bool() const { return IsValid(); }

Entity::operator const uint32_t() const { return static_cast<uint32_t>(m_entity_handle); }

Entity::operator const entt::entity() const { return m_entity_handle; }

bool Entity::operator==(const Entity& other) const {
  return m_entity_handle == other.m_entity_handle && m_registry == other.m_registry;
}

bool Entity::operator!=(const Entity& other) const { return !(*this == other); }

registry::EntityRegistry* Entity::Registry() const { return m_registry; }

bool Entity::HasParent() { return GetParent().IsValid(); }

Entity Entity::GetParent() { return GetComponent<HierarchyComponent>().GetParent(); }

std::vector<Entity> Entity::GetChildren() { return GetComponent<HierarchyComponent>().GetChildren(); }

bool Entity::SetParent(const Entity& parent) { return GetComponent<HierarchyComponent>().SetParent(parent); }

void Entity::UnsetParent() { GetComponent<HierarchyComponent>().UnsetParent(); }

void Entity::Destroy() {
  // This is ok. Destructor of Entity does not get called.
  // All component destructors will get called though, including that of the PropertyOwner
  m_registry->RemoveEntity(this);
}

/// HierarchyComponent

HierarchyComponent::HierarchyComponent(const Entity& entity) : m_entity(entity), m_parent{} {}

bool HierarchyComponent::SetParent(const Entity& parent) {
  // Make sure we don't find ourselves in ancestry as it would lead to a cycle
  Entity ancestor = parent;
  while (ancestor.IsValid()) {
    if (ancestor == m_entity) {
      return false;
    }
    ancestor = ancestor.GetParent();
  }

  UnsetParent();

  if (parent.IsValid()) {
    parent.GetComponent<HierarchyComponent>().m_childHandles.insert(static_cast<entt::entity>(m_entity));
  }
  m_parent = parent;
  return true;
}

void HierarchyComponent::UnsetParent() {
  if (m_parent.IsValid()) {
    m_parent.GetComponent<HierarchyComponent>().m_childHandles.erase(static_cast<entt::entity>(m_entity));
  }
  m_parent = Entity();  // Set to invalid entity
}

std::vector<Entity> HierarchyComponent::GetChildren() const {
  std::vector<Entity> res;
  res.reserve(m_childHandles.size());
  for (entt::entity child_handle : m_childHandles) {
    res.push_back({child_handle, m_entity.Registry()});
  }
  return res;
}

Entity HierarchyComponent::GetParent() const { return m_parent; }

namespace registry {

/// EntityRegistry

Entity EntityRegistry::CreateEntity() {
  auto entity_handle = m_registry.create();
  Entity entity = {entity_handle, this};
  if (entity.IsValid()) {
    // All entities should have a PropertyOwner, uuid, and HierarchyComponent
    auto& property_owner = m_registry.emplace<properties::PropertyOwner>(entity.m_entity_handle);
    if (!m_registry.any_of<uuids::uuid>(entity.m_entity_handle)) {
      m_registry.emplace<uuids::uuid>(entity.m_entity_handle) = uuids::uuid_system_generator{}();
    }
    m_registry.emplace<HierarchyComponent>(entity.m_entity_handle, entity);
  }
  return entity;
}

Entity EntityRegistry::CreateEntity(const std::string& name) {
  Entity entity = CreateEntity();
  if (entity.IsValid()) {
    m_registry.emplace<std::string>(entity.m_entity_handle) = name;
  }
  return entity;
}

void EntityRegistry::RemoveEntity(Entity* entity) {
  m_registry.destroy(static_cast<entt::entity>(entity->m_entity_handle));
}

bool EntityRegistry::IsValid(const Entity& entity) { return m_registry.valid(entity.m_entity_handle); }

}  // namespace registry
}  // namespace ecs
}  // namespace core
}  // namespace nv3dvc
