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

#include "AudioFeedbackSystem.h"

#include <vector>

#include "Core/EntityComponentSystem/Entity.h"
#include "Core/Error.h"
#include "Modules/AudioModule/Components/AudioSinkComponent.h"
#include "Modules/AudioModule/Components/AudioSourceComponent.h"

namespace nv3dvc {
namespace modules {
namespace audiomodule {
namespace systems {

AudioFeedbackSystem::AudioFeedbackSystem() {}

AudioFeedbackSystem::~AudioFeedbackSystem() {}

core::Error AudioFeedbackSystem::OnLoadScene(core::ecs::registry::EntityRegistry* reg) {
  auto sink_source_entities =
      reg->view<audiomodule::components::AudioSinkComponent, audiomodule::components::AudioSourceComponent>();

  for (core::ecs::Entity& entity : sink_source_entities) {
    auto& sink_component = entity.GetComponent<audiomodule::components::AudioSinkComponent>();
    auto& source_component = entity.GetComponent<audiomodule::components::AudioSourceComponent>();
    source_component.Initialize(sink_component.GetSampleRate(), sink_component.GetNumChannels());
  }

  return core::SUCCESS;
}

core::Error AudioFeedbackSystem::Run(core::ecs::registry::EntityRegistry* reg, float dt) {
  auto sink_source_entities =
      reg->view<audiomodule::components::AudioSinkComponent, audiomodule::components::AudioSourceComponent>();

  // For each entity with an sink component and a source component, pop the frames from the sink component and push
  // them into the source component.
  std::vector<float> tmp_buffer;
  for (core::ecs::Entity& entity : sink_source_entities) {
    auto& sink_component = entity.GetComponent<audiomodule::components::AudioSinkComponent>();
    auto& source_component = entity.GetComponent<audiomodule::components::AudioSourceComponent>();
    // Grab up to 1 second of audio.
    const int max_samples = 1 * sink_component.GetSampleRate();
    tmp_buffer.reserve(max_samples * sink_component.GetNumChannels());
    const int num_samples = sink_component.PopSinkAudio(tmp_buffer.data(), max_samples, 1);
    source_component.PushSourceAudio(tmp_buffer.data(), num_samples);
  }

  return core::SUCCESS;
}

}  // namespace systems
}  // namespace audiomodule
}  // namespace modules
}  // namespace nv3dvc
