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

#ifndef SRC_MODULES_CAPTUREMODULE_SYSTEMS_CAMERACAPTURESYSTEM_H_
#define SRC_MODULES_CAPTUREMODULE_SYSTEMS_CAMERACAPTURESYSTEM_H_

#include <memory>
#include <string>

#include "Core/Engine/Engine.h"
#include "Core/EntityComponentSystem/System.h"
#include "nvCVImage.h"

namespace nv3dvc {

namespace core {
namespace ecs {
namespace registry {
class EntityRegistry;
}  // namespace registry
}  // namespace ecs
}  // namespace core

namespace modules {
namespace capturemodule {
namespace systems {

/// @defgroup CameraCaptureSystemProperties CameraCaptureSystem
/// @ingroup  SystemProperties
/// @brief    System for capturing camera streams and writing to video frames
///
/// During initialization, a camera or file stream will be opened for all instances of
/// nv3dvc::modules::capturemodule::components::WebCameraComponent
///
/// Acts on entities with both of the following components attached:
/// - nv3dvc::modules::capturemodule::components::WebCameraComponent
/// - nv3dvc::modules::commonmodule::components::VideoFrameComponent
///
/// For all entities that fulfill the above, frames will be read from the web camera component's API and written to the
/// video frame component.

/// See @ref CameraCaptureSystemProperties
class CameraCaptureSystem : public core::ecs::System {
 public:
  constexpr static const char* NAME = "CameraCaptureSystem";
  std::string Name() const override { return "CameraCaptureSystem"; };

  /// @brief Constructor
  /// @param[in] engine The engine. Used to initialize camera capture on the engine's CUDA context
  explicit CameraCaptureSystem(core::engine::Engine* engine);

  nv3dvc::core::Error OnLoadScene(core::ecs::registry::EntityRegistry* reg) override;
  nv3dvc::core::Error Initialize() override;
  nv3dvc::core::Error Uninitialize() override;

  nv3dvc::core::Error Run(core::ecs::registry::EntityRegistry* reg, float dt) override;

 private:
  core::engine::Engine* m_engine;
  NvCVImage m_playbackFrame;
  NvCVImage m_tmpImage;
};

}  // namespace systems
}  // namespace capturemodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_CAPTUREMODULE_SYSTEMS_CAMERACAPTURESYSTEM_H_
