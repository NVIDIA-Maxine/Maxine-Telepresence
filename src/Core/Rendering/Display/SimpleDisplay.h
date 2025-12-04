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

#ifndef SRC_CORE_RENDERING_DISPLAY_SIMPLEDISPLAY_H_
#define SRC_CORE_RENDERING_DISPLAY_SIMPLEDISPLAY_H_

#include <mutex>  // NOLINT [build/c++11] (cpplint outside Chromium: https://github.com/google/styleguide/issues/483)
#include <string>
#include <vector>

#include "Core/Error.h"
#include "Core/Properties/Property.h"
#include "Modules/CommonModule/Components/CameraComponent.h"
#include "VirtualDisplay.h"
#include "glm/glm.hpp"
#include "nvAR.h"
#include "nvCVTriplaneVolume.h"

namespace nv3dvc {
namespace core {
namespace rendering {
namespace display {

/// @brief Base class for display
///
/// This class represents a common 2D physical display type. Extensions may represent stereo display types
class SimpleDisplay : public BaseDisplay {
 public:
  constexpr static const char* NAME = "SimpleDisplay";
  std::string Name() const { return NAME; }

  /// @brief Find a monitor by index given a pixel location on the desktop
  ///
  /// The location needs to match exactly for for the monitor to be detected
  /// @param[in] pos_x The x position of the monitor in OS desktop coordinates
  /// @param[in] pos_y The y position of the monitor in OS desktop coordinates
  /// @return          The index of the monitor
  ///                  -1 if no monitor was found at this location
  static int32_t FindMonitorBasedOnDesktopLocation(int32_t pos_x, int32_t pos_y);

  /// @brief Get the physical size of a monitor, given a monitor index
  /// @param[in] monitor_idx The index of the monitor to query
  /// @param[in] size_x      The x size in millimeters
  /// @param[in] size_y      The y size in millimeters
  /// @return                core::Error::SUCCESS if successful
  static core::Error GetPhysicalSizeMm(int32_t monitor_idx, int32_t* size_x, int32_t* size_y);

  SimpleDisplay() = default;
  ~SimpleDisplay() override = default;

  core::Error Initialize(uint32_t render_width, uint32_t render_height, bool enforce_render_size,
                         uint32_t horizontal_resolution_divider, uint32_t vertical_resolution_divider) override;
};

}  // namespace display
}  // namespace rendering
}  // namespace core
}  // namespace nv3dvc

#endif  // SRC_CORE_RENDERING_DISPLAY_SIMPLEDISPLAY_H_
