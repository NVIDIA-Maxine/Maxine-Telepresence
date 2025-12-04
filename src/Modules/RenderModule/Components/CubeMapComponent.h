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

#ifndef SRC_MODULES_RENDERMODULE_COMPONENTS_CUBEMAPCOMPONENT_H_
#define SRC_MODULES_RENDERMODULE_COMPONENTS_CUBEMAPCOMPONENT_H_

#include <string>

#include "Core/Engine/Engine.h"
#include "Core/EntityComponentSystem/Component.h"
#include "Core/Rendering/CubeMapModel.h"
#include "Core/Rendering/Shader.h"

namespace nv3dvc {
namespace modules {
namespace rendermodule {
namespace components {

/// @defgroup CubeMapComponentProperties CubeMapComponent
/// @ingroup  ComponentProperties
/// @brief    Component representing a renderable cubemap/skybox object

/// @see CubeMapComponentProperties
class CubeMapComponent : public core::ecs::Component {
 public:
  constexpr static const char* NAME = "CubeMapComponent";
  std::string Name() const override { return NAME; };

  CubeMapComponent() = default;

  /// @brief Load the cube map textures from disk
  /// @return core::Error::SUCCESS If successful.
  core::Error Load();

  /// @brief Get the raw pointer to the cube map model
  /// @return The raw pointer to the cube map model
  core::rendering::CubeMapModel* GetCubeMapModelPtr();

 public:
  /// @ingroup CubeMapComponentProperties
  /// @{
  core::properties::Property<std::string> skybox_directory = {
      this,
      "skybox_directory",
      "The directory within which the cube map textures are stored. Within this folder, the six cube map images "
      "px.png, nx.png, py.png, ny.png, pz.png, nz.png are expected.",
      core::engine::Engine::GetResourcesDir() + "skyboxes/gradient",
  };
  core::properties::Property<bool> is_infinite = {
      this,
      "is_infinite",
      "Whether the cube is infinite, else matching the display box.",
      false,
  };
  /// @}

 private:
  core::rendering::CubeMapModel m_cubeMapModel;
};

}  // namespace components
}  // namespace rendermodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_RENDERMODULE_COMPONENTS_CUBEMAPCOMPONENT_H_
