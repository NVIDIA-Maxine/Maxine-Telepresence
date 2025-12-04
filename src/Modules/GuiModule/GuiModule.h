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

#ifndef SRC_MODULES_GUIMODULE_GUIMODULE_H_
#define SRC_MODULES_GUIMODULE_GUIMODULE_H_

#include <string>

#include "Core/Engine/Engine.h"
#include "Core/Engine/Module.h"

// Export system
#include "Systems/GuiSystem.h"

namespace nv3dvc {
namespace modules {
namespace guimodule {

/// @defgroup GuiModuleProperties GuiModule
/// @ingroup  ModuleProperties
/// @brief    Module for displaying a graphical user interface (GUI) to allow the user to interact with properties
///
/// Registers the system nv3dvc::modules::guimodule::systems::GuiSystem

/// See @ref GuiModuleProperties
class GuiModule : public core::engine::Module {
 public:
  constexpr static const char* NAME = "GuiModule";
  std::string Name() const override { return NAME; }

  /// @brief Constructor
  /// @param[in,out] engine A reference to the engine, which properties can be modified using the GUI
  explicit GuiModule(core::engine::Engine* engine);

  ~GuiModule() override = default;

  nv3dvc::core::Error Initialize() override;
  nv3dvc::core::Error Uninitialize() override;

 private:
  core::engine::Engine* m_engine;
};

}  // namespace guimodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_GUIMODULE_GUIMODULE_H_
