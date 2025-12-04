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

#ifndef SRC_MODULES_CODECMODULE_CODECMODULE_H_
#define SRC_MODULES_CODECMODULE_CODECMODULE_H_

#include <string>

#include "Core/Engine/Engine.h"
#include "Core/Engine/Module.h"

// Export components and systems
#include "Components/AudioDecoderComponent.h"
#include "Components/AudioEncoderComponent.h"
#include "Components/FrameDecoderComponent.h"
#include "Components/FrameEncoderComponent.h"
#include "Systems/DecoderSystem.h"
#include "Systems/EncoderSystem.h"

namespace nv3dvc {
namespace modules {
namespace codecmodule {

/// @defgroup CodecModuleProperties CodecModule
/// @ingroup  ModuleProperties
/// @brief    Module for registering common components and systems used for audio and video encoding and decoding
///
/// Registers components:
/// - components::AudioDecoderComponent
/// - components::AudioEncoderComponent
/// - components::FrameDecoderComponent
/// - components::FrameEncoderComponent
///
/// Registers systems:
/// - systems::DecoderSystem
/// - systems::EncoderSystem

/// See @ref CodecModuleProperties
class CodecModule : public core::engine::Module {
 public:
  constexpr static const char* NAME = "CodecModule";
  std::string Name() const override { return NAME; };

  /// @brief Constructor
  /// @param[in] engine The engine. Used for getting the CUDA context
  explicit CodecModule(core::engine::Engine* engine);

  core::Error Initialize() override;
  core::Error Uninitialize() override;
  core::Error Update(float dt) override;
  core::Error EncodeProperties(nlohmann::json* json_description, const PropertyOwner* property_owner) const override;
  core::Error DecodeProperties(const nlohmann::json& json_description, PropertyOwner* property_owner) const override;
};

}  // namespace codecmodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_CODECMODULE_CODECMODULE_H_
