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

#include "AudioReceiverSystem.h"

#include "Core/EntityComponentSystem/Entity.h"
#include "Core/Error.h"
#include "Core/Util/Logger.h"
#include "Modules/CodecModule/Components/AudioDecoderComponent.h"
#include "Modules/NetworkModule/Components/StreamSourceComponent.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Macros                                                                                                           ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace nv3dvc {
namespace modules {
namespace networkmodule {
namespace systems {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Static function declarations                                                                                     ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Static function definitions                                                                                      ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Member function definitions                                                                                      ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

AudioReceiverSystem::AudioReceiverSystem() {}

core::Error AudioReceiverSystem::Initialize() { return core::SUCCESS; }

core::Error AudioReceiverSystem::Uninitialize() { return core::SUCCESS; }

core::Error AudioReceiverSystem::OnLoadScene(core::ecs::registry::EntityRegistry* reg) {
  // Initialize all sources
  auto entities = reg->view<components::StreamSourceComponent, codecmodule::components::AudioDecoderComponent>();
  for (auto& entity : entities) {
    auto& stream_source = entity.GetComponent<components::StreamSourceComponent>();
    auto& audio_decoder = entity.GetComponent<codecmodule::components::AudioDecoderComponent>();
    if (!stream_source.IsInitialized()) stream_source.Initialize();
    // Check if an audio callback has been added.
    if (stream_source.receive_audio) {
      stream_source.SetAudioCallback(
          [&audio_decoder](uint8_t* metadata_payload, const size_t metadata_size, uint8_t* buffer, const size_t size) {
            audio_decoder.DecodeSamples(buffer, size);
          });
    }
  }
  return core::SUCCESS;
}

core::Error AudioReceiverSystem::Run(core::ecs::registry::EntityRegistry* reg, float dt) {
  auto entities = reg->view<components::StreamSourceComponent, codecmodule::components::AudioDecoderComponent>();

  for (auto& entity : reg->view<components::StreamSourceComponent>()) {
    auto& stream_source = entity.GetComponent<components::StreamSourceComponent>();
    if (stream_source.receive_audio) stream_source.PollAudioAppSink();
  }
  return core::SUCCESS;
}

}  // namespace systems
}  // namespace networkmodule
}  // namespace modules
}  // namespace nv3dvc
