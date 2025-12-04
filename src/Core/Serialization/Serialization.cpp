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

#include "Serialization.h"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <utility>

#include "Core/Engine/Module.h"
#include "Core/EntityComponentSystem/Entity.h"
#include "Core/EntityComponentSystem/System.h"
#include "Core/Error.h"
#include "Core/Properties/Property.h"
#include "Core/Serialization/CustomJsonTypes.h"
#include "Core/Serialization/CustomJsonTypesGlm.h"
#include "Core/Serialization/PropertyDescription.h"
#include "Core/Util/Logger.h"
#include "Core/Util/Types.h"
#include "nlohmann/json.hpp"
#include "uuid.h"

namespace nv3dvc {
namespace core {
namespace serialization {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Macros                                                                                                           ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// A list of all supported serialized types. The list can be extended with new types, as long as the types have
// supported json serialization schemes
#define SUPPORT_ENCODED_TYPES(ENC)   \
  {                                  \
    /* Basic types */                \
    ENC(util::Trigger);              \
    ENC(bool);                       \
    ENC(int16_t);                    \
    ENC(uint16_t);                   \
    ENC(unsigned int);               \
    ENC(int);                        \
    ENC(int32_t);                    \
    ENC(int64_t);                    \
    ENC(uint32_t);                   \
    ENC(uint64_t);                   \
    ENC(signed char);                \
    ENC(unsigned char);              \
    ENC(float);                      \
    ENC(double);                     \
    ENC(wchar_t);                    \
    ENC(std::string);                \
    /* Glm types */                  \
    ENC(glm::u8vec1); /*Color R*/    \
    ENC(glm::u8vec2); /*Color RG*/   \
    ENC(glm::u8vec3); /*Color RGB*/  \
    ENC(glm::u8vec4); /*Color RGBA*/ \
    ENC(glm::bvec1);                 \
    ENC(glm::bvec2);                 \
    ENC(glm::bvec3);                 \
    ENC(glm::bvec4);                 \
    ENC(glm::ivec1);                 \
    ENC(glm::ivec2);                 \
    ENC(glm::ivec3);                 \
    ENC(glm::ivec4);                 \
    ENC(glm::uvec1);                 \
    ENC(glm::uvec2);                 \
    ENC(glm::uvec3);                 \
    ENC(glm::uvec4);                 \
    ENC(glm::vec1);                  \
    ENC(glm::vec2);                  \
    ENC(glm::vec3);                  \
    ENC(glm::vec4);                  \
    ENC(glm::dvec1);                 \
    ENC(glm::dvec2);                 \
    ENC(glm::dvec3);                 \
    ENC(glm::dvec4);                 \
    ENC(glm::mat2x2);                \
    ENC(glm::mat2x3);                \
    ENC(glm::mat2x4);                \
    ENC(glm::mat3x2);                \
    ENC(glm::mat3x3);                \
    ENC(glm::mat3x4);                \
    ENC(glm::mat4x2);                \
    ENC(glm::mat4x3);                \
    ENC(glm::mat4x4);                \
    ENC(glm::dmat2x2);               \
    ENC(glm::dmat2x3);               \
    ENC(glm::dmat2x4);               \
    ENC(glm::dmat3x2);               \
    ENC(glm::dmat3x3);               \
    ENC(glm::dmat3x4);               \
    ENC(glm::dmat4x2);               \
    ENC(glm::dmat4x3);               \
    ENC(glm::dmat4x4);               \
    ENC(glm::fquat);                 \
    ENC(glm::dquat);                 \
  }

// A list of all supported serialized atomic types. Integral types are supported as atomics. The list can be extended
// with new types, as long as the types have supported json serialization schemes
#define SUPPORT_ENCODED_ATOMIC_TYPES(ENC) \
  {                                       \
    ENC(bool);                            \
    ENC(char);                            \
    ENC(signed char);                     \
    ENC(unsigned char);                   \
    ENC(int16_t);                         \
    ENC(uint16_t);                        \
    ENC(int);                             \
    ENC(unsigned int);                    \
    ENC(int32_t);                         \
    ENC(uint32_t);                        \
    ENC(int64_t);                         \
    ENC(uint64_t);                        \
    ENC(char16_t);                        \
    ENC(char32_t);                        \
    ENC(wchar_t);                         \
    ENC(float);                           \
    ENC(double);                          \
  }

// Encode properties of the given type. See EncodeProperties
#define ENCODE_PROPERTIES_OF_TYPE(T) EncodeProperties<T>(&json_description, property_owner)

// Decode properties of the given type. See DeocdeProperties
#define DECODE_PROPERTIES_OF_TYPE(T) DecodeProperties<T>(json_description, property_owner)

// Encode properties of the given atomic type. See EncodeAtomicProperties
#define ENCODE_PROPERTIES_OF_ATOMIC_TYPE(T) EncodeAtomicProperties<T>(&json_description, property_owner)

// Decode properties of the given atomic type. See DecodeAtomicProperties
#define DECODE_PROPERTIES_OF_ATOMIC_TYPE(T) DecodeAtomicProperties<T>(json_description, property_owner)

// Encode properties of all supported types
#define ENCODE_PROPERTIES() SUPPORT_ENCODED_TYPES(ENCODE_PROPERTIES_OF_TYPE)

// Decode properties of all supported types
#define DECODE_PROPERTIES() SUPPORT_ENCODED_TYPES(DECODE_PROPERTIES_OF_TYPE)

// Encode properties of all supported atomic types
#define ENCODE_ATOMIC_PROPERTIES() SUPPORT_ENCODED_ATOMIC_TYPES(ENCODE_PROPERTIES_OF_ATOMIC_TYPE)

// Decode properties of all supported atomic types
#define DECODE_ATOMIC_PROPERTIES() SUPPORT_ENCODED_ATOMIC_TYPES(DECODE_PROPERTIES_OF_ATOMIC_TYPE)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Static function declarations ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Static function definitions ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Public function definitions ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PropertyDescription EmptyDescription() { return PropertyDescription(); }

Error WriteDescription(const PropertyDescription& scene_description, const std::string& file_path) {
  const nlohmann::json serialized_scene = scene_description.ToJson();
  std::ofstream file(file_path);
  if (!file.is_open()) return ERR_FILE;
  file << serialized_scene.dump(2);
  file.close();
  return SUCCESS;
}

Error WriteDescriptionString(const PropertyDescription& scene_description, std::string* result) {
  core::Error err = core::Error::SUCCESS;
  CHECK_NONNULL(result, core::ERR_NULL_POINTER);
  {
    const nlohmann::json serialized_scene = scene_description.ToJson();
    std::ostringstream file;
    file << serialized_scene.dump(2);
    *result = file.str();
  }
bail:
  return err;
}

PropertyDescription LoadDescription(const std::string& file_path, Error* err_ptr) {
  if (err_ptr) *err_ptr = SUCCESS;
  nlohmann::json json_descr;
  try {
    std::ifstream file;
    file.open(file_path);
    if (!file.is_open()) throw std::ios_base::failure("Failed to open file");
    json_descr = nlohmann::json::parse(file);
    file.close();
  } catch (const nlohmann::json::parse_error& e) {
    LOG_ERROR("Unable to parse %s with error %s", file_path.c_str(), e.what());
    if (err_ptr) *err_ptr = ERR_PARSE;
  } catch (const nlohmann::detail::type_error& e) {
    LOG_ERROR("Unable to parse %s with error %s", file_path.c_str(), e.what());
    if (err_ptr) *err_ptr = ERR_PARSE;
  } catch (const std::ios_base::failure& e) {
    LOG_ERROR("Unable to read file %s with error %s", file_path.c_str(), e.what());
    if (err_ptr) *err_ptr = ERR_FILE;
  } catch (const std::system_error& e) {
    LOG_ERROR("Unable to read file %s with error %s", file_path.c_str(), e.what());
    if (err_ptr) *err_ptr = ERR_FILE;
  } catch (...) {
    LOG_ERROR("Unknown error when reading file %s", file_path.c_str());
    if (err_ptr) *err_ptr = ERR_FILE;
  }
  return PropertyDescription(std::move(json_descr));
}

PropertyDescription LoadDescriptionString(const std::string& str, Error* err_ptr) {
  if (err_ptr) *err_ptr = SUCCESS;
  nlohmann::json json_descr;
  try {
    std::istringstream isstream(str);
    json_descr = nlohmann::json::parse(isstream);
  } catch (const nlohmann::json::parse_error& e) {
    LOG_ERROR("Unable to parse string with error %s", e.what());
    if (err_ptr) *err_ptr = ERR_PARSE;
  } catch (const nlohmann::detail::type_error& e) {
    LOG_ERROR("Unable to parse string with error %s", e.what());
    if (err_ptr) *err_ptr = ERR_PARSE;
  } catch (...) {
    LOG_ERROR("Unknown error when parsing string");
    if (err_ptr) *err_ptr = ERR_PARSE;
  }
  return PropertyDescription(std::move(json_descr));
}

PropertyDescription SerializeRegistry(const properties::PropertyOwner& super_owner,
                                      const std::vector<std::unique_ptr<engine::Module>>& modules, Error* err_ptr) {
  Error err = SUCCESS;
  nlohmann::json full_json_description;
  full_json_description[super_owner.Name()] = {};

  nlohmann::json json_description = full_json_description[super_owner.Name()];
  const properties::PropertyOwner* property_owner = &super_owner;

  ENCODE_PROPERTIES();
  ENCODE_ATOMIC_PROPERTIES();

  // Encode any module specific component properties
  for (const auto& module : modules) {
    err = module->EncodeProperties(&json_description, property_owner);
    if (err != core::Error::SUCCESS) {
      LOG_WARNING("Failed to encode properties from module: %s", module->Name().c_str());
    }
  }

  for (auto& sub_owner_pair : super_owner.SubOwners()) {
    PropertyDescription prop_desc = SerializeRegistry(*sub_owner_pair.second, modules, &err);
    BAIL_IF_ERR(err);
    json_description[sub_owner_pair.first] = prop_desc.ToJson();
  }

bail:
  if (err_ptr) *err_ptr = err;
  return PropertyDescription(std::move(json_description));
}

PropertyDescription SerializeRegistry(ecs::registry::EntityRegistry* registry,
                                      const std::vector<std::unique_ptr<engine::Module>>& modules, Error* err_ptr) {
  Error err = Error::SUCCESS;
  nlohmann::json entities_json_description;
  auto view = registry->view<properties::PropertyOwner>();
  for (auto entity : view) {
    properties::PropertyOwner& pso = entity.GetComponent<properties::PropertyOwner>();

    nlohmann::json entity_json_description;
    entity_json_description["components"] = {};

    // Name
    if (entity.HasComponent<std::string>()) {
      entity_json_description["name"] = entity.GetComponent<std::string>();
    }

    // Encode components
    nlohmann::json& json_description = entity_json_description["components"];
    const properties::PropertyOwner* property_owner = &pso;

    ENCODE_PROPERTIES();
    ENCODE_ATOMIC_PROPERTIES();

    // Encode any module specific component properties
    for (const auto& module : modules) {
      err = module->EncodeProperties(&json_description, property_owner);
      BAIL_IF_ERR(err);
    }

    // Encode subowners
    for (auto& sub_owner_pair : pso.SubOwners()) {
      PropertyDescription prop_desc = SerializeRegistry(*sub_owner_pair.second, modules, &err);
      BAIL_IF_ERR(err);
      json_description[sub_owner_pair.first] = prop_desc.ToJson();
    }

    // Write parent
    if (entity.GetParent().IsValid()) {
      const uuids::uuid parent_uuid = entity.GetParent().GetComponent<uuids::uuid>();
      const std::string parent_uuid_str = uuids::to_string(parent_uuid);
      entity_json_description["parent"] = parent_uuid_str;
    }

    uuids::uuid uuid = entity.GetComponent<uuids::uuid>();
    std::string uuid_str = uuids::to_string(uuid);
    entities_json_description[uuid_str] = entity_json_description;
  }
bail:
  if (err_ptr) *err_ptr = err;
  nlohmann::json full_json_description;
  full_json_description["scene"] = {};
  full_json_description["scene"]["entities"] = entities_json_description;
  return PropertyDescription(std::move(full_json_description));
}

Error DeserializeRegistry(const PropertyDescription& serialized_description, properties::PropertyOwner* super_owner,
                          const std::vector<std::unique_ptr<engine::Module>>& modules) {
  Error err = SUCCESS;
  nlohmann::json json_description = serialized_description.ToJson();
  properties::PropertyOwner* property_owner = super_owner;

  DECODE_PROPERTIES();
  DECODE_ATOMIC_PROPERTIES();

  // Decode any module specific component properties
  for (const auto& module : modules) {
    Error e = module->DecodeProperties(json_description, super_owner);
    if (SUCCESS == err) err = e;  // Keep only the first error
  }

  for (auto& property_owner_pair : super_owner->SubOwners()) {
    PropertyDescription prop_desc = PropertyDescription(json_description[property_owner_pair.first]);
    Error e = DeserializeRegistry(prop_desc, property_owner_pair.second, modules);
    if (SUCCESS == err) err = e;  // Keep only the first error
  }

  return err;
}

Error DeserializeRegistry(const PropertyDescription& description, ecs::registry::EntityRegistry* entity_registry,
                          const std::vector<std::unique_ptr<engine::Module>>& modules) {
  Error err = SUCCESS;

  // Map for unique names. Unique names enables identification of entities that were created programatically
  std::unordered_map<std::string, ecs::Entity*> string_entity_map;
  std::unordered_map<ecs::Entity, uuids::uuid, ecs::EntityHash> child_to_parent_map;
  std::unordered_map<uuids::uuid, ecs::Entity> uuid_to_entity_map;
  std::vector<ecs::Entity> entities = entity_registry->view<uuids::uuid>();
  for (ecs::Entity& entity : entities) {
    string_entity_map[entity.GetComponent<std::string>()] = &entity;
  }
  try {
    nlohmann::json full_json_description = description.ToJson();
    for (auto& [uuid_str, entity_config] : full_json_description["scene"]["entities"].items()) {
      // Unique ID as key
      uuids::uuid uuid = uuids::uuid::from_string(uuid_str).value();

      // Either this entity already exists, or it needs to be created
      bool entity_already_exists = false;
      ecs::Entity* entity;
      ecs::Entity entity_;
      if (entity_config.contains("name")) {
        std::string name = entity_config["name"];
        if (string_entity_map.count(name) != 0) {
          entity = string_entity_map[name];
          entity_already_exists = true;
        } else {
          entity_ = entity_registry->CreateEntity(name);
          entity = &entity_;
        }
      } else {
        entity_ = entity_registry->CreateEntity();
        entity = &entity_;
      }

      entity->GetComponent<uuids::uuid>() = uuid;
      uuid_to_entity_map[uuid] = *entity;

      const nlohmann::json& component_configs = entity_config["components"];

      // Add all components
      if (!entity_already_exists) {
        for (const auto& component_config : component_configs.items()) {
          const std::string& component_name = component_config.key();
          bool attached = false;
          for (const auto& module : modules) {
            attached = attached || module->TryAttachComponentInModule(component_name, entity);
          }
          if (!attached) {
            LOG_WARNING("Component \"%s\" is not registered in any currently loaded modules", component_name.c_str());
          }
        }
      }

      // Set property values
      properties::PropertyOwner& property_super_owner = entity->GetComponent<properties::PropertyOwner>();
      auto& ref_owners = property_super_owner.SubOwners();

      for (const auto& component_config : component_configs.items()) {
        const std::string& component_name = component_config.key();
        if (ref_owners.find(component_name) == ref_owners.end()) {
          // Component does not exist, skip it.
          continue;
        }

        const nlohmann::json& json_description = component_config.value();
        properties::PropertyOwner* property_owner = ref_owners.at(component_name);

        DECODE_PROPERTIES();
        DECODE_ATOMIC_PROPERTIES();

        // Decode any module specific component properties
        for (const auto& module : modules) {
          const Error e = module->DecodeProperties(json_description, property_owner);
          if (SUCCESS == err) err = e;  // Keep only the first error
        }

        const auto& sub_owners = property_owner->SubOwners();
        for (const auto& sub_owner : sub_owners) {
          if (json_description.contains(sub_owner.first)) {
            PropertyDescription prop_desc = PropertyDescription(json_description[sub_owner.first]);
            const Error e = DeserializeRegistry(prop_desc, sub_owner.second, modules);
            if (SUCCESS == err) err = e;  // Keep only the first error
          }
        }
        // Get parent
        if (entity_config.contains("parent")) {
          const nlohmann::json& parent_config = entity_config["parent"];
          const std::string parent_uuid_str = parent_config.get<std::string>();
          uuids::uuid parent_uuid = uuids::uuid::from_string(parent_uuid_str).value();
          child_to_parent_map[*entity] = parent_uuid;
        }
      }
    }
    // Set up hierarchy
    for (const auto& [child, parent_uuid] : child_to_parent_map) {
      child.GetComponent<ecs::HierarchyComponent>().SetParent(uuid_to_entity_map[parent_uuid]);
    }
  } catch (const std::exception& e) {
    LOG_ERROR("Failed to deserialize with error %s", e.what());
    err = ERR_DESERIALIZE;
  } catch (...) {
    LOG_ERROR("Failed to deserialize with unknown error");
    err = ERR_DESERIALIZE;
  }
  return err;
}

}  // namespace serialization
}  // namespace core
}  // namespace nv3dvc
