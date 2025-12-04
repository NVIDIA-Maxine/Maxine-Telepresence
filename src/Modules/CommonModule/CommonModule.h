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

#ifndef SRC_MODULES_COMMONMODULE_COMMONMODULE_H_
#define SRC_MODULES_COMMONMODULE_COMMONMODULE_H_

#include <string>

#include "Core/Engine/Module.h"
#include "glm/fwd.hpp"

// Export components and systems
#include "Components/CallbackComponent.h"
#include "Components/CameraCalibrationComponent.h"
#include "Components/CameraComponent.h"
#include "Components/DataBufferComponent.h"
#include "Components/TransformComponent.h"
#include "Components/VideoFrameComponent.h"

namespace nv3dvc {
namespace modules {
namespace commonmodule {

/// @brief Computes the global transform matrix
///
/// The global transform matrix specifies the chain of transforms from the current entity, its parent, its parent's
/// parent and so on up to the root. Note that to allow for a well-defined unit of translation for the full chain of
/// transforms, the scale of any of the parents are not factored in to the transform. Scaling is only applied to the
/// local transform of the object which gets incorporated with the parent transform in this function. This has the
/// effect that applying scaling to an object does not change the 3D location of its children. For any entity that has
/// the "relative" field of its TransformComponent set to false, the global transform is the same as the local transform
/// component.
/// @param entity The entity for which we want to compute the global transform matrix
/// @return       The global transform matrix
glm::mat4 GetGlobalTransform(const core::ecs::Entity& entity);

/// @brief Computes the global transform matrix using temporally filtered transform
///
/// See GetGlobalTransform
/// @param entity The entity for which we want to compute the global transform matrix
/// @return       The filtered global transform matrix
glm::mat4 GetGlobalTransformFiltered(const core::ecs::Entity& entity);

/// @defgroup CommonModuleProperties CommonModule
/// @ingroup  ModuleProperties
/// @brief    Module for registering common components used by other modules
///
/// Registers components:
/// - components::CameraComponent
/// - components::CameraCalibrationComponent
/// - components::DataBufferComponent
/// - components::TransformComponent
/// - components::VideoFrameComponent
/// - components::EncodedVideoBufferComponent
/// - components::EncodedVideoCallbackComponent
/// - components::RecordingCallbackComponent
/// - components::WebCameraAudioCallbackComponent

/// See @ref CommonModuleProperties
class CommonModule : public core::engine::Module {
 public:
  constexpr static const char* NAME = "CommonModule";
  std::string Name() const override { return NAME; };

  CommonModule();
  ~CommonModule() override;

  core::Error Initialize() override;
  core::Error Uninitialize() override;
  core::Error Update(float dt) override { return core::SUCCESS; }
};

}  // namespace commonmodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_COMMONMODULE_COMMONMODULE_H_
