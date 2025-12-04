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

#ifndef SRC_MODULES_WINDOWMODULE_WINDOWMODULE_H_
#define SRC_MODULES_WINDOWMODULE_WINDOWMODULE_H_

#include <string>

#include "Core/Application/Window.h"
#include "Core/Engine/Module.h"
#include "Core/Properties/Property.h"
#include "Modules/RenderModule/Components/DisplayComponent.h"

namespace nv3dvc {

namespace core {
namespace engine {
class Engine;
}  // namespace engine
}  // namespace core

namespace modules {
namespace windowmodule {

/// @defgroup WindowModuleProperties WindowModule
/// @ingroup ModuleProperties
/// @brief Module which will create the window upon initialization. The window creation carries the graphics context.
///
/// The WindowModule has a reference to an Engine, which allows it to pass events triggered by the window implementation
/// to the engine and all its registered systems

/// @see WindowModuleProperties
class WindowModule : public core::engine::Module {
 public:
  constexpr static const char* NAME = "WindowModule";
  std::string Name() const override { return NAME; };

  /// @brief Create a window module
  /// @param[in,out] engine The engine for which window events should be passed
  explicit WindowModule(core::engine::Engine* engine);

  nv3dvc::core::Error Initialize() override;

  nv3dvc::core::Error Uninitialize() override;

  nv3dvc::core::Error OnLoadScene(core::ecs::registry::EntityRegistry* reg) override;

  nv3dvc::core::Error Update(float dt) override;

  nv3dvc::core::Error EncodeProperties(nlohmann::json* json_description,
                                       const PropertyOwner* property_owner) const override;

  nv3dvc::core::Error DecodeProperties(const nlohmann::json& json_description,
                                       PropertyOwner* property_owner) const override;

 public:
  /// @ingroup WindowModuleProperties
  /// @{
  core::properties::Property<unsigned> window_width = {
      this,
      "window_width",
      "Width of the window in pixels. Only applicable if enforce_window_resolution=true",
      1920,
  };
  core::properties::Property<unsigned> window_height = {
      this,
      "window_height",
      "Height of the window in pixels. Only applicable if enforce_window_resolution=true",
      1080,
  };
  core::properties::Property<core::application::Window::WindowMode> window_mode = {
      this,
      "window_mode",
      "Mode in which to display the window.",
      core::application::Window::WindowMode::UNDECORATED,
  };
  core::properties::Property<bool> enforce_window_resolution = {
      this,
      "enforce_window_resolution",
      "Whether the window resolution should be enforced by window_width and window_height. If false, the window "
      "will cover the full screen. This option should typically be set to false for telepresence experiences as "
      "the view dependent display intrisics are set from the properties of the full screen",
      false,
  };
  core::properties::Property<bool> enable_vsync = {
      this,
      "enable_vsync",
      "Whether to enable vertical synchronization when rendering to the display buffer. If true, the "
      "render rate will be matched with that of the display. If false, vertical tearing may be displayed.",
      true,
  };
  /// @}

 private:
  core::engine::Engine* m_engine;
  core::application::Window m_window;
};

}  // namespace windowmodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_WINDOWMODULE_WINDOWMODULE_H_
