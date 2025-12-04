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

#ifndef SRC_MODULES_VIDEOEFFECTSMODULE_VIDEOEFFECTSMODULE_H_
#define SRC_MODULES_VIDEOEFFECTSMODULE_VIDEOEFFECTSMODULE_H_

#include <string>

#include "Components/VideoEffectsComponent.h"
#include "Core/Engine/Module.h"
#include "Systems/AigsSystem.h"
namespace nv3dvc {
namespace modules {
namespace videoeffectsmodule {

/// @brief Module for applying video effects
///
/// Registers the following systems:
/// * AigsSystem
///
/// Registers the following components:
/// * VideoEffectsComponent
class VideoEffectsModule : public core::engine::Module {
 public:
  constexpr static const char* NAME = "VideoEffectsModule";
  std::string Name() const override { return "VideoEffectsModule"; };

  explicit VideoEffectsModule(core::engine::Engine* engine);

  core::Error Initialize() override;

  core::Error Uninitialize() override;

  core::Error Update(float dt) override;

 private:
  core::engine::Engine* m_engine;
};

}  // namespace videoeffectsmodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_VIDEOEFFECTSMODULE_VIDEOEFFECTSMODULE_H_
