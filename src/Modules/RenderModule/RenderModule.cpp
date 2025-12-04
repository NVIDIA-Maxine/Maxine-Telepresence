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

#include "RenderModule.h"

#include "Components/DisplayComponent.h"
#include "Components/RenderableTriplaneComponent.h"
#include "Core/Error.h"
#include "Core/Serialization/Serialization.h"
#include "Systems/RenderSystem.h"

/// @brief NvCVRenderVolume error callback function
/// @param[in] user_data Renderer pointer
/// @param msg The error message
static void RenderVolumeErrorCallback(void* user_data, const char* msg) {
  auto* engine = static_cast<nv3dvc::core::engine::Engine*>(user_data);
  if (msg) {
    if (engine->log_level == NVCV_LOG_FATAL) {
      LOG_FATAL("%s", msg);
    }
    if (engine->log_level == NVCV_LOG_ERROR) {
      LOG_ERROR("%s", msg);
    }
    if (engine->log_level == NVCV_LOG_WARNING) {
      LOG_WARNING("%s", msg);
    }
    if (engine->log_level == NVCV_LOG_INFO) {
      LOG_INFO("%s", msg);
    }
    if (engine->log_level == NVCV_LOG_DEBUG) {
      LOG_DEBUG("%s", msg);
    }
    if (engine->log_level == NVCV_LOG_VERBOSE) {
      LOG_VERBOSE("%s", msg);
    }
  }
}

namespace nv3dvc {
namespace core {
namespace rendering {
NLOHMANN_JSON_SERIALIZE_ENUM(display::DisplayType,
                             {{display::DisplayType::VIRTUAL_DISPLAY, "VIRTUAL_DISPLAY"},
                              {display::DisplayType::SIMPLE_DISPLAY, "SIMPLE_DISPLAY"},
                              {display::DisplayType::DIMENCO_DISPLAY, "DIMENCO_DISPLAY"},
                              {display::DisplayType::LOOKING_GLASS_DISPLAY, "LOOKING_GLASS_DISPLAY"}});
}  // namespace rendering
}  // namespace core
}  // namespace nv3dvc

namespace nv3dvc {
namespace modules {
namespace rendermodule {

RenderModule::RenderModule(core::engine::Engine* engine) : m_engine(engine) {
  RegisterSystem<systems::RenderSystem>(engine);
  RegisterComponent<components::CubeMapComponent>();
  RegisterComponent<components::TexturedQuadComponent>();
  RegisterComponent<components::DisplayComponent>();
  RegisterComponent<components::RenderableTriplaneComponent>();
  RegisterComponent<components::StereoViewComponent>();
}

RenderModule::~RenderModule() {}

core::Error RenderModule::Initialize() {
  core::Error err = core::Error::SUCCESS;
  if (m_engine) {
    CHECK_NVCV_SUCCESS(
        NvAR_ConfigureLogger(m_engine->log_level, nullptr, RenderVolumeErrorCallback, static_cast<void*>(m_engine)));
  } else {
    LOG_WARNING("Engine not provided to RenderModule. Logging may be unconfigured for AR SDK.");
  }
bail:
  return err;
}

core::Error RenderModule::Uninitialize() { return core::Error::SUCCESS; }

core::Error RenderModule::OnLoadScene(core::ecs::registry::EntityRegistry* reg) {
  core::Error err = core::Error::SUCCESS;
  for (auto entity : reg->view<components::DisplayComponent>()) {
    auto& display_component = entity.GetComponent<components::DisplayComponent>();
    CHECK_SUCCESS(display_component.Initialize());
  }
  for (auto entity : reg->view<components::DisplayComponent, components::StereoViewComponent>()) {
    auto* display = entity.GetComponent<components::DisplayComponent>().GetDisplayPtr();
    auto& stereo_view = entity.GetComponent<components::StereoViewComponent>();

    std::vector<glm::vec3> view_points;

    if (display->GetStereoViewPoints(&view_points)) {
      float screen_width_mm = display->ScreenWidthMm();
      float screen_height_mm = display->ScreenHeightMm();
      float screen_width_pixels = display->RenderWidth();
      float screen_height_pixels = display->RenderHeight();
      stereo_view.SetViews(view_points, {screen_width_pixels, screen_height_pixels},
                           {screen_width_mm * 0.001f, screen_height_mm * 0.001f});
    }
  }
bail:
  return err;
}

core::Error RenderModule::Update(float dt) { return core::Error::SUCCESS; }

core::Error RenderModule::EncodeProperties(nlohmann::json* json_description,
                                           const PropertyOwner* property_owner) const {
  core::Error err = core::Error::SUCCESS;
  CHECK_SUCCESS(
      core::serialization::EncodeProperties<core::rendering::display::DisplayType>(json_description, property_owner));
bail:
  return err;
}

core::Error RenderModule::DecodeProperties(const nlohmann::json& json_description,
                                           PropertyOwner* property_owner) const {
  core::Error err = core::Error::SUCCESS;
  CHECK_SUCCESS(
      core::serialization::DecodeProperties<core::rendering::display::DisplayType>(json_description, property_owner));
bail:
  return err;
}

}  // namespace rendermodule
}  // namespace modules
}  // namespace nv3dvc
