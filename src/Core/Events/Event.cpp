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

#include "Event.h"

#include <ostream>

namespace nv3dvc {
namespace core {
namespace events {

std::ostream& operator<<(std::ostream& out, Event const& e) { return out << e.ToString(); }

void Event::Consume() { m_consumed = true; }

bool Event::IsConsumed() const { return m_consumed; }

}  // namespace events
}  // namespace core
}  // namespace nv3dvc
