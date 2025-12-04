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

#ifndef SRC_MODULES_TRACKINGMODULE_TRACKINGMODULE_H_
#define SRC_MODULES_TRACKINGMODULE_TRACKINGMODULE_H_

#include <string>

#include "Core/Engine/Module.h"

// Export components and systems
#include "Components/TrackedHeadComponent.h"
#include "Systems/HeadTrackingSystem.h"

namespace nv3dvc {
namespace modules {
namespace trackingmodule {

/// @defgroup TrackingModuleProperties TrackingModule
/// @ingroup  ModuleProperties
/// @brief    Module for head tracking
///
/// Registers the component
/// - nv3dvc::modules::trackingmodule::components::TrackedHeadComponent
///
/// Registers the system
/// - nv3dvc::modules::trackingmodule::systems::HeadTrackingSystem

/// See @ref TrackingModuleProperties
class TrackingModule : public core::engine::Module {
 public:
  constexpr static const char* NAME = "TrackingModule";
  std::string Name() const override { return "TrackingModule"; };

  explicit TrackingModule(core::engine::Engine* engine);

  nv3dvc::core::Error Initialize() override;

  nv3dvc::core::Error Uninitialize() override;

  nv3dvc::core::Error Update(float dt) override;

 private:
  core::engine::Engine* m_engine;
};

}  // namespace trackingmodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_TRACKINGMODULE_TRACKINGMODULE_H_
