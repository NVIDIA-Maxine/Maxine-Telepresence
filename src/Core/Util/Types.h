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

#ifndef SRC_CORE_UTIL_TYPES_H_
#define SRC_CORE_UTIL_TYPES_H_

#include "core/Application/Inputs.h"

namespace nv3dvc {
namespace core {
namespace util {

/// @brief Use as serializable type. Does not store any data, but can be triggered from GUI to call OnChange
struct Trigger {
  application::inputs::Key key;
  application::inputs::Modifier modifier;
};

}  // namespace util
}  // namespace core
}  // namespace nv3dvc

#endif  // SRC_CORE_UTIL_TYPES_H_
