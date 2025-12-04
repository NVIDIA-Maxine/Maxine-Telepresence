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

#ifndef SRC_MODULES_AUDIOMODULE_AUDIOMODULE_H_
#define SRC_MODULES_AUDIOMODULE_AUDIOMODULE_H_

#include <string>

#include "Core/Engine/Module.h"

// Export components and systems
#include "Core/Error.h"
#include "Modules/AudioModule/Components/AudioInputComponent.h"
#include "Modules/AudioModule/Components/AudioOutputComponent.h"
#include "Modules/AudioModule/Components/AudioSinkComponent.h"
#include "Modules/AudioModule/Components/AudioSourceComponent.h"
#include "Modules/AudioModule/Systems/AudioFeedbackSystem.h"
#include "Modules/AudioModule/Systems/AudioInputSystem.h"
#include "Modules/AudioModule/Systems/AudioOutputSystem.h"

namespace nv3dvc {
namespace modules {
namespace audiomodule {

/// @defgroup AudioModuleProperties AudioModule
/// @ingroup  ModuleProperties
/// @brief    Module defining components for recording and playing sound
///
/// Registers systems:
/// - nv3dvc::modules::audiomodule::systems::AudioInputSystem
/// - nv3dvc::modules::audiomodule::systems::AudioOutputSystem
/// - nv3dvc::modules::audiomodule::systems::AudioFeedbackSystem
///
/// Registers components:
/// - nv3dvc::modules::audiomodule::components::AudioInputComponent
/// - nv3dvc::modules::audiomodule::components::AudioOutputComponent
/// - nv3dvc::modules::audiomodule::components::AudioSinkComponent
/// - nv3dvc::modules::audiomodule::components::AudioSourceComponent

/// See @ref AudioModuleProperties
class AudioModule : public core::engine::Module {
 public:
  constexpr static const char* NAME = "AudioModule";
  std::string Name() const override { return NAME; };

  /// @brief Constructor
  AudioModule();

  core::Error Initialize() override;

  core::Error Uninitialize() override;

  core::Error Update(float dt) override;

  core::Error EncodeProperties(nlohmann::json* json_description, const PropertyOwner* property_owner) const override;

  core::Error DecodeProperties(const nlohmann::json& json_description, PropertyOwner* property_owner) const override;

 private:
  bool m_isInitialized = false;
};

}  // namespace audiomodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_AUDIOMODULE_AUDIOMODULE_H_
