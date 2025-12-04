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

#ifndef SRC_MODULES_CODECMODULE_SYSTEMS_DECODERSYSTEM_H_
#define SRC_MODULES_CODECMODULE_SYSTEMS_DECODERSYSTEM_H_

#include <string>

#include "Core/Engine/Engine.h"
#include "Core/EntityComponentSystem/System.h"
#include "Core/Properties/Property.h"
#include "Modules/CodecModule/VideoDecoder.h"
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

/// @defgroup DecoderSystemProperties DecoderSystem
/// @ingroup  SystemProperties
/// @brief    System for decoding incoming video and audio packets
///
/// Will during scene load, initialize the following components if not already initialized:
/// - triplanemodule::components::TriplaneBufferComponent
/// - components::FrameDecoderComponent
/// - commonmodule::components::EncodedVideoCallbackComponent
///
/// Packet decoding does not happen in the Run function of the decoder system, instead, the systems sets up callbacks
/// when the following conditions are met:
/// - components::FrameDecoderComponent, and commonmodule::components::EncodedVideoCallbackComponent exists:
///   - Set up callback for decoding into triplanemodule::components::TriplaneBufferComponent
///   - Set up callback for decoding into commonmodule::components::VideoFrameComponent if the triplane buffer component
///   doesn't exist
/// - codecmodule::components::AudioDecoderComponent, and audiomodule::components::AudioSourceComponent exists:
///   - Set up callback for decoding into audiomodule::components::AudioSourceComponent
///
/// Callbacks are called by other systems (e.g. networkmodule::systems::ReceiverSystem) when packets arrive

/// See @ref DecoderSystemProperties
class DecoderSystem : public core::ecs::System {
 public:
  constexpr static const char* NAME = "DecoderSystem";
  std::string Name() const override { return NAME; };

  /// @brief Constructor
  /// @param[in] engine The engine. Used for getting the CUDA context
  explicit DecoderSystem(core::engine::Engine* engine);

  core::Error Initialize() override;
  core::Error Uninitialize() override;
  core::Error OnLoadScene(core::ecs::registry::EntityRegistry* reg) override;
  core::Error Run(core::ecs::registry::EntityRegistry* reg, float dt) override;

 private:
  core::engine::Engine* m_engine;
  NvCVImage m_tmpImage;
};

}  // namespace systems
}  // namespace codecmodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_CODECMODULE_SYSTEMS_DECODERSYSTEM_H_
