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

#ifndef SRC_CORE_ENTITYCOMPONENTSYSTEM_COMPONENT_H_

#define SRC_CORE_ENTITYCOMPONENTSYSTEM_COMPONENT_H_

#include "Core/Properties/Property.h"

namespace nv3dvc {
namespace core {
namespace ecs {

/// @brief Components are smaller data classes which can be attached to entities
///
/// The component base class defines a basic interface to ensure the ability to copy and move components. This is a
/// strict requirement to enable usage of the entity registry. In addition, the Component base class is a PropertyOwner
/// which allows for serialization, deserialization, and modification of property values added to the Component.
/// Properties should be added in the default constructor of the Component
///
/// Sample extension of component:
/// \snippet src/Samples/MyModule/MyModule.h Simple component sample
class Component : public nv3dvc::core::properties::PropertyOwner {};

}  // namespace ecs
}  // namespace core
}  // namespace nv3dvc

#endif  // SRC_CORE_ENTITYCOMPONENTSYSTEM_COMPONENT_H_
