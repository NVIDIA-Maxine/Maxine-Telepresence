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

#include "DataBufferComponent.h"

#include <utility>
#include <vector>

namespace nv3dvc {
namespace modules {
namespace commonmodule {
namespace components {

_DataBufferComponent::_DataBufferComponent() {}

void _DataBufferComponent::Append(std::vector<uint8_t>&& packet) { m_dataBuffers.emplace_back(std::move(packet)); }

void _DataBufferComponent::Append(std::vector<std::vector<uint8_t>>&& packets) {
  m_dataBuffers.insert(m_dataBuffers.end(), std::make_move_iterator(packets.begin()),
                       std::make_move_iterator(packets.end()));
}

const std::vector<std::vector<uint8_t>>& _DataBufferComponent::GetBuffers() { return m_dataBuffers; }

void _DataBufferComponent::Consume() { m_dataBuffers.clear(); }

}  // namespace components
}  // namespace commonmodule
}  // namespace modules
}  // namespace nv3dvc
