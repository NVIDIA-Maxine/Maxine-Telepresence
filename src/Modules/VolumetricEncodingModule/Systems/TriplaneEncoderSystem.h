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

#ifndef SRC_MODULES_VOLUMETRICENCODINGMODULE_SYSTEMS_TRIPLANEENCODERSYSTEM_H_
#define SRC_MODULES_VOLUMETRICENCODINGMODULE_SYSTEMS_TRIPLANEENCODERSYSTEM_H_

#include <string>

#include "Core/Engine/Engine.h"
#include "Core/EntityComponentSystem/Component.h"
#include "Core/EntityComponentSystem/System.h"
#include "Core/Properties/Property.h"
#include "glm/glm.hpp"
#include "nvAR.h"

namespace nv3dvc {

namespace core {
namespace ecs {
namespace registry {
class EntityRegistry;
}  // namespace registry
}  // namespace ecs
}  // namespace core

namespace modules {
namespace volumetricencodingmodule {
namespace systems {

/// @defgroup TriplaneEncoderSystemProperties TriplaneEncoderSystem
/// @ingroup  SystemProperties
/// @brief    System for processing video frames to generate volumetric triplane objects
///
/// The system acts on entities with one commonmodule::components::VideoFrameComponent and one
/// triplanemodule::components::EncodedTriplaneComponent component attached. Entities which apply, and which also have a
/// trackingmodule::components::TrackedHeadComponent attached to their parent will have their tracked head pose
/// information forwarded to the triplane package generated during run. Additionally, there should be one
/// triplanemodule::components::TriplaneBufferComponent attached to any entity in the scene. During run, the system will
/// read from the commonmodule::components::VideoFrameComponent, run volumetric encoding, and push the result to the
/// TriplaneBufferComponent.

/// See @ref TriplaneEncoderSystemProperties
class TriplaneEncoderSystem : public core::ecs::System {
 public:
  constexpr static const char* NAME = "TriplaneEncoderSystem";
  std::string Name() const override { return "TriplaneEncoderSystem"; };

  TriplaneEncoderSystem();

  core::Error Initialize() override;

  core::Error Uninitialize() override;

  core::Error OnLoadScene(core::ecs::registry::EntityRegistry* reg) override;

  core::Error Run(core::ecs::registry::EntityRegistry* reg, float dt) override;

 public:
  /// @ingroup TriplaneEncoderSystemProperties
  /// @{
  core::properties::Property<std::string> ar_sdk_model_dir = {
      this,
      "ar_sdk_model_dir",
      "Path to AR SDK model folder. The default value will be determined based on the environment variable ARSDK "
      "which should be set before running the engine. See README.md for details on setting up environment variables.",
      core::engine::Engine::GetArSdkDir() + "bin/models/",
  };
  /// @}
 private:
  NvAR_FeatureHandle m_encoderHandle;
  NvCVImage m_croppedHeadImageRgbPlanar;
  NvCVImage m_tmpImg;
  float m_outputFocalScale;
  glm::mat4 m_modelOutputPose;
};

}  // namespace systems
}  // namespace volumetricencodingmodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_VOLUMETRICENCODINGMODULE_SYSTEMS_TRIPLANEENCODERSYSTEM_H_
