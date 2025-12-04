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

#include "SimpleDisplay.h"

#include "GLFW/glfw3.h"
#include "nvCVStatus.h"
#include "nvCVTriplaneVolume.h"
#include "nvCVVolumeDefs.h"

namespace nv3dvc {
namespace core {
namespace rendering {
namespace display {

int32_t SimpleDisplay::FindMonitorBasedOnDesktopLocation(int32_t pos_x, int32_t pos_y) {
  int monitor_count = 0;
  GLFWmonitor** monitors = glfwGetMonitors(&monitor_count);
  int found_monitor = -1;
  for (int i = 0; i < monitor_count; i++) {
    int glfw_pos_x, glfw_pos_y;
    glfwGetMonitorPos(monitors[i], &glfw_pos_x, &glfw_pos_y);
    if (glfw_pos_x == pos_x && glfw_pos_y == pos_y) {
      // Found the monitor at this desktop location
      found_monitor = i;
    }
  }
  return found_monitor;
}

core::Error SimpleDisplay::GetPhysicalSizeMm(int32_t monitor_idx, int32_t* size_x, int32_t* size_y) {
  core::Error err = core::Error::SUCCESS;
  int monitor_count = 0;
  GLFWmonitor** monitors;
  BAIL_IF_TRUE(monitor_idx < 0, err, core::Error::ERR_DISPLAY);
  monitors = glfwGetMonitors(&monitor_count);
  BAIL_IF_TRUE(monitor_idx >= monitor_count, err, core::Error::ERR_DISPLAY);
  glfwGetMonitorPhysicalSize(monitors[monitor_idx], size_x, size_y);
bail:
  return err;
}

core::Error SimpleDisplay::Initialize(uint32_t render_width, uint32_t render_height, bool enforce_render_size,
                                      uint32_t horizontal_resolution_divider, uint32_t vertical_resolution_divider) {
  core::Error err = core::Error::SUCCESS;
  // Create window on main screen
  GLFWmonitor* monitor = glfwGetPrimaryMonitor();
  CHECK_NONNULL(monitor, core::Error::ERR_DISPLAY, "Could not find primary monitor");
  const GLFWvidmode* mode = glfwGetVideoMode(monitor);
  CHECK_NONNULL(mode, core::Error::ERR_DISPLAY, "Could not get video mode for primary monitor");
  // get physical screen width and height in millimeters
  int screen_width_mm_i, screen_height_mm_i;
  glfwGetMonitorPhysicalSize(monitor, &screen_width_mm_i, &screen_height_mm_i);
  const float screen_width_mm = static_cast<float>(screen_width_mm_i);
  const float screen_height_mm = static_cast<float>(screen_height_mm_i);
  LOG_DEBUG("Physical screen size reported to be %d mm x %d mm", screen_width_mm_i, screen_height_mm_i);
  CHECK_SUCCESS(BaseDisplay::Initialize(mode->width, mode->height, screen_width_mm, screen_height_mm,
                                        enforce_render_size, horizontal_resolution_divider,
                                        vertical_resolution_divider));
  // get screen position in global coordinates
  glfwGetMonitorPos(monitor, &m_screenPositionX, &m_screenPositionY);
bail:
  return err;
}

}  // namespace display
}  // namespace rendering
}  // namespace core
}  // namespace nv3dvc
