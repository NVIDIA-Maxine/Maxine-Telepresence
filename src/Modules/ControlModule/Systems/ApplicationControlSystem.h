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

#ifndef SRC_MODULES_CONTROLMODULE_SYSTEMS_APPLICATIONCONTROLSYSTEM_H_
#define SRC_MODULES_CONTROLMODULE_SYSTEMS_APPLICATIONCONTROLSYSTEM_H_

#include <string>

#include "Core/Engine/Engine.h"
#include "Core/EntityComponentSystem/System.h"

namespace nv3dvc {
namespace modules {
namespace controlmodule {
namespace systems {

/// @defgroup ApplicationControlSystemProperties ApplicationControlSystem
/// @ingroup  SystemProperties
/// @brief    System for high level application control
///
/// This system enables exiting the application using the escape key

/// See @ref ApplicationControlSystemProperties
class ApplicationControlSystem : public core::ecs::System {
 public:
  constexpr static const char* NAME = "ApplicationControlSystem";
  std::string Name() const override { return NAME; }

  explicit ApplicationControlSystem(core::engine::EngineControl* engine_control);

  core::Error Initialize() override;
  core::Error OnEvent(core::ecs::registry::EntityRegistry* reg, core::events::Event* e) override;
  core::Error Run(core::ecs::registry::EntityRegistry* reg, float dt) override;

 private:
  core::engine::EngineControl* m_engineControl;
};

}  // namespace systems
}  // namespace controlmodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_CONTROLMODULE_SYSTEMS_APPLICATIONCONTROLSYSTEM_H_
