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

#ifndef SRC_MODULES_GUIMODULE_PANELS_RENDERPROPERTIES_H_
#define SRC_MODULES_GUIMODULE_PANELS_RENDERPROPERTIES_H_

#include "Core/Properties/Property.h"

namespace nv3dvc {
namespace modules {
namespace guimodule {
namespace panels {

/// @brief Helper functions for rendering all properties of a property owner
///
/// Will also render subowners recursively
/// @param[in] name           The name of the property owner to display
/// @param[in] property_owner The property owner to display all properties for
void RenderProperties(const char* name, const core::properties::PropertyOwner* property_owner);

}  // namespace panels
}  // namespace guimodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_GUIMODULE_PANELS_RENDERPROPERTIES_H_
