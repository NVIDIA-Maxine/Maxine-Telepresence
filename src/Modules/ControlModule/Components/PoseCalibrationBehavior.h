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

#ifndef SRC_MODULES_CONTROLMODULE_COMPONENTS_POSECALIBRATIONBEHAVIOR_H_
#define SRC_MODULES_CONTROLMODULE_COMPONENTS_POSECALIBRATIONBEHAVIOR_H_

#include <string>

#include "Core/Application/Inputs.h"
#include "Core/Util/Types.h"
#include "Modules/BehaviorModule/Components/BehaviorComponent.h"

namespace nv3dvc {
namespace modules {
namespace controlmodule {
namespace components {

/// @defgroup PoseCalibrationBehaviorProperties PoseCalibrationBehavior
/// @ingroup  ComponentProperties
/// @brief    Behavior component to control head pose translational calibration
///
/// This behavior component assumes that the entity has an attached TrackedHeadComponent whose offset will be influenced
/// using the current translation during calibration. Calibration is triggered using a trigger property, or using key
/// input. Additionally, calibration can be implicitly triggered during startup if calibrateOnStartup is set to true. In
/// that case, after calibrationDelayMs milliseconds, a calibration is triggered automarically. The meaning of a
/// calibration is simply to subtract the current head translation, and add a desired translation (centerLocation) to
/// the tracked head translation. This will translate the tracked head to the desired location, "centering" it in view.
/// This can be used to position a tracked head to a desired location, such as the view focal plane center.

/// See @ref PoseCalibrationBehaviorProperties
class PoseCalibrationBehavior : public behaviormodule::components::BehaviorComponent {
 public:
  constexpr static const char* NAME = "PoseCalibrationBehavior";
  std::string Name() const override { return NAME; };

  PoseCalibrationBehavior();

  core::Error OnEvent(core::events::Event* e) override;
  core::Error OnUpdate(float dt) override;
  core::Error OnInitialize() override;

 public:
  /// @ingroup PoseCalibrationBehaviorProperties
  /// @{
  core::properties::Property<std::atomic<int>> countdown_ms = {
      this,
      "countdown_ms",
      "If < 0, calibration should not take place in the future",
      -1,
  };
  core::properties::Property<core::util::Trigger> trigger_calibrate = {
      this,
      "trigger_calibrate",
      "Triggers calibration to center the object",
      {kKeyCalibrate, core::application::inputs::NONE},
  };
  core::properties::Property<core::util::Trigger> trigger_uncalibrate = {
      this,
      "trigger_uncalibrate",
      "Triggers uncalibration to reset the object",
      {},
  };
  core::properties::Property<glm::vec3> last_calibration_location = {
      this,
      "last_calibration_location",
      "Last location, cached when calibrating",
      glm::vec3(0.0f, 0.0f, 0.0f),
  };
  core::properties::Property<glm::vec3> center_location = {
      this,
      "center_location",
      "Desired center location, new location when calibrating. Default roughly centers the eyes.",
      glm::vec3(0.0f, 2.0f, 7.0f),
  };
  core::properties::Property<std::atomic<uint32_t>> calibration_delay_ms = {
      this,
      "calibration_delay_ms",
      "Delay until calibration after it has been triggered",
      1000,
  };
  core::properties::Property<bool> calibrate_on_startup = {
      this,
      "calibrate_on_startup",
      "Whether to calibrate at startup",
      false,
  };
  /// @}

 private:
  bool m_isFirstIteration;

  // Controller
  static const core::application::inputs::Key kKeyCalibrate = core::application::inputs::Key::C;
};

}  // namespace components
}  // namespace controlmodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_CONTROLMODULE_COMPONENTS_POSECALIBRATIONBEHAVIOR_H_
