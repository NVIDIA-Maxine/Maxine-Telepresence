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

#ifndef SRC_MODULES_GUIMODULE_IMGUIBACKEND_H_
#define SRC_MODULES_GUIMODULE_IMGUIBACKEND_H_

#include "Core/Application/Inputs.h"

class ImDrawData;

namespace nv3dvc {
namespace modules {
namespace guimodule {

/// @brief API for ImGui backend. Implementation can use different rendering or windowing backends
///
/// The current implementation uses ImGui for GLFW and OpenGL 3
namespace imgui_backend {

/// @brief Initialize ImGui backend
void ImGuiInitialize();

/// @brief Uninitialize ImGui
void ImGuiUninitialize();

/// @brief Create new ImGui frame
void ImGuiNewFrame();

/// @brief Render ImGui draw data
/// @param[in,out] draw_data ImGui draw data
void ImGuiRenderDrawData(ImDrawData* draw_data);

/// @brief Map mouse button for ImGui
/// @param[in] mouse The mouse button to map
/// @return    The mouse button mapped for ImGui
int ImGuiMapMouseButton(core::application::inputs::Mouse mouse);

/// @brief Map a key for ImGui
/// @param[in] key The key to map
/// @return    The key mapped for ImGui
int ImGuiMapKey(core::application::inputs::Key key);

}  // namespace imgui_backend
}  // namespace guimodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_GUIMODULE_IMGUIBACKEND_H_
