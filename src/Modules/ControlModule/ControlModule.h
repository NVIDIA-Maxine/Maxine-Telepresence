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

#ifndef SRC_MODULES_CONTROLMODULE_CONTROLMODULE_H_
#define SRC_MODULES_CONTROLMODULE_CONTROLMODULE_H_

#include <string>

#include "Core/Engine/Engine.h"
#include "Core/Events/KeyEvent.h"
#include "Core/Properties/Property.h"
#include "Modules/BehaviorModule/BehaviorModule.h"

// Export components and systems
#include "Components/CameraControlBehavior.h"
#include "Components/PoseCalibrationBehavior.h"
#include "Components/ViewExtensionBehavior.h"
#include "Systems/ApplicationControlSystem.h"

namespace nv3dvc {
namespace modules {
namespace controlmodule {

/// @defgroup ControlModuleProperties ControlModule
/// @ingroup  ModuleProperties
/// @brief    Behavior module defining components for controlling objects movement within the scene
///
/// Registers systems:
/// - nv3dvc::modules::controlmodule::systems::ApplicationControlSystem
///
/// Registers behaviors:
/// - nv3dvc::modules::controlmodule::components::CameraControlBehavior
/// - nv3dvc::modules::controlmodule::components::PoseCalibrationBehavior
/// - nv3dvc::modules::controlmodule::components::ViewExtensionBehavior

/// See @ref ControlModuleProperties
class ControlModule : public core::engine::Module {
 public:
  constexpr static const char* NAME = "ControlModule";
  std::string Name() const override { return NAME; };

  /// @brief Constructor
  /// @param[in,out] engine_control  Engine control fed to systems for controling application instance
  /// @param[in,out] behavior_module Behavior module used for registering behavior components
  ControlModule(core::engine::EngineControl* engine_control, behaviormodule::BehaviorModule* behavior_module);

  nv3dvc::core::Error Initialize() override;

  nv3dvc::core::Error Uninitialize() override;

  nv3dvc::core::Error Update(float dt) override { return nv3dvc::core::SUCCESS; }
};

}  // namespace controlmodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_CONTROLMODULE_CONTROLMODULE_H_
