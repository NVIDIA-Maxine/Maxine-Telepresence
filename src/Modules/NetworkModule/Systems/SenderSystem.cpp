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

#include "SenderSystem.h"

#include "Core/EntityComponentSystem/Entity.h"
#include "Modules/CommonModule/Components/DataBufferComponent.h"
#include "Modules/NetworkModule/Components/StreamSinkComponent.h"
#include "uuid.h"
namespace nv3dvc {
namespace modules {
namespace networkmodule {
namespace systems {

SenderSystem::SenderSystem() {}

core::Error SenderSystem::Initialize() { return core::Error::SUCCESS; }

core::Error SenderSystem::Uninitialize() { return core::Error::SUCCESS; }

core::Error SenderSystem::OnLoadScene(core::ecs::registry::EntityRegistry* reg) {
  core::Error err = core::Error::SUCCESS;
  for (auto& entity : reg->view<components::StreamSinkComponent>()) {
    auto& stream_sink = entity.GetComponent<components::StreamSinkComponent>();
    if (!stream_sink.IsInitialized()) {
      err = stream_sink.Initialize();
      BAIL_IF_ERR(err);
    }
  }
bail:
  return err;
}

core::Error SenderSystem::Run(core::ecs::registry::EntityRegistry* reg, float dt) {
  core::Error err = core::Error::SUCCESS;
  for (auto& entity : reg->view<commonmodule::components::EncodedVideoBufferComponent,  //
                                components::StreamSinkComponent>()) {
    auto& stream_sink = entity.GetComponent<components::StreamSinkComponent>();
    auto& data_buffer_component = entity.GetComponent<commonmodule::components::EncodedVideoBufferComponent>();

    for (const auto& data_buffer : data_buffer_component.GetBuffers()) {
      core::Error e = stream_sink.SendVideoData(data_buffer.data(), data_buffer.size());
      if (err == core::Error::SUCCESS) err = e;  // Save first error
    }
    data_buffer_component.Consume();
  }
  return err;
}

}  // namespace systems
}  // namespace networkmodule
}  // namespace modules
}  // namespace nv3dvc
