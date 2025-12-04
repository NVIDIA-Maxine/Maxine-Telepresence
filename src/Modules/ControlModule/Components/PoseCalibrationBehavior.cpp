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

#include "PoseCalibrationBehavior.h"

#include <algorithm>

#include "Core/Application/Inputs.h"
#include "Core/Events/KeyEvent.h"
#include "Core/Events/MouseEvent.h"
#include "Core/Events/WindowEvent.h"
#include "Modules/TrackingModule/Components/TrackedHeadComponent.h"
#include "glm/glm.hpp"

namespace nv3dvc {
namespace modules {
namespace controlmodule {
namespace components {

PoseCalibrationBehavior::PoseCalibrationBehavior() : m_isFirstIteration(false) {
  trigger_calibrate.SetOnChangeFunction([this]() { countdown_ms.get()->store(calibration_delay_ms.get()->load()); });
  trigger_uncalibrate.SetOnChangeFunction([this]() {
    if (HasComponent<trackingmodule::components::TrackedHeadComponent>()) {
      auto& tracked_head = GetComponent<trackingmodule::components::TrackedHeadComponent>();
      last_calibration_location = glm::vec3(0.0f, 0.0f, 0.0f);
      tracked_head.SetTranslationOffset(glm::vec3(0.0f, 0.0f, 0.0f));
    }
  });
}

core::Error PoseCalibrationBehavior::OnInitialize() {
  if (calibrate_on_startup) {
    countdown_ms.get()->store(calibration_delay_ms.get()->load());  // Auto-calibration should take place
  }
  m_isFirstIteration = true;
  return core::SUCCESS;
}

core::Error PoseCalibrationBehavior::OnEvent(core::events::Event* e) { return HandleTriggers(e); }

core::Error PoseCalibrationBehavior::OnUpdate(const float dt) {
  if (countdown_ms.get()->load() >= 0 && !m_isFirstIteration) {
    countdown_ms.get()->store(countdown_ms.get()->load() - dt * 1000.0f);
    countdown_ms.get()->store(std::max(static_cast<int>(countdown_ms.get()->load()), 0));
  }
  if (countdown_ms.get()->load() == 0 && HasComponent<trackingmodule::components::TrackedHeadComponent>()) {
    // Perform calibration
    auto& tracked_head = GetComponent<trackingmodule::components::TrackedHeadComponent>();
    last_calibration_location = tracked_head.face_box_to_display_translation;
    tracked_head.SetTranslationOffset(center_location - last_calibration_location);

    countdown_ms.get()->store(-1);  // Auto-calibration should not take place
  }
  m_isFirstIteration = false;
  return core::SUCCESS;
}

}  // namespace components
}  // namespace controlmodule
}  // namespace modules
}  // namespace nv3dvc
