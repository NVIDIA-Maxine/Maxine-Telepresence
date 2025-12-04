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

#include "PropertyDescription.h"

#include <utility>

namespace nv3dvc {
namespace core {
namespace serialization {

PropertyDescription::PropertyDescription() {}

PropertyDescription::PropertyDescription(const nlohmann::json& json_description)
    : m_jsonDescription(json_description) {}

PropertyDescription::PropertyDescription(nlohmann::json&& json_description)
    : m_jsonDescription(std::move(json_description)) {}

PropertyDescription::~PropertyDescription() {}

PropertyDescription& PropertyDescription::operator=(PropertyDescription&& other) noexcept {
  m_jsonDescription = std::move(other.m_jsonDescription);
  return *this;
}

nlohmann::json PropertyDescription::ToJson() const { return m_jsonDescription; }

PropertyDescription& PropertyDescription::Update(const PropertyDescription& other) {
  if (!other.m_jsonDescription.is_null()) {
    m_jsonDescription.update(other.m_jsonDescription, true);
  }
  return *this;
}

}  // namespace serialization
}  // namespace core
}  // namespace nv3dvc
