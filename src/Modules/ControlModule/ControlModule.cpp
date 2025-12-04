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

#include "ControlModule.h"

namespace nv3dvc {
namespace modules {
namespace controlmodule {

ControlModule::ControlModule(core::engine::EngineControl* engine_control,
                             behaviormodule::BehaviorModule* behavior_module) {
  RegisterSystem<systems::ApplicationControlSystem>(engine_control);
  behavior_module->RegisterBehaviorComponent<components::CameraControlBehavior>();
  behavior_module->RegisterBehaviorComponent<components::PoseCalibrationBehavior>();
  behavior_module->RegisterBehaviorComponent<components::ViewExtensionBehavior>();
}

nv3dvc::core::Error ControlModule::Initialize() { return nv3dvc::core::SUCCESS; }

nv3dvc::core::Error ControlModule::Uninitialize() { return nv3dvc::core::SUCCESS; }

}  // namespace controlmodule
}  // namespace modules
}  // namespace nv3dvc
