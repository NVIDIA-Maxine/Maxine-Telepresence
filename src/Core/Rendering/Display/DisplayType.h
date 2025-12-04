/*
 * SPDX-FileCopyrightText: Copyright (c) 2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: LicenseRef-NvidiaProprietary
 *
 * NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
 * property and proprietary rights in and to this material, related
 * documentation and any modifications thereto. Any use, reproduction,
 * disclosure or distribution of this material and related documentation
 * without an express license agreement from NVIDIA CORPORATION or
 * its affiliates is strictly prohibited.
 */

#ifndef SRC_CORE_RENDERING_DISPLAY_DISPLAYTYPE_H_
#define SRC_CORE_RENDERING_DISPLAY_DISPLAYTYPE_H_

#include "Core/Error.h"
#include "glm/glm.hpp"

namespace nv3dvc {
namespace core {
namespace rendering {
namespace display {

/// @ingroup EnumProperties
/// @defgroup DisplayType DisplayType
/// @brief Display type
/// @{
enum class DisplayType {
  VIRTUAL_DISPLAY,        ///< Virtual, non-physical display.
  SIMPLE_DISPLAY,         ///< Common 2D display type
  DIMENCO_DISPLAY,        ///< Dimenco stereo display type. Requires build with dimenco support
  LOOKING_GLASS_DISPLAY,  ///< LookingGlass stereo display type. Requires build with lookingglass support
};
/// @}

}  // namespace display
}  // namespace rendering
}  // namespace core
}  // namespace nv3dvc

#endif  // SRC_CORE_RENDERING_DISPLAY_DISPLAYTYPE_H_
