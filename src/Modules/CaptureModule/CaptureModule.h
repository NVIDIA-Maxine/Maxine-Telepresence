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

#ifndef SRC_MODULES_CAPTUREMODULE_CAPTUREMODULE_H_
#define SRC_MODULES_CAPTUREMODULE_CAPTUREMODULE_H_

#include <string>

#include "Core/Engine/Engine.h"
#include "Core/Engine/Module.h"
#include "Modules/BehaviorModule/BehaviorModule.h"

// Export components and systems
#include "Components/RecordingBehavior.h"
#include "Components/WebCameraComponent.h"
#include "Systems/CameraCaptureSystem.h"

namespace nv3dvc {
namespace modules {
namespace capturemodule {

/// @defgroup CaptureModuleProperties CaptureModule
/// @ingroup  ModuleProperties
/// @brief    Module for camera capture using web cameras or video files
///
/// Registers systems:
/// - nv3dvc::modules::capturemodule::systems::CameraCaptureSystem
///
/// Registers components:
/// - nv3dvc::modules::capturemodule::components::RecordingBehavior
/// - nv3dvc::modules::capturemodule::components::WebCameraComponent

/// See @ref CaptureModuleProperties
class CaptureModule : public core::engine::Module {
 public:
  constexpr static const char* NAME = "CaptureModule";
  std::string Name() const override { return "CaptureModule"; };

  /// @brief Constructor
  /// @param[in,out] engine          The engine. Required for registering CameraCaptureSystem
  /// @param[in,out] behavior_module The behavior module. Required for registering RecordingBehavior
  CaptureModule(core::engine::Engine* engine, behaviormodule::BehaviorModule* behavior_module);

  nv3dvc::core::Error Initialize() override;

  nv3dvc::core::Error Uninitialize() override;

  nv3dvc::core::Error Update(float dt) override;

  nv3dvc::core::Error EncodeProperties(nlohmann::json* json_description,
                                       const PropertyOwner* property_owner) const override;

  nv3dvc::core::Error DecodeProperties(const nlohmann::json& json_description,
                                       PropertyOwner* property_owner) const override;

 private:
  bool m_gstInitialized = false;
};

}  // namespace capturemodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_CAPTUREMODULE_CAPTUREMODULE_H_
