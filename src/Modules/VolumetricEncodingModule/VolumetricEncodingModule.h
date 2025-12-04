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

#ifndef SRC_MODULES_VOLUMETRICENCODINGMODULE_VOLUMETRICENCODINGMODULE_H_
#define SRC_MODULES_VOLUMETRICENCODINGMODULE_VOLUMETRICENCODINGMODULE_H_

#include <string>

#include "Core/Engine/Module.h"

// Export components and systems
#include "Components/EncodedTriplaneComponent.h"
#include "Systems/TriplaneEncoderSystem.h"

namespace nv3dvc {
namespace modules {
namespace volumetricencodingmodule {

/// @defgroup VolumetricEncodingModuleProperties VolumetricEncodingModule
/// @ingroup  ModuleProperties
/// @brief    Module required for performing encoding of images into volumetric objects
///
/// Registers the following component:
/// - components::EncodedTriplaneComponent
///
/// Registers the following system:
/// - systems::TriplaneEncoderSystem

/// See @ref VolumetricEncodingModuleProperties
class VolumetricEncodingModule : public core::engine::Module {
 public:
  constexpr static const char* NAME = "VolumetricEncodingModule";
  std::string Name() const override { return "VolumetricEncodingModule"; };

  VolumetricEncodingModule();

  nv3dvc::core::Error Initialize() override;

  nv3dvc::core::Error Uninitialize() override;

  nv3dvc::core::Error Update(float dt) override;

 private:
};

}  // namespace volumetricencodingmodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_VOLUMETRICENCODINGMODULE_VOLUMETRICENCODINGMODULE_H_
