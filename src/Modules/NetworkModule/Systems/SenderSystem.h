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

#ifndef SRC_MODULES_NETWORKMODULE_SYSTEMS_SENDERSYSTEM_H_
#define SRC_MODULES_NETWORKMODULE_SYSTEMS_SENDERSYSTEM_H_

#include <string>

#include "Core/EntityComponentSystem/System.h"

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

/// @defgroup SenderSystemProperties SenderSystem
/// @ingroup  SystemProperties
/// @brief    System for receiving video data over the network
///
/// This system acts on entities that contain a components::StreamSinkComponent and
/// commonmodule::components::EncodedVideoBufferComponent. During run, each
/// commonmodule::components::EncodedVideoBufferComponent will be polled to see if any new video packets can be sent. If
/// there are new video packets, they will be passed to components::StreamSinkComponent on the same entity.

/// See @ref SenderSystemProperties
class SenderSystem : public core::ecs::System {
 public:
  constexpr static const char* NAME = "SenderSystem";
  std::string Name() const override { return NAME; }

  SenderSystem();

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

#endif  // SRC_MODULES_NETWORKMODULE_SYSTEMS_SENDERSYSTEM_H_
