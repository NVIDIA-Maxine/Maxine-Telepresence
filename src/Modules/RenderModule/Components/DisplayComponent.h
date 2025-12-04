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

#ifndef SRC_MODULES_RENDERMODULE_COMPONENTS_DISPLAYCOMPONENT_H_
#define SRC_MODULES_RENDERMODULE_COMPONENTS_DISPLAYCOMPONENT_H_

#include <memory>
#include <string>

#include "Core/EntityComponentSystem/Component.h"
#include "Core/Rendering/Display/Display.h"
#include "Core/Rendering/Display/DisplayType.h"

namespace nv3dvc {
namespace modules {
namespace rendermodule {
namespace components {

/// @defgroup DisplayComponentProperties DisplayComponent
/// @ingroup  ComponentProperties
/// @brief    Component representing a physical display

/// @see DisplayComponentProperties
class DisplayComponent : public core::ecs::Component {
 public:
  constexpr static const char* NAME = "DisplayComponent";
  std::string Name() const override { return NAME; };

  DisplayComponent() = default;

  /// @brief Initialize the display
  ///
  /// Creates a display object depending on the value of display_type
  /// @return core::Error::SUCCESS If successful
  core::Error Initialize();

  /// @brief Get a pointer to the display object
  /// @return A pointer to the display object
  core::rendering::display::Display* GetDisplayPtr();

  /// @brief Get the display type
  /// @return The display type
  core::rendering::display::DisplayType GetDisplayType();

  /// @brief Get the desired number of views for this display
  /// @return The desired number of views for this display
  uint32_t GetDesiredNumViews();

  /// @brief Get the desired quilt tiling's number of columns for this display
  /// @return The desired quilt tiling's number of columns for this display
  uint32_t GetDesiredQuiltCols();

  /// @brief Get the desired quilt tiling's number of rows for this display
  /// @return The desired quilt tiling's number of rows for this display
  uint32_t GetDesiredQuiltRows();

  /// @brief Get the desired render width for this display
  /// @return The desired render width for this display [pixels]
  uint32_t GetDesiredRenderWidth();

  /// @brief Get the desired render height for this display
  /// @return The desired render height for this display [pixels]
  uint32_t GetDesiredRenderHeight();

 public:
  /// @ingroup DisplayComponentProperties
  /// @{
  core::properties::Property<core::rendering::display::DisplayType> display_type = {
      this,
      "display_type",
      "The display type. See documentation for possible values.",
      core::rendering::display::DisplayType::SIMPLE_DISPLAY,
  };
  core::properties::Property<uint32_t> render_width = {
      this,
      "render_width",
      "The width of a single view tile of the display buffer. Only effective if enforce_render_size==true, if not the "
      "render size is based on the display's resolution",
      512,
  };
  core::properties::Property<uint32_t> render_height = {
      this,
      "render_height",
      "The height of a single view tile of the display buffer. Only effective if enforce_render_size==true, if not the "
      "render size is based on the display's resolution",
      512,
  };
  core::properties::Property<bool> enforce_render_size = {
      this,
      "enforce_render_size",
      "Whether to use render_width, and render_height rather than the resolution of the display for the size of the "
      "display buffer.",
      false,
  };
  core::properties::Property<uint32_t> horizontal_resolution_divider = {
      this,
      "horizontal_resolution_divider",
      "Reduces the horizontal resolution by a factor. Enables quality / performance tradeoff",
      1,
  };
  core::properties::Property<uint32_t> vertical_resolution_divider = {
      this,
      "vertical_resolution_divider",
      "Reduces the vertical resolution by a factor. Enables quality / performance tradeoff",
      1,
  };
  core::properties::Property<uint32_t> landscape_preset = {
      this,
      "landscape_preset",
      "Landscape preset, only used for the LOOKING_GLASS_DISPLAY type",
      0,
  };
  core::properties::Property<bool> disable_rendering = {
      this,
      "disable_rendering",
      "Disable rendering on this display",
      false,
  };
  /// @}

 private:
  std::unique_ptr<core::rendering::display::Display> m_display = nullptr;
};

}  // namespace components
}  // namespace rendermodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_RENDERMODULE_COMPONENTS_DISPLAYCOMPONENT_H_
