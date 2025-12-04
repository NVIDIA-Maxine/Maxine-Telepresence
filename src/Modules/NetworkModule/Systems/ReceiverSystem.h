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

#ifndef SRC_MODULES_NETWORKMODULE_SYSTEMS_RECEIVERSYSTEM_H_
#define SRC_MODULES_NETWORKMODULE_SYSTEMS_RECEIVERSYSTEM_H_

#include <chrono>  // NOLINT(build/c++11) (cpplint outside Chromium: https://github.com/google/styleguide/issues/483)
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

/// @defgroup ReceiverSystemProperties ReceiverSystem
/// @ingroup  SystemProperties
/// @brief    System for receiving video data over the network
///
/// This system acts on entities that contain a components::StreamSourceComponent and either
/// commonmodule::components::VideoFrameComponent or commonmodule::components::EncodedVideoCallbackComponent. During
/// run, each components::StreamSourceComponent will be polled to see if any new video data has been received. If there
/// is new video data, it will be passed to the commonmodule::components::VideoFrameComponent or
/// commonmodule::components::EncodedVideoCallbackComponent on the same entity.
///
/// Incoming video data may only be passed to a commonmodule::components::VideoFrameComponent if
/// components::StreamSourceComponent.decode_video_frames is `true`.
///
/// Incoming video data may only be passed to a commonmodule::components::EncodedVideoCallbackComponent if
/// components::StreamSourceComponent.decode_video_frames is `false`.

/// See @ref ReceiverSystemProperties
class ReceiverSystem : public core::ecs::System {
 public:
  constexpr static const char* NAME = "ReceiverSystem";
  std::string Name() const override { return NAME; }

  ReceiverSystem();

  core::Error Initialize() override;

  core::Error Uninitialize() override;

  core::Error OnLoadScene(core::ecs::registry::EntityRegistry* reg) override;

  core::Error Run(core::ecs::registry::EntityRegistry* reg, float dt) override;

 private:
  float m_deltaTime = 0.0f;  // Updated at the start of every run. Used as user data for decoder callback
  // We can set m_nextPoll if we need to throttle the receiver loop.
  std::chrono::steady_clock::time_point m_nextPoll = std::chrono::steady_clock::time_point::min();
};

}  // namespace systems
}  // namespace networkmodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_NETWORKMODULE_SYSTEMS_RECEIVERSYSTEM_H_
