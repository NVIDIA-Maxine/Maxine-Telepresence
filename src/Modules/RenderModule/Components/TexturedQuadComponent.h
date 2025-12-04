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

#ifndef SRC_MODULES_RENDERMODULE_COMPONENTS_TEXTUREDQUADCOMPONENT_H_
#define SRC_MODULES_RENDERMODULE_COMPONENTS_TEXTUREDQUADCOMPONENT_H_

#include <string>

#include "Core/Engine/Engine.h"
#include "Core/EntityComponentSystem/Component.h"
#include "Core/Rendering/MeshModel.h"

namespace nv3dvc {
namespace modules {
namespace rendermodule {
namespace components {

/// @defgroup TexturedQuadComponentProperties TexturedQuadComponent
/// @ingroup  ComponentProperties
/// @brief    Represents a textured quad to be rendered in the 3D scene.
///

/// See @ref TexturedQuadComponentProperties
class TexturedQuadComponent : public core::ecs::Component {
 public:
  constexpr static const char* NAME = "TexturedQuadComponent";
  std::string Name() const override { return NAME; };

  TexturedQuadComponent();

  core::Error Load();

  core::rendering::MeshModel* GetMeshModelPtr();

 public:
  /// @defgroup TexturedQuadComponentProperties TexturedQuadComponent
  /// @ingroup  ComponentProperties
  /// @{
  core::properties::Property<std::string> texture_path = {
      this,
      "texture_path",
      "Path to the image file to load",
      core::engine::Engine::GetResourcesDir() + "skyboxes/small_empty_room_3_8k/nz.png",
  };
  core::properties::Property<float> texture_scale = {
      this,
      "texture_scale",
      "Texture image scale -- larger values will cause the texture to get smaller and repeat",
      1.0f,
  };
  /// @}

 private:
  core::rendering::MeshModel m_meshModel;
};

}  // namespace components
}  // namespace rendermodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_RENDERMODULE_COMPONENTS_TEXTUREDQUADCOMPONENT_H_
