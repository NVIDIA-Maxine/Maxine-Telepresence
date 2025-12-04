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

#include "DisplayComponent.h"

#include <utility>

#include "Core/Rendering/Display/DimencoDisplay.h"
#include "Core/Rendering/Display/LookingGlassDisplay.h"
#include "Core/Rendering/Display/SimpleDisplay.h"
#include "Core/Rendering/Display/VirtualDisplay.h"
#include "Core/Util/Logger.h"

namespace nv3dvc {
namespace modules {
namespace rendermodule {
namespace components {

core::Error DisplayComponent::Initialize() {
  core::Error err = core::Error::SUCCESS;
  std::unique_ptr<core::rendering::display::Display> display;
  switch (display_type) {
    case core::rendering::display::DisplayType::VIRTUAL_DISPLAY: {
      display = std::make_unique<core::rendering::display::VirtualDisplay>();
      err = display->Initialize(render_width, render_height, enforce_render_size, horizontal_resolution_divider,
                                vertical_resolution_divider);
      CHECK_SUCCESS(err, "Failed to create Virtual Display");
      break;
    }
    case core::rendering::display::DisplayType::SIMPLE_DISPLAY: {
      display = std::make_unique<core::rendering::display::SimpleDisplay>();
      err = display->Initialize(render_width, render_height, enforce_render_size, horizontal_resolution_divider,
                                vertical_resolution_divider);
      CHECK_SUCCESS(err, "Failed to create SimpleDisplay");
      break;
    }
    case core::rendering::display::DisplayType::DIMENCO_DISPLAY: {
      auto dimenco_display = std::make_unique<core::rendering::display::DimencoDisplay>();
      err = dimenco_display->Initialize(render_width, render_height, enforce_render_size, horizontal_resolution_divider,
                                        vertical_resolution_divider);
      CHECK_SUCCESS(err, "Failed to create DimencoDisplay");
      display = std::move(dimenco_display);
      break;
    }
    case core::rendering::display::DisplayType::LOOKING_GLASS_DISPLAY: {
      auto looking_glass_display = std::make_unique<core::rendering::display::LookingGlassDisplay>();
      err = looking_glass_display->Initialize(render_width, render_height, enforce_render_size,
                                              horizontal_resolution_divider, vertical_resolution_divider);
      CHECK_SUCCESS(err, "Failed to create LookingGlassDisplay");
      err = looking_glass_display->SetupQuiltLayoutFromPreset(looking_glass_display->RenderWidth(),
                                                              looking_glass_display->RenderHeight(), landscape_preset);
      CHECK_SUCCESS(err, "Failed to setup quilt layout for LookingGlassDisplay");
      err = looking_glass_display->LoadLightFieldShader();
      CHECK_SUCCESS(err, "Failed to load Light Field Shader for LookingGlassDisplay");
      display = std::move(looking_glass_display);
      break;
    }
    default: {
      // If we don't have a display we can't use any other render size
      enforce_render_size = true;
      horizontal_resolution_divider = 1;
      vertical_resolution_divider = 1;
      display = nullptr;
    }
  }
  m_display = std::move(display);
bail:
  return err;
}

core::rendering::display::Display* DisplayComponent::GetDisplayPtr() { return m_display.get(); }

core::rendering::display::DisplayType DisplayComponent::GetDisplayType() { return display_type; }

uint32_t DisplayComponent::GetDesiredNumViews() {
  if (m_display) {
    return m_display->NumViews();
  } else {
    return 1;
  }
}

uint32_t DisplayComponent::GetDesiredQuiltCols() {
  if (m_display) {
    return m_display->QuiltCols();
  } else {
    return 1;
  }
}

uint32_t DisplayComponent::GetDesiredQuiltRows() {
  if (m_display) {
    return m_display->QuiltRows();
  } else {
    return 1;
  }
}

uint32_t DisplayComponent::GetDesiredRenderWidth() {
  if (m_display) {
    return enforce_render_size ? render_width / horizontal_resolution_divider : m_display->RenderWidth();
  } else {
    return render_width;
  }
}

uint32_t DisplayComponent::GetDesiredRenderHeight() {
  if (m_display) {
    return enforce_render_size ? render_height / vertical_resolution_divider : m_display->RenderHeight();
  } else {
    return render_height;
  }
}

}  // namespace components
}  // namespace rendermodule
}  // namespace modules
}  // namespace nv3dvc
