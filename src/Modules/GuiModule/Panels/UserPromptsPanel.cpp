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

#include "UserPromptsPanel.h"

#include <string>

#include "imgui.h"

namespace nv3dvc {
namespace modules {
namespace guimodule {
namespace panels {

core::Error UserPromptsPanel::Render(core::engine::Engine* engine) {
  core::Error err = core::Error::SUCCESS;

  ImGui::SetNextWindowPos(m_panelPos);
  ImGui::SetNextWindowSize(m_panelSize);
  ImGuiWindowFlags flags_for_text_only = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground |
                                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration |
                                         ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoScrollWithMouse |
                                         ImGuiWindowFlags_NoNav;
  if (ImGui::Begin("User prompts", nullptr, flags_for_text_only)) {
    for (const std::string& prompt : engine->GetUserPrompts()) {
      ImGui::SetWindowFontScale(m_fontScale);
      ImGui::Text("%s", prompt.c_str());
    }
    ImGui::End();
  }

bail:
  return err;
}

}  // namespace panels
}  // namespace guimodule
}  // namespace modules
}  // namespace nv3dvc
