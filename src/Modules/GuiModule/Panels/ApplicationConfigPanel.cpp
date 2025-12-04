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

#include "ApplicationConfigPanel.h"

#include <imgui.h>

#include "Core/Engine/Engine.h"
#include "RenderProperties.h"

namespace nv3dvc {
namespace modules {
namespace guimodule {
namespace panels {

void ApplicationConfigPanel::Render(core::engine::Engine* engine, const float dt) {
  m_frameCounter++;
  m_timer += dt;
  if (m_timer >= 1.0f) {
    // One second. Save and reset;
    m_nFramesPerSecond = m_frameCounter;
    m_frameCounter = 0;
    m_timer = 0;
  }
  ImGui::Begin("Application Config");
  ImGui::Text("Frame rate: %d FPS", m_nFramesPerSecond);
  RenderProperties(engine->Name().c_str(), engine);
  ImGui::End();
}

}  // namespace panels
}  // namespace guimodule
}  // namespace modules
}  // namespace nv3dvc
