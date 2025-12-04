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

#ifndef SRC_MODULES_GUIMODULE_PANELS_APPLICATIONCONFIGPANEL_H_
#define SRC_MODULES_GUIMODULE_PANELS_APPLICATIONCONFIGPANEL_H_

#include "Core/Engine/Engine.h"

namespace nv3dvc {
namespace modules {
namespace guimodule {
namespace panels {

/// @brief Panel for displaying the application configuration
///
/// The application configuration consists of all the properties and subowners registered for the Engine
/// The subowners of the Engine is typically the set of all Modules and their corresponding Systems which are all
/// property owners
class ApplicationConfigPanel {
 public:
  /// @brief Render all the properties and subowners of the engine of the engine
  /// @param[in,out] engine The engine. It's properties are allowed to change based on user input
  /// @param[in]         dt Time differential, the time taken for the last iteration on this thread
  void Render(core::engine::Engine* engine, float dt);

 private:
  int m_frameCounter = 0;      // Number of frames within the last second
  int m_nFramesPerSecond = 0;  // Number of frames rendered within the last second
  float m_timer = 0.0f;        // Counts up to one second, then resets
};

}  // namespace panels
}  // namespace guimodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_GUIMODULE_PANELS_APPLICATIONCONFIGPANEL_H_
