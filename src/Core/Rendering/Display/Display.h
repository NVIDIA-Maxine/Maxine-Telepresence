/*
 * SPDX-FileCopyrightText: Copyright (c) 2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: LicenseRef-NvidiaProprietary
 *
 * NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
 * property and proprietary rights in and to this material, related
 * documentation and any modifications thereto. Any use, reproduction,
 * disclosure or distribution of this material and related documentation
 * without an express license agreement from NVIDIA CORPORATION or
 * its affiliates is strictly prohibited.
 */

#ifndef SRC_CORE_RENDERING_DISPLAY_DISPLAY_H_
#define SRC_CORE_RENDERING_DISPLAY_DISPLAY_H_

#include <string>
#include <vector>

#include "Core/Error.h"
#include "glm/glm.hpp"

namespace nv3dvc {
namespace core {
namespace rendering {
namespace display {

/// @brief Abstract base class for display
class Display {
 public:
  constexpr static const char* NAME = "Display";
  std::string Name() const { return NAME; }

  Display() = default;
  virtual ~Display() {}

  /// @brief Initialize the display
  /// @param[in] render_width                  The with of the display buffer to be used by the renderer.
  ///                                          Only used if enforce_render_size = true.
  /// @param[in] render_height                 The height of the display buffer to be used by the renderer
  ///                                          Only used if enforce_render_size = true.
  /// @param[in] enforce_render_size           If true, enforce the use of explicit render with and render height
  ///                                          If false, the resolution of the display itself will be used to determine
  ///                                          the render width, and render height.
  /// @param[in] horizontal_resolution_divider If enforce_render_size = true, this is ignored.
  ///                                          If enforce_render_size = false, the display's width in pixels divided by
  ///                                          the horizontal resolution divider will result in the render width
  /// @param[in] vertical_resolution_divider   If enforce_render_size = true, this is ignored.
  ///                                          If enforce_render_size = false, the display's height in pixels divided by
  ///                                          the vertical resolution divider will result in the render width
  /// @return    core::Error::SUCCESS          If successful
  virtual core::Error Initialize(uint32_t render_width, uint32_t render_height, bool enforce_render_size,
                                 uint32_t horizontal_resolution_divider, uint32_t vertical_resolution_divider) = 0;

  /// @brief Get the number of views configured for this display
  /// @return The number of views. Should be 1 for regular 2D displays, > 1 for stereo display types
  virtual uint32_t NumViews() const = 0;

  /// @brief Bool whether stereo views need regular update
  /// @return Whether this is an active stereo display
  virtual bool IsActiveStereo() const = 0;

  /// @brief Get the number of columns in the quilt. Should be 1 for regular 2D displays
  /// @return The number of quilt columns
  virtual uint32_t QuiltCols() const = 0;

  /// @brief Get the number of rows in the quilt. Should be 1 for regular 2D displays
  /// @return The number of quilt rows
  virtual uint32_t QuiltRows() const = 0;

  /// @brief Get the physical screen width in millimeter units
  /// @return The screen width
  virtual float ScreenWidthMm() const = 0;

  /// @brief Get the physical screen height in millimeter units
  /// @return The screen height
  virtual float ScreenHeightMm() const = 0;

  /// @brief Get the screen resolution in the horizontal dimension
  /// @return The horizontal screen resolution
  virtual uint32_t ScreenResolutionX() const = 0;

  /// @brief Get the screen resolution in the vertical dimension
  /// @return The vertical screen resolution
  virtual uint32_t ScreenResolutionY() const = 0;

  /// @brief Get the horizontal coordinate for the position of this screen in desktop pixel coodinates
  ///
  /// If multiple monitors are connected, they will each have their own unique location on the desktop. With only one
  /// monitor connected, this is expected to be 0.
  /// @return The screen's horizontal desktop coordinate
  virtual uint32_t ScreenPositionX() const = 0;

  /// @brief Get the vertical coordinate for the position of this screen in desktop pixel coodinates
  ///
  /// If multiple monitors are connected, they will each have their own unique location on the desktop. With only one
  /// monitor connected, this is expected to be 0.
  /// @return The screen's vertical desktop coordinate
  virtual uint32_t ScreenPositionY() const = 0;

  /// @brief Get the position and size in desktop pixel coordinates for the window to fit this display
  /// @param[out] pos_x  The desired horizontal coordinate [pixels]
  /// @param[out] pos_y  The desired vertical coordinate [pixels]
  /// @param[out] width  The desired horizontal size [pixels]
  /// @param[out] height The desired horizontal size [pixels]
  /// @return     true   If all input parameters were set
  ///             false  Otherwise
  virtual bool GetDesiredWindowPositionAndSize(int* pos_x, int* pos_y, int* width, int* height) const = 0;

  /// @brief Get the render width for the display buffer to use
  ///
  /// Note that this does not necessarily correspond to the display resolution for two reasons
  /// - Quilted rendering is not taken into account, the render width corresponds to one tile in the quilt
  /// - The render buffer resolution may otherwise differ from the resolution of the display
  ///   - It can be set explicitly, if enforce_render_size was provided to the Display constructor
  ///   - It can be of lower resolution, if horizontal_resolution_divider passed to the Display constructor was > 1
  /// @return The render width
  virtual uint32_t RenderWidth() const = 0;

  /// @brief Get the render height for the display buffer to use
  ///
  /// Note that this does not necessarily correspond to the display resolution for two reasons
  /// - Quilted rendering is not taken into account, the render height corresponds to one tile in the quilt
  /// - The render buffer resolution may otherwise differ from the resolution of the display
  ///   - It can be set explicitly, if enforce_render_size was provided to the Display constructor
  ///   - It can be of lower resolution, if vertical_resolution_divider passed to the Display constructor was > 1
  /// @return The render height
  virtual uint32_t RenderHeight() const = 0;

  /// @brief Get the render aspect ratio. The ratio is render width / render height
  ///
  /// Note that it may differ from the display's aspect ratio
  /// @return The render aspect ratio
  virtual float RenderAspectRatio() const = 0;

  /// @brief Get the display's aspect ratio. The ratio is display width [pixels] / display height [pixels]
  ///
  /// Note that it may differ from the render aspect ratio
  /// @return The display's pixel aspect ratio
  virtual float DisplayAspectRatio() const = 0;

  /// @brief Get the display's physical aspect ratio. The ratio is display width [mm] / display height [mm]
  ///
  /// Note that it may differ from the render aspect ratio
  /// @return The display's physical aspect ratio
  virtual float PhysicalDisplayAspectRatio() const = 0;

  /// @brief If this is a stereo view display, get the desired view points from which the views should be rendered
  /// @param[out] view_points The display's desired view points
  /// @return     true        If this is a stereo display with desired viewpoints, and they were provided
  virtual bool GetStereoViewPoints(std::vector<glm::vec3>* view_points) = 0;

  /// @brief Blit a rendered frame buffer object to the display
  ///
  /// @param[in] fbo                 The GL frame buffer object to which the render texture is attached
  /// @param[in] rendered_texture_id The render texture containing the quilted layout of the views
  /// @return    true                If successful
  virtual bool BlitToDisplayBuffer(uint32_t fbo, uint32_t rendered_texture_id) = 0;
};

}  // namespace display
}  // namespace rendering
}  // namespace core
}  // namespace nv3dvc

#endif  // SRC_CORE_RENDERING_DISPLAY_DISPLAY_H_
