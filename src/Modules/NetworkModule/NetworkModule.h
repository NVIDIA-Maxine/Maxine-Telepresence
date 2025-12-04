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

#ifndef SRC_MODULES_NETWORKMODULE_NETWORKMODULE_H_
#define SRC_MODULES_NETWORKMODULE_NETWORKMODULE_H_

#include <string>

#include "Core/Engine/Module.h"

// Export components and systems
#include "Components/StreamSinkComponent.h"
#include "Components/StreamSourceComponent.h"
#include "Systems/AudioReceiverSystem.h"
#include "Systems/AudioSenderSystem.h"
#include "Systems/ReceiverSystem.h"
#include "Systems/SenderSystem.h"

namespace nv3dvc {
namespace modules {
namespace networkmodule {

/// @defgroup NetworkModuleProperties NetworkModule
/// @ingroup  ModuleProperties
/// @brief    Module for sending and receiving data over the network.
///
/// Registers systems:
/// - nv3dvc::modules::networkmodule::systems::AudioReceiverSystem
/// - nv3dvc::modules::networkmodule::systems::AudioSenderSystem
/// - nv3dvc::modules::networkmodule::systems::ReceiverSystem
/// - nv3dvc::modules::networkmodule::systems::SenderSystem
///
/// Registers components:
/// - nv3dvc::modules::networkmodule::components::StreamSinkComponent
/// - nv3dvc::modules::networkmodule::components::StreamSourceComponent

/// See @ref  NetworkModuleProperties
class NetworkModule : public core::engine::Module {
 public:
  constexpr static const char* NAME = "NetworkModule";
  std::string Name() const override { return NAME; }

  NetworkModule();

  core::Error Initialize() override;

  core::Error Uninitialize() override;

  core::Error Update(float dt) override;

  core::Error EncodeProperties(nlohmann::json* json_description, const PropertyOwner* property_owner) const override;

  core::Error DecodeProperties(const nlohmann::json& json_description, PropertyOwner* property_owner) const override;

 private:
  bool m_gstInitialized = false;
};

}  // namespace networkmodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_NETWORKMODULE_NETWORKMODULE_H_
