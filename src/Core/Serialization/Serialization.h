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

#ifndef SRC_CORE_SERIALIZATION_SERIALIZATION_H_
#define SRC_CORE_SERIALIZATION_SERIALIZATION_H_

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "Core/Engine/Module.h"
#include "Core/EntityComponentSystem/Entity.h"
#include "Core/EntityComponentSystem/System.h"
#include "Core/Error.h"
#include "Core/Util/Logger.h"
#include "PropertyDescription.h"
#include "nlohmann/json.hpp"

namespace nv3dvc {
namespace core {
namespace serialization {

/// @brief Encode properties of a given type into the provided json description
///
/// Only properties of type T registered in the property owner will be added to the json description. Hence this
/// function needs to be called for all types that should be encoded into the json description
/// @tparam T      The template type to encode
/// @param[in,out] json_description    Where the properties will be serialized
/// @param[in]     property_owner      The property owner to serialize
/// @return        nv3dvc::core::SUCCESS if successful
template <typename T>
Error EncodeProperties(nlohmann::json* json_description, const properties::PropertyOwner* property_owner);

/// @brief Decode properties of a given type from a json description
///
/// Only properties of type T registered in the property owner will be read from the json description. Hence this
/// function needs to be called for all types that should be decoded from the json description.
/// Note the example usage @code DecodeProperties<bool>(json_description, property_owner) @endcode, as opposed to @code
/// DecodeProperties<std::atomic<bool>>(json_description, property_owner) @endcode as the std::atomic wrapper is
/// implicit in the interpretation of the type
/// @tparam T      The template type to decode
/// @param[in]     json_description    Where the properties will be read
/// @param[in,out] property_owner      Where the property values will be written
/// @return        nv3dvc::core::SUCCESS if successful
template <typename T>
Error DecodeProperties(const nlohmann::json& json_description, properties::PropertyOwner* property_owner);

/// @brief Encode properties of a given atomic type into the provided json description
///
/// Only properties of type T registered in the property owner will be added to the json description. Hence this
/// function needs to be called for all types that should be encoded into the json description.
/// Note the example usage @code EncodeAtomicProperties<bool>(json_description, property_owner) @endcode, as opposed to
/// @code EncodeAtomicProperties<std::atomic<bool>>(json_description, property_owner) @endcode as the std::atomic
/// wrapper is implicit in the interpretation of the type
/// @tparam T      The raw type to encode
/// @param[in,out] json_description    Where the properties will be serialized
/// @param[in]     property_owner      The property owner to serialize
/// @return        nv3dvc::core::SUCCESS if successful
template <typename T>
Error EncodeAtomicProperties(nlohmann::json* json_description, const properties::PropertyOwner* property_owner);

/// @brief Decode properties of a given atomic type from a json description
///
/// Only properties of type T registered in the property owner will be read from the json description. Hence this
/// function needs to be called for all types that should be decoded from the json description
/// @tparam T      The raw type to decode
/// @param[in]     json_description    Where the properties will be read
/// @param[in,out] property_owner      Where the property values will be written
/// @return        nv3dvc::core::SUCCESS if successful
template <typename T>
Error DecodeAtomicProperties(const nlohmann::json& json_description, properties::PropertyOwner* property_owner);

/// @brief Create an empty PropertyDescription
///
/// Can be used when serializing multiple property super owners to the same description
/// @return an empty PropertyDescription
PropertyDescription EmptyDescription();

/// @brief Load a PropertyDescription from file
/// @param[in]  file_path The path to the file to read. Current supported format is json
/// @param[out] err_ptr   The address to write an error code for the operation
/// @return              A property description based on the input file. Valid if the file was read successfully
PropertyDescription LoadDescription(const std::string& file_path, Error* err_ptr);

/// @brief Load a PropertyDescription from string
/// @param[in] str  The string to the file to read. Current supported format is json
/// @return         A property description based on the input file. Valid if the file was read successfully
PropertyDescription LoadDescriptionString(const std::string& str, Error* err_ptr);

/// @brief Save a PropertyDescription to file
/// @param[in] property_description The PropertyDescription to write to the output
/// @param[in] file_path            The path to the file to write
/// @return    nv3dvc::core::SUCCESS if successful
Error WriteDescription(const PropertyDescription& property_description, const std::string& file_path);

/// @brief Save a PropertyDescription to string
/// @param[in] property_description The PropertyDescription to write to the output
/// @param[in] file_path            The string to write
/// @return    SUCCESS              If successful
Error WriteDescriptionString(const PropertyDescription& scene_description, std::string* result);

/// @brief Serialize a common registry of super owners
/// @param[in] super_owner The PropertyOwner to serialize
/// @param[in] modules     The modules where all the different component types that the serialization supports are
///                        registered.
/// @return A PropertyDescription with all properties available in the super owner, serialized
PropertyDescription SerializeRegistry(const properties::PropertyOwner& super_owner,
                                      const std::vector<std::unique_ptr<engine::Module>>& modules = {},
                                      Error* err_ptr = nullptr);

/// @brief Serialize an EntityRegistry into a PropertyDescription
/// @param[in] registry The entity registry to serialize
/// @param[in] modules  The modules where all the different component types that the serialization supports are
///                     registered.
/// @return             The serialized EntityRegistry as a PropertyDescription
PropertyDescription SerializeRegistry(ecs::registry::EntityRegistry* registry,
                                      const std::vector<std::unique_ptr<engine::Module>>& modules = {},
                                      Error* err_ptr = nullptr);

/// @brief Deserialize a common property description to a representation of property super owners
/// @param[in]     serialized_description The property description defining all property values
/// @param[in,out] super_owner            The PropertyOwner where to serialize the description
/// @param[in]     modules                The modules where all the different component types that the deserialization
///                                       supports are registered.
/// @return        nv3dvc::core::SUCCESS if successful
Error DeserializeRegistry(const PropertyDescription& serialized_description, properties::PropertyOwner* super_owner,
                          const std::vector<std::unique_ptr<engine::Module>>& modules);

/// @brief Deserialize an EntityRegistry based on a PropertyDescription
/// @param[in]     description     The PropertyDescription to deserialize into a entity registry
/// @param[in,out] entity_registry The EntityRegistry in which properties will be written. Each entity is assumed to be
///                                identified using a unique id.
/// @param[in]     modules         The modules where all the different component types that the deserialization supports
///                                are registered.
/// @return        nv3dvc::core::SUCCESS If successful
Error DeserializeRegistry(const PropertyDescription& description, ecs::registry::EntityRegistry* entity_registry,
                          const std::vector<std::unique_ptr<engine::Module>>& modules);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Template function definitions                                                                                      //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
Error EncodeProperties(nlohmann::json* json_description, const properties::PropertyOwner* property_owner) {
  Error err = Error::SUCCESS;
  CHECK_NONNULL(json_description, Error::ERR_NULL_POINTER);
  CHECK_NONNULL(property_owner, Error::ERR_NULL_POINTER);
  try {
    for (const auto prop : property_owner->GetProperties<T>()) {
      (*json_description)[prop->name()] = *prop->get();
    }
  } catch (const nlohmann::detail::exception& e) {
    LOG_ERROR("Json error: %s", e.what());
    err = Error::ERR_SERIALIZE;
  }
bail:
  return err;
}

template <typename T>
Error DecodeProperties(const nlohmann::json& json_description, properties::PropertyOwner* property_owner) {
  Error err = Error::SUCCESS;
  CHECK_NONNULL(property_owner, Error::ERR_NULL_POINTER);
  for (const auto prop : property_owner->GetProperties<T>()) {
    if (!json_description.contains(prop->name())) {
      std::string default_value_str;
      try {
        default_value_str = nlohmann::json(*prop->get()).dump();
      } catch (const std::exception& e) {
        LOG_ERROR("json exception: % s ", e.what());
        default_value_str = "{unable to get default value due to json exception}";
        err = Error::ERR_DESERIALIZE;
      }
      LOG_WARNING("Cannot find %s in %s. Using Default value: %s", prop->name(), property_owner->Name().c_str(),
                  default_value_str.c_str());
      continue;
    }
    T& value = *prop->get();
    json_description[prop->name()].get_to(value);
    prop->OnChange();
  }
bail:
  return err;
}

template <typename T>
Error EncodeAtomicProperties(nlohmann::json* json_description, const properties::PropertyOwner* property_owner) {
  Error err = Error::SUCCESS;
  CHECK_NONNULL(json_description, Error::ERR_NULL_POINTER);
  CHECK_NONNULL(property_owner, Error::ERR_NULL_POINTER);
  try {
    for (const auto prop : property_owner->GetProperties<std::atomic<T>>()) {
      (*json_description)[prop->name()] = prop->get()->load();
    }
  } catch (const nlohmann::detail::exception& e) {
    LOG_ERROR("Json error: %s", e.what());
    err = Error::ERR_SERIALIZE;
  }
bail:
  return err;
}

template <typename T>
Error DecodeAtomicProperties(const nlohmann::json& json_description, properties::PropertyOwner* property_owner) {
  Error err = Error::SUCCESS;
  CHECK_NONNULL(property_owner, Error::ERR_NULL_POINTER);
  for (const auto prop : property_owner->GetProperties<std::atomic<T>>()) {
    if (!json_description.contains(prop->name())) {
      std::string default_value_str;
      try {
        default_value_str = nlohmann::json(prop->get()->load()).dump();
      } catch (const std::exception& e) {
        LOG_ERROR("json exception: % s ", e.what());
        default_value_str = "{unable to get default value due to json exception}";
        err = Error::ERR_DESERIALIZE;
      }
      LOG_WARNING("Cannot find %s in %s. Using Default value: %s", prop->name(), property_owner->Name().c_str(),
                  default_value_str.c_str());
      continue;
    }
    T value;
    json_description[prop->name()].get_to(value);
    prop->get()->store(value);
    prop->OnChange();
  }
bail:
  return err;
}

}  // namespace serialization
}  // namespace core
}  // namespace nv3dvc

#endif  // SRC_CORE_SERIALIZATION_SERIALIZATION_H_
