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

#ifndef SRC_MODULES_VIDEOEFFECTSMODULE_SYSTEMS_AIGSSYSTEM_H_
#define SRC_MODULES_VIDEOEFFECTSMODULE_SYSTEMS_AIGSSYSTEM_H_

#include <string>

#include "Core/Engine/Engine.h"
#include "Core/EntityComponentSystem/System.h"
#include "Core/Properties/Property.h"
#include "nvVideoEffects.h"

namespace nv3dvc {

namespace core {
namespace ecs {
namespace registry {
class EntityRegistry;
}  // namespace registry
}  // namespace ecs
}  // namespace core

namespace modules {
namespace videoeffectsmodule {
namespace systems {

/// @brief System for running AI Green Screen (AIGS) on video frames
///
/// The system acts on entities with one VideoFrameComponent and one VideoEffectsComponent attached, and only if the
/// VideoEffectsComponents property perform_aigs is set to true. The system modifies the video frame's buffer by
/// computing a matte image, and applying it as an overlay with the color defined in the VideoEffectsComponent. The
/// matte image represets the separation between foreground and background where the human subject is in the foreground.
/// This separation acts as a typical green screen when the matte overlay is applied.
class AigsSystem : public core::ecs::System {
 public:
  constexpr static const char* NAME = "AigsSystem";
  std::string Name() const override { return "AigsSystem"; };

  AigsSystem();

  core::Error Initialize() override;

  core::Error Uninitialize() override;

  core::Error Run(core::ecs::registry::EntityRegistry* reg, float dt);

 public:
  /// @defgroup AigsSystemProperties AigsSystem
  /// @ingroup SystemProperties
  /// @{
  core::properties::Property<std::string> vfx_sdk_model_dir = {
      this,
      "vfx_sdk_model_dir",
      "The path from where to load VideoEffects TRT models",
      core::engine::Engine::GetVfxSdkDir() + "bin/models/",
  };
  core::properties::Property<uint32_t> effects_mode = {
      this,
      "effects_mode",
      "AIGS mode available. 0 - Quality mode, 1 - Performance mode, 2 - Quality mode with chair background "
      "segmentation, 3 - Performance mode with chair background segmentation",
      2,
  };
  core::properties::Property<bool> cuda_graph = {
      this,
      "cuda_graph",
      "Enable cuda graph (0|1)",
      false,
  };
  /// @}

 private:
  uint32_t m_maxInputWidth;   // May change depending on input image
  uint32_t m_maxInputHeight;  // May change depending on input image

  NvVFX_Handle m_aigsEffect;
  NvCVImage m_tmpImg;
};

}  // namespace systems
}  // namespace videoeffectsmodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_VIDEOEFFECTSMODULE_SYSTEMS_AIGSSYSTEM_H_
