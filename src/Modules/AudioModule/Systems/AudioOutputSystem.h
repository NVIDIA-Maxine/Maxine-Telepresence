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

#ifndef SRC_MODULES_AUDIOMODULE_SYSTEMS_AUDIOOUTPUTSYSTEM_H_
#define SRC_MODULES_AUDIOMODULE_SYSTEMS_AUDIOOUTPUTSYSTEM_H_

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
namespace audiomodule {

namespace systems {

/// @defgroup AudioOutputSystemProperties AudioOutputSystem
/// @ingroup  SystemProperties
/// @brief    System for playing back audio
///
/// The system acts on entities with components components::AudioOutputComponent attached. During scene load, the system
/// will create a callback that an AudioOutputComponent can call to request audio data for playback. The callback will
/// iterate over all entities with AudioSourceComponent attached, pull audio samples from them, and mix all sources into
/// a single audio stream for the AudioOutputComponent to play.

/// See @ref AudioOutputSystemProperties
class AudioOutputSystem : public core::ecs::System {
 public:
  constexpr static const char* NAME = "AudioOutputSystem";
  std::string Name() const override { return NAME; };

  AudioOutputSystem();
  ~AudioOutputSystem() override;

  core::Error OnLoadScene(core::ecs::registry::EntityRegistry* reg) override;

  core::Error OnUnloadScene(core::ecs::registry::EntityRegistry* reg) override;

  core::Error Run(core::ecs::registry::EntityRegistry* reg, float dt) override;

 private:
};

}  // namespace systems
}  // namespace audiomodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_AUDIOMODULE_SYSTEMS_AUDIOOUTPUTSYSTEM_H_
