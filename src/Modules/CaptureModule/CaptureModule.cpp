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

#include "CaptureModule.h"

#include <gst/gst.h>

#include "Core/Serialization/Serialization.h"

namespace nv3dvc {
namespace modules {
namespace capturemodule {
namespace capturedevice {
NLOHMANN_JSON_SERIALIZE_ENUM(CaptureApi, {{CaptureApi::UNKNOWN, "UNKNOWN"},
                                          {CaptureApi::OPENCV_WEBCAM, "OPENCV_WEBCAM"},
                                          {CaptureApi::NV_WEBCAM, "NV_WEBCAM"},
                                          {CaptureApi::VIDEO, "VIDEO"},
                                          {CaptureApi::MULTIMEDIA_FILE, "MULTIMEDIA_FILE"}});
}  // namespace capturedevice

CaptureModule::CaptureModule(core::engine::Engine* engine, behaviormodule::BehaviorModule* behavior_module) {
  RegisterSystem<systems::CameraCaptureSystem>(engine);
  RegisterComponent<components::WebCameraComponent>();
  behavior_module->RegisterBehaviorComponent<components::RecordingBehavior>();
}

core::Error CaptureModule::Initialize() {
  // Initialize gstreamer
  if (!m_gstInitialized) {
    gst_init(nullptr, nullptr);
    m_gstInitialized = true;
  }
  return m_gstInitialized ? core::Error::SUCCESS : core::Error::ERR_GENERAL;
}

core::Error CaptureModule::Uninitialize() { return core::SUCCESS; }

core::Error CaptureModule::Update(float dt) { return core::SUCCESS; }

core::Error CaptureModule::EncodeProperties(nlohmann::json* json_description,
                                            const PropertyOwner* property_owner) const {
  core::Error err = core::Error::SUCCESS;
  CHECK_SUCCESS(core::serialization::EncodeProperties<capturedevice::CaptureApi>(json_description, property_owner));
bail:
  return err;
}

core::Error CaptureModule::DecodeProperties(const nlohmann::json& json_description,
                                            PropertyOwner* property_owner) const {
  core::Error err = core::Error::SUCCESS;
  CHECK_SUCCESS(core::serialization::DecodeProperties<capturedevice::CaptureApi>(json_description, property_owner));
bail:
  return err;
}

}  // namespace capturemodule
}  // namespace modules
}  // namespace nv3dvc
