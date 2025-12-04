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

#ifndef SRC_MODULES_AUDIOMODULE_SYSTEMS_AUDIOFEEDBACKSYSTEM_H_
#define SRC_MODULES_AUDIOMODULE_SYSTEMS_AUDIOFEEDBACKSYSTEM_H_

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

/// @defgroup AudioFeedbackSystemProperties AudioFeedbackSystem
/// @ingroup  SystemProperties
/// @brief    System for capturing audio
///
/// The system acts on entities with components components::AudioSinkComponent and components::AudioSourceComponent
/// attached. During run, audio will be popped from the AudioSinkComponent and pushed to the AudioSourceComponent. This
/// allows direct playback of audio that is captured locally.

/// See @ref AudioFeedbackSystemProperties
class AudioFeedbackSystem : public core::ecs::System {
 public:
  constexpr static const char* NAME = "AudioFeedbackSystem";
  std::string Name() const override { return NAME; };

  AudioFeedbackSystem();
  ~AudioFeedbackSystem() override;

  core::Error OnLoadScene(core::ecs::registry::EntityRegistry* reg) override;

  core::Error Run(core::ecs::registry::EntityRegistry* reg, float dt) override;

 private:
};

}  // namespace systems
}  // namespace audiomodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_AUDIOMODULE_SYSTEMS_AUDIOFEEDBACKSYSTEM_H_
