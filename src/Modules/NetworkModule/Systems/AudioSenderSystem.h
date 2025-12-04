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

#ifndef SRC_MODULES_NETWORKMODULE_SYSTEMS_AUDIOSENDERSYSTEM_H_
#define SRC_MODULES_NETWORKMODULE_SYSTEMS_AUDIOSENDERSYSTEM_H_

#include <string>

#include "Core/EntityComponentSystem/System.h"
#include "Core/Error.h"

namespace nv3dvc {

namespace core {
namespace ecs {
namespace registry {
class EntityRegistry;
}  // namespace registry
}  // namespace ecs
}  // namespace core

namespace modules {
namespace networkmodule {
namespace systems {

/// @defgroup AudioSenderSystemProperties AudioSenderSystem
/// @ingroup  SystemProperties
/// @brief    System for receiving audio data over the network
///
/// This system acts on entities that contain a components::StreamSinkComponent and
/// codecmodule::components::AudioEncoderComponent. During run, each codecmodule::components::AudioEncoderComponent will
/// be polled to see if any new audio data can be encoded. If there is new audio data, it will be encoded and passed to
/// the components::StreamSinkComponent on the same entity.

/// See @ref AudioSenderSystemProperties
class AudioSenderSystem : public core::ecs::System {
 public:
  constexpr static const char* NAME = "AudioSenderSystem";
  std::string Name() const override { return NAME; }

  AudioSenderSystem();

  core::Error Initialize() override;

  core::Error Uninitialize() override;

  core::Error OnLoadScene(core::ecs::registry::EntityRegistry* reg) override;

  core::Error Run(core::ecs::registry::EntityRegistry* reg, float dt) override;

 private:
};

}  // namespace systems
}  // namespace networkmodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_NETWORKMODULE_SYSTEMS_AUDIOSENDERSYSTEM_H_
