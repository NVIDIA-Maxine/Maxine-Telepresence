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

#include "VirtualDisplay.h"

#include <vector>

namespace nv3dvc {
namespace core {
namespace rendering {
namespace display {

Error VirtualDisplay::Initialize(uint32_t render_width, uint32_t render_height, bool enforce_render_size,
                                 uint32_t horizontal_resolution_divider, uint32_t vertical_resolution_divider) {
  Error err = Error::SUCCESS;
  static const float kMmPerInch = 25.4;                       // Millimeters per inch
  const float resolution_ppi = 110;                           // A typical resolution [pixels per inch]
  const float resolution_ppmm = resolution_ppi / kMmPerInch;  // [pixels per millimeter]

  const float screen_width_mm = static_cast<float>(render_width) / resolution_ppmm;
  const float screen_height_mm = static_cast<float>(render_height) / resolution_ppmm;

  LOG_INFO("Setting up virtual display");
  CHECK_TRUE(enforce_render_size, Error::ERR_DISPLAY, "Virtual Display must enforce render size");
  CHECK_SUCCESS(BaseDisplay::Initialize(render_width, render_height, screen_width_mm, screen_height_mm,
                                        enforce_render_size, horizontal_resolution_divider,
                                        vertical_resolution_divider));

  LOG_INFO("Virtual display resolution [PPI]  : %i", resolution_ppi);
  LOG_INFO("Virtual display size [millimeters]: %.2f x %.2f", m_screenWidthMm, m_screenHeightMm);
bail:
  return err;
}

}  // namespace display
}  // namespace rendering
}  // namespace core
}  // namespace nv3dvc
