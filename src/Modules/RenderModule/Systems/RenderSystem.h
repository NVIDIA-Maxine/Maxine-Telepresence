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

#ifndef SRC_MODULES_RENDERMODULE_SYSTEMS_RENDERSYSTEM_H_
#define SRC_MODULES_RENDERMODULE_SYSTEMS_RENDERSYSTEM_H_

#include <memory>
#include <string>
#include <vector>

#include "Core/EntityComponentSystem/System.h"
#include "Core/Properties/Property.h"
#include "Core/Rendering/Renderer.h"
#include "Modules/CommonModule/Components/CallbackComponent.h"

namespace nv3dvc {

namespace core {
namespace ecs {
namespace registry {
class EntityRegistry;
}  // namespace registry
}  // namespace ecs
}  // namespace core

namespace modules {
namespace rendermodule {
namespace systems {

/// @defgroup RenderSystemProperties RenderSystem
/// @ingroup  SystemProperties
/// @brief    System responsible for rendering object to the display buffer
///
/// See @ref RendererProperties for the properties of the renderer.
///
/// On scene load, the renderer will get initialized based on the one components::DisplayComponent in the scene. Any
/// required assets for renderable objects will also get loaded. The system requires one components::DisplayComponent
/// and one commonmodule::components::CameraComponent in the scene
///
/// During run, the following object types will get rendered
///
/// - Entities with:
///   - components::CubeMapComponent
///
/// - Entities with:
///   - components::RenderableTriplaneComponent, and either
///   - triplanemodule::components::TriplaneBufferComponent, or
///   - triplanemodule::components::TriplaneVolumeComponent
///
/// With commonmodule::components::TransformComponent attached, the objects' global transform will be used to determine
/// its location and orientation in the scene.
///
/// If a commonmodule::components::RecordingCallbackComponent is attached to the same entity as the
/// components::DisplayComponent, rendered frames can be recorded.

/// @see RenderSystemProperties
class RenderSystem : public core::ecs::System {
 public:
  constexpr static const char* NAME = "RenderSystem";
  std::string Name() const override { return NAME; };

  explicit RenderSystem(core::engine::Engine* engine);
  ~RenderSystem();

  nv3dvc::core::Error Initialize() override;
  nv3dvc::core::Error Uninitialize() override;
  core::Error OnLoadScene(core::ecs::registry::EntityRegistry* reg) override;
  core::Error OnUnloadScene(core::ecs::registry::EntityRegistry* reg) override;
  core::Error Run(core::ecs::registry::EntityRegistry* reg, float dt) override;
  core::Error OnEvent(core::ecs::registry::EntityRegistry* reg, core::events::Event* e) override;

 public:
  std::unique_ptr<core::rendering::Renderer> renderer;

  /// @ingroup RenderSystemProperties
  /// @{
  core::properties::Property<bool> record_frames_gpu = {
      this,
      "record_frames_gpu",
      "Whether frames should be recorded to GPU memory rather than CPU memory. Only effective if "
      "RecordingCallbackComponent exists in entities with DisplayComponent; otherwise recording is ignored.",
      false,
  };
  /// @}
 private:
  core::engine::Engine* m_engine = nullptr;

  uint32_t m_windowSizeX;  // Updated on window size event
  uint32_t m_windowSizeY;  // Updated on window size event
  bool m_isInitialized;
  std::vector<uint8_t> m_tmpRecordingBuffer;
};

}  // namespace systems
}  // namespace rendermodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_RENDERMODULE_SYSTEMS_RENDERSYSTEM_H_
