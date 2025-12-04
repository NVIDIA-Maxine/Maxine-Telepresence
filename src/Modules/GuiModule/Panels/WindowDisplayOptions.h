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

#ifndef SRC_MODULES_GUIMODULE_PANELS_WINDOWDISPLAYOPTIONS_H_
#define SRC_MODULES_GUIMODULE_PANELS_WINDOWDISPLAYOPTIONS_H_

namespace nv3dvc {
namespace modules {
namespace guimodule {
namespace panels {

/// @brief Display options for controlling which GUI elements should be shown
struct WindowDisplayOptions {
  bool show_app_config_panel = true;
  bool show_scene_hierarchy_panel = true;
  bool show_entity_properties_panel = true;
  bool show_video_panel = true;
  bool show_user_prompts = true;
};

}  // namespace panels
}  // namespace guimodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_GUIMODULE_PANELS_WINDOWDISPLAYOPTIONS_H_
