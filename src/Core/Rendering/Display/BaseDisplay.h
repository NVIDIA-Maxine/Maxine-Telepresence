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

#ifndef SRC_CORE_RENDERING_DISPLAY_BASEDISPLAY_H_
#define SRC_CORE_RENDERING_DISPLAY_BASEDISPLAY_H_

#include <string>
#include <vector>

#include "Core/Error.h"
#include "Display.h"
#include "glm/glm.hpp"

namespace nv3dvc {
namespace core {
namespace rendering {
namespace display {

/// @brief Base class for display
///
/// This class represents a common base display with typical API functions implemented. Initialization is not
/// implemented, but a helper Initialize function eases usage in derived classes
class BaseDisplay : public Display {
 public:
  constexpr static const char* NAME = "BaseDisplay";
  std::string Name() const { return NAME; }

  BaseDisplay() = default;
  ~BaseDisplay() override = default;

  uint32_t NumViews() const override;
  bool IsActiveStereo() const override;
  uint32_t QuiltCols() const override;
  uint32_t QuiltRows() const override;
  float ScreenWidthMm() const override;
  float ScreenHeightMm() const override;
  uint32_t ScreenResolutionX() const override;
  uint32_t ScreenResolutionY() const override;
  uint32_t ScreenPositionX() const override;
  uint32_t ScreenPositionY() const override;
  bool GetDesiredWindowPositionAndSize(int* pos_x, int* pos_y, int* width, int* height) const override;
  uint32_t RenderWidth() const override;
  uint32_t RenderHeight() const override;
  float RenderAspectRatio() const override;
  float DisplayAspectRatio() const override;
  float PhysicalDisplayAspectRatio() const override;
  bool GetStereoViewPoints(std::vector<glm::vec3>* view_points) override;
  bool BlitToDisplayBuffer(uint32_t fbo, uint32_t rendered_texture_id) override;

 protected:
  /// @brief Initialize the display with a known physical size
  ///
  /// This is a helper function that can be used in derived classes. It sets up all the protected variables in the
  /// BaseDisplay
  /// @param[in] render_width                  The with of the display buffer to be used by the renderer.
  ///                                          Only used if enforce_render_size = true.
  /// @param[in] render_height                 The height of the display buffer to be used by the renderer
  ///                                          Only used if enforce_render_size = true.
  /// @param[in] screen_width_mm               The physical width of the display [millimeters]
  /// @param[in] screen_height_mm              The physical height of the display [millimeters]
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
  Error BaseDisplay::Initialize(uint32_t render_width, uint32_t render_height, float screen_width_mm,
                                float screen_height_mm, bool enforce_render_size,
                                uint32_t horizontal_resolution_divider, uint32_t vertical_resolution_divider);

 protected:
  uint32_t m_numViews = 1;                     ///< > 1 If the display supports stereo rendering
  uint32_t m_horizontalResolutionDivider = 1;  ///< To reduce m_renderWidth
  uint32_t m_verticalResolutionDivider = 1;    ///< To reduce m_renderHeight
  uint32_t m_renderWidth = 0;   ///< Size in pixels to render. Can be the same as m_screenResolutionX but is independent
  uint32_t m_renderHeight = 0;  ///< Size in pixels to render. Can be the same as m_screenResolutionY but is independent

  float m_screenWidthMm = 0.f;   ///< physical screen width in mm
  float m_screenHeightMm = 0.f;  ///< physical screen height in mm

  int32_t m_screenResolutionX = 0;  ///< screen width in pixels
  int32_t m_screenResolutionY = 0;  ///< screen height in pixels

  int32_t m_screenPositionX = 0;  ///< X position of screen in global desktop coordinates, reported by OS
  int32_t m_screenPositionY = 0;  ///< Y position of screen in global desktop coordinates, reported by OS

  float m_renderAspectRatio = 1.f;           ///< The ratio is render width / render height
  float m_displayAspectRatio = 1.f;          ///< The ratio is display width [pixels] / display height [pixels]
  float m_physicalDisplayAspectRatio = 1.f;  ///< The ratio is display width [mm] / display height [mm]
};

}  // namespace display
}  // namespace rendering
}  // namespace core
}  // namespace nv3dvc

#endif  // SRC_CORE_RENDERING_DISPLAY_BASEDISPLAY_H_
