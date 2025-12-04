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

#ifndef SRC_CORE_SERIALIZATION_PROPERTYDESCRIPTION_H_
#define SRC_CORE_SERIALIZATION_PROPERTYDESCRIPTION_H_

#include "nlohmann/json.hpp"

namespace nv3dvc {
namespace core {
namespace serialization {

/// @brief A serialized description of a set of PropertyOwners
///
/// A PropertyDescription can be used to describe any set of of owners of properties. This includes a scene description
/// based on an EntityRegistry, or a system description based on a SystemRegistry. A PropertyDescription is really just
/// a hierarchical map of named identifiers to property values. The internal representation is a json object.
class PropertyDescription {
 public:
  /// @brief Construct an empty PropertyDescription object
  PropertyDescription();

  /// @brief Copy-construct a PropertyDescription object from a JSON object
  explicit PropertyDescription(const nlohmann::json& json_description);

  /// @brief Move-construct a PropertyDescription object from a JSON object
  explicit PropertyDescription(nlohmann::json&& json_description);

  /// @brief Destructor
  ~PropertyDescription();

  /// @brief Move-assign this PropertyDescription object from another PropertyDescription object
  /// @param other The other PropertyDescription to move from
  /// @return A reference to this PropertyDescription object
  PropertyDescription& operator=(PropertyDescription&& other) noexcept;

  /// @brief Create a JSON representation of this PropertyDescription object
  /// @return The JSON representation
  nlohmann::json ToJson() const;

  /// @brief Update a current property description by merging with other, leaving other untouched
  /// @param other The other property description to merge with
  /// @return This updated property description
  PropertyDescription& Update(const PropertyDescription& other);

 private:
  nlohmann::json m_jsonDescription;  /// Internal map of property values
};

}  // namespace serialization
}  // namespace core
}  // namespace nv3dvc

#endif  // SRC_CORE_SERIALIZATION_PROPERTYDESCRIPTION_H_
