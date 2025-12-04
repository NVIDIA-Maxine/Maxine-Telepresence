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

#include "CodecModule.h"

#include "Core/Serialization/Serialization.h"

namespace nv3dvc {
namespace modules {
namespace codecmodule {

NLOHMANN_JSON_SERIALIZE_ENUM(OpusMode, {{OpusMode::VOIP, "VOIP"},
                                        {OpusMode::AUDIO, "AUDIO"},
                                        {OpusMode::RESTRICTED_LOWDELAY, "RESTRICTED_LOWDELAY"}});

CodecModule::CodecModule(core::engine::Engine* engine) {
  RegisterComponent<components::AudioDecoderComponent>();
  RegisterComponent<components::AudioEncoderComponent>();
  RegisterComponent<components::FrameDecoderComponent>();
  RegisterComponent<components::FrameEncoderComponent>();
  RegisterSystem<systems::DecoderSystem>(engine);
  RegisterSystem<systems::EncoderSystem>(engine);
}

core::Error CodecModule::Initialize() { return core::Error::SUCCESS; }

core::Error CodecModule::Uninitialize() { return core::Error::SUCCESS; }

core::Error CodecModule::Update(float dt) { return core::Error::SUCCESS; }

core::Error CodecModule::EncodeProperties(nlohmann::json* json_description, const PropertyOwner* property_owner) const {
  core::Error err = core::Error::SUCCESS;
  CHECK_SUCCESS(core::serialization::EncodeProperties<OpusMode>(json_description, property_owner));
bail:
  return err;
}

core::Error CodecModule::DecodeProperties(const nlohmann::json& json_description, PropertyOwner* property_owner) const {
  core::Error err = core::Error::SUCCESS;
  CHECK_SUCCESS(core::serialization::DecodeProperties<OpusMode>(json_description, property_owner));
bail:
  return err;
}

}  // namespace codecmodule
}  // namespace modules
}  // namespace nv3dvc
