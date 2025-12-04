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

#include "NetworkModule.h"

#include <gst/gst.h>

#include "Core/Serialization/Serialization.h"

namespace nv3dvc {
namespace modules {
namespace networkmodule {

NLOHMANN_JSON_SERIALIZE_ENUM(StreamSinkType, {{StreamSinkType::UNKNOWN, "UNKNOWN"},
                                              {StreamSinkType::RTP_UDP_SENDER, "RTP_UDP_SENDER"},
                                              {StreamSinkType::RTSP_SERVER, "RTSP_SERVER"}});

NetworkModule::NetworkModule() {
  RegisterComponent<components::StreamSinkComponent>();
  RegisterComponent<components::StreamSourceComponent>();
  RegisterSystem<systems::AudioReceiverSystem>();
  RegisterSystem<systems::AudioSenderSystem>();
  RegisterSystem<systems::ReceiverSystem>();
  RegisterSystem<systems::SenderSystem>();
}

core::Error NetworkModule::Initialize() {
  // Initialize gstreamer
  if (!m_gstInitialized) {
    gst_init(nullptr, nullptr);
    m_gstInitialized = true;
  }
  return m_gstInitialized ? core::Error::SUCCESS : core::Error::ERR_GENERAL;
}

core::Error NetworkModule::Uninitialize() { return core::Error::SUCCESS; }

core::Error NetworkModule::Update(float dt) { return core::Error::SUCCESS; }

core::Error NetworkModule::EncodeProperties(nlohmann::json* json_description,
                                            const PropertyOwner* property_owner) const {
  core::Error err = core::Error::SUCCESS;
  CHECK_SUCCESS(core::serialization::EncodeProperties<StreamSinkType>(json_description, property_owner));
bail:
  return err;
}

core::Error NetworkModule::DecodeProperties(const nlohmann::json& json_description,
                                            PropertyOwner* property_owner) const {
  core::Error err = core::Error::SUCCESS;
  CHECK_SUCCESS(core::serialization::DecodeProperties<StreamSinkType>(json_description, property_owner));
bail:
  return err;
}

}  // namespace networkmodule
}  // namespace modules
}  // namespace nv3dvc
