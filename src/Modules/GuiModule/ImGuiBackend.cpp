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

#include "ImGuiBackend.h"

#include <imgui_impl_opengl3.h>

#include "Core/Application/InputMapping.h"

namespace nv3dvc {
namespace modules {
namespace guimodule {
namespace imgui_backend {

void ImGuiInitialize() {
  ImGuiIO& io = ImGui::GetIO();

  // Keyboard mapping from https://github.com/ocornut/imgui/blob/v1.86/backends/imgui_impl_glfw.cpp#L202
  io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;
  io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
  io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
  io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
  io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
  io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
  io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
  io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
  io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
  io.KeyMap[ImGuiKey_Insert] = GLFW_KEY_INSERT;
  io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
  io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
  io.KeyMap[ImGuiKey_Space] = GLFW_KEY_SPACE;
  io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
  io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
  io.KeyMap[ImGuiKey_KeyPadEnter] = GLFW_KEY_KP_ENTER;
  io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
  io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
  io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
  io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
  io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
  io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;

  ImGui_ImplOpenGL3_Init("#version 410");
}

void ImGuiUninitialize() { ImGui_ImplOpenGL3_Shutdown(); }

void ImGuiNewFrame() { ImGui_ImplOpenGL3_NewFrame(); }

void ImGuiRenderDrawData(ImDrawData* draw_data) { ImGui_ImplOpenGL3_RenderDrawData(draw_data); }

int ImGuiMapMouseButton(const core::application::inputs::Mouse mouse) {
  return core::application::inputs::MOUSE_NV3DVC_TO_GLFW.at(mouse);
}

int ImGuiMapKey(const core::application::inputs::Key key) {
  return core::application::inputs::KEY_NV3DVC_TO_GLFW.at(key);
}

}  // namespace imgui_backend
}  // namespace guimodule
}  // namespace modules
}  // namespace nv3dvc
