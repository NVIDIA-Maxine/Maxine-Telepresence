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

#ifndef SRC_MODULES_CODECMODULE_SYSTEMS_ENCODERSYSTEM_H_
#define SRC_MODULES_CODECMODULE_SYSTEMS_ENCODERSYSTEM_H_

#include <string>

#include "Core/Engine/Engine.h"
#include "Core/EntityComponentSystem/System.h"
#include "Core/Properties/Property.h"
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
namespace codecmodule {
namespace systems {

/// @defgroup EncoderSystemProperties EncoderSystem
/// @ingroup  SystemProperties
/// @brief    System for encoding outgoing video and audio packets
///
/// Will, during scene load, initialize the following components if both exist in any entity:
/// - audiomodule::components::AudioSinkComponent
/// - codecmodule::components::AudioEncoderComponent
///
/// Will, during run, initialize components::FrameEncoderComponent if the following exist in an entity:
/// - components::FrameEncoderComponent
/// - commonmodule::components::VideoFrameComponent
/// - commonmodule::components::EncodedVideoBufferComponent>()) {
///
/// Moreover, commonmodule::components::EncodedVideoBufferComponent will get pushed with new incoming frames after being
/// encoded. If a trackingmodule::components::TrackedHeadComponent in the same entity, tracked head data will be pushed
/// to the SourceVideoPackage as a serialized json object in the SEI metadata for the packet.
/// The commonmodule::components::EncodedVideoBufferComponent can be used for reading packets in a network sender.

/// See @ref EncoderSystemProperties
class EncoderSystem : public core::ecs::System {
 public:
  constexpr static const char* NAME = "EncoderSystem";
  std::string Name() const override { return "EncoderSystem"; };

  /// @brief Constructor
  /// @param[in] engine The engine. Used for getting the CUDA context
  explicit EncoderSystem(core::engine::Engine* engine);

  core::Error Initialize() override;
  core::Error Uninitialize() override;
  core::Error OnLoadScene(core::ecs::registry::EntityRegistry* reg) override;
  core::Error Run(core::ecs::registry::EntityRegistry* reg, float dt) override;

 private:
  core::engine::Engine* m_engine;
  NvCVImage m_tmpImg;
};

}  // namespace systems
}  // namespace codecmodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_CODECMODULE_SYSTEMS_ENCODERSYSTEM_H_
