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

#include "Timeable.h"

namespace nv3dvc {
namespace core {
namespace util {

void Timeable::SetTimeStamp(int64_t timestamp) { m_timeStamp.store(timestamp); }

int64_t Timeable::GetTimeStamp() const { return m_timeStamp.load(); }

}  // namespace util
}  // namespace core
}  // namespace nv3dvc
