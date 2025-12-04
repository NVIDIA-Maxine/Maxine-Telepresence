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

#include "BaseDisplay.h"

#include <vector>

namespace nv3dvc {
namespace core {
namespace rendering {
namespace display {

uint32_t BaseDisplay::NumViews() const { return m_numViews; }
bool BaseDisplay::IsActiveStereo() const { return false; }
uint32_t BaseDisplay::QuiltCols() const { return 1; }
uint32_t BaseDisplay::QuiltRows() const { return 1; }
float BaseDisplay::ScreenWidthMm() const { return m_screenWidthMm; }
float BaseDisplay::ScreenHeightMm() const { return m_screenHeightMm; }
uint32_t BaseDisplay::ScreenResolutionX() const { return m_screenResolutionX; }
uint32_t BaseDisplay::ScreenResolutionY() const { return m_screenResolutionY; }
uint32_t BaseDisplay::ScreenPositionX() const { return m_screenPositionX; }
uint32_t BaseDisplay::ScreenPositionY() const { return m_screenPositionY; }
bool BaseDisplay::GetDesiredWindowPositionAndSize(int* pos_x, int* pos_y, int* width, int* height) const {
  return false;
}
uint32_t BaseDisplay::RenderWidth() const { return m_renderWidth; }
uint32_t BaseDisplay::RenderHeight() const { return m_renderHeight; }
float BaseDisplay::RenderAspectRatio() const { return m_renderAspectRatio; }
float BaseDisplay::DisplayAspectRatio() const { return m_displayAspectRatio; }
float BaseDisplay::PhysicalDisplayAspectRatio() const { return m_physicalDisplayAspectRatio; }
bool BaseDisplay::GetStereoViewPoints(std::vector<glm::vec3>* view_points) { return false; }
bool BaseDisplay::BlitToDisplayBuffer(uint32_t fbo, uint32_t rendered_texture_id) { return false; }

Error BaseDisplay::Initialize(uint32_t render_width, uint32_t render_height, float screen_width_mm,
                              float screen_height_mm, bool enforce_render_size, uint32_t horizontal_resolution_divider,
                              uint32_t vertical_resolution_divider) {
  core::Error err = core::Error::SUCCESS;
  CHECK_TRUE(render_width > 0, core::Error::ERR_DISPLAY, "Render width needs to be > 0");
  CHECK_TRUE(render_height > 0, core::Error::ERR_DISPLAY, "Render height needs to be > 0");
  CHECK_TRUE(screen_width_mm > 0, core::Error::ERR_DISPLAY, "Screen width needs to be > 0");
  CHECK_TRUE(screen_height_mm > 0, core::Error::ERR_DISPLAY, "Screen height needs to be > 0");
  CHECK_TRUE(horizontal_resolution_divider > 0, core::Error::ERR_DISPLAY,
             "Horizontal resolution divider needs to be > 0");
  CHECK_TRUE(vertical_resolution_divider > 0, core::Error::ERR_DISPLAY, "Vertical resolution divider needs to be > 0");

  m_horizontalResolutionDivider = horizontal_resolution_divider;
  m_verticalResolutionDivider = vertical_resolution_divider;

  m_screenResolutionX = render_width;
  m_screenResolutionY = render_height;

  if (enforce_render_size) {
    m_renderWidth = render_width;
    m_renderHeight = render_height;
  } else {
    m_renderWidth = m_screenResolutionX / m_horizontalResolutionDivider;
    m_renderHeight = m_screenResolutionY / m_verticalResolutionDivider;
  }

  m_screenPositionX = 0;
  m_screenPositionY = 0;

  m_screenWidthMm = screen_width_mm;
  m_screenHeightMm = screen_height_mm;

  m_displayAspectRatio = static_cast<float>(m_screenResolutionX) / static_cast<float>(m_screenResolutionY);
  m_renderAspectRatio = static_cast<float>(m_renderWidth) / static_cast<float>(m_renderHeight);
  m_physicalDisplayAspectRatio = m_screenWidthMm / m_screenHeightMm;
bail:
  return err;
}

}  // namespace display
}  // namespace rendering
}  // namespace core
}  // namespace nv3dvc
