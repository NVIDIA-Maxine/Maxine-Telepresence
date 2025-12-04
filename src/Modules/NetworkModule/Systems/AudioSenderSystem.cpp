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

#include "AudioSenderSystem.h"

#include <vector>

#include "Core/EntityComponentSystem/Entity.h"
#include "Core/Error.h"
#include "Modules/CodecModule/Components/AudioEncoderComponent.h"
#include "Modules/NetworkModule/Components/StreamSinkComponent.h"

namespace nv3dvc {
namespace modules {
namespace networkmodule {
namespace systems {

AudioSenderSystem::AudioSenderSystem() {}

core::Error AudioSenderSystem::Initialize() { return core::SUCCESS; }

core::Error AudioSenderSystem::Uninitialize() { return core::SUCCESS; }

core::Error AudioSenderSystem::OnLoadScene(core::ecs::registry::EntityRegistry* reg) {
  // Initialize all sinks.
  auto entities = reg->view<components::StreamSinkComponent, codecmodule::components::AudioEncoderComponent>();
  for (auto& entity : entities) {
    auto& stream_sink = entity.GetComponent<components::StreamSinkComponent>();
    if (!stream_sink.IsInitialized()) stream_sink.Initialize();
  }
  return core::SUCCESS;
}

core::Error AudioSenderSystem::Run(core::ecs::registry::EntityRegistry* reg, float dt) {
  auto entities = reg->view<components::StreamSinkComponent, codecmodule::components::AudioEncoderComponent>();
  for (auto& entity : entities) {
    auto& stream_sink = entity.GetComponent<components::StreamSinkComponent>();
    auto& audio_encoder = entity.GetComponent<codecmodule::components::AudioEncoderComponent>();
    if (stream_sink.send_audio) {
      std::vector<std::vector<unsigned char>> packets;
      audio_encoder.EncodeSamples(&packets);

      for (const auto& data_buffer : packets) {
        stream_sink.SendAudioData(data_buffer.data(), data_buffer.size());
      }
    }
  }
  return core::SUCCESS;
}

}  // namespace systems
}  // namespace networkmodule
}  // namespace modules
}  // namespace nv3dvc
