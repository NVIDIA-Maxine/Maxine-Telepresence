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

#ifndef SRC_CORE_RENDERING_DISPLAY_VIRTUALDISPLAY_H_
#define SRC_CORE_RENDERING_DISPLAY_VIRTUALDISPLAY_H_

#include <string>
#include <vector>

#include "BaseDisplay.h"

namespace nv3dvc {
namespace core {
namespace rendering {
namespace display {

/// @brief Base class for display
///
/// This class represents virtual display that does not have a physical representation
class VirtualDisplay : public BaseDisplay {
 public:
  constexpr static const char* NAME = "VirtualDisplay";
  std::string Name() const { return NAME; }

  VirtualDisplay() = default;
  ~VirtualDisplay() override = default;

  core::Error Initialize(uint32_t render_width, uint32_t render_height, bool enforce_render_size,
                         uint32_t horizontal_resolution_divider, uint32_t vertical_resolution_divider) override;
};

}  // namespace display
}  // namespace rendering
}  // namespace core
}  // namespace nv3dvc

#endif  // SRC_CORE_RENDERING_DISPLAY_VIRTUALDISPLAY_H_
