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

#ifndef SRC_MODULES_RENDERMODULE_COMPONENTS_RENDERABLETRIPLANECOMPONENT_H_
#define SRC_MODULES_RENDERMODULE_COMPONENTS_RENDERABLETRIPLANECOMPONENT_H_

#include <string>

#include "Core/EntityComponentSystem/Component.h"
#include "Core/Util/OneEuroFilter.h"
#include "nvCVTriplaneVolume.h"

namespace nv3dvc {
namespace modules {
namespace rendermodule {
namespace components {

/// @defgroup RenderableTriplaneComponentProperties RenderableTriplaneComponent
/// @ingroup  ComponentProperties
/// @brief    Component representing a renderable triplane object
///
/// The entity carrying the RenderableTriplaneComponent is expected to have either a
/// triplanemodule::TriplaneBufferComponent or a triplanemodule::TriplaneVolumeComponent attached as this component by
/// itself does not carry actual triplane object data. It merely signals which triplane object to render.

/// @see RenderableTriplaneComponentProperties
class RenderableTriplaneComponent : public core::ecs::Component {
 public:
  constexpr static const char* NAME = "RenderableTriplaneComponent";
  std::string Name() const override { return NAME; };

  RenderableTriplaneComponent() = default;

  /// @brief Set the pointer to the triplane object to render
  /// @param[in] triplane_volume_ptr The pointer to the triplane object to render
  void SetPtr(NvCVTriplaneVolume* triplane_volume_ptr);

  /// @brief Get the pointer to the triplane object to render
  /// @return The pointer to the triplane object to render
  NvCVTriplaneVolume* GetPtr();

  /// @brief Filter confidence value used to render faded out triplane volume object
  /// To avoid flickering when the confidence value fluctuates
  /// @param confidence The current confidence
  /// @param dt         Delta time [Seconds]
  /// @return           The filtered confidence value
  float FilterConfidence(float confidence, float dt);

  /// @brief Factor with which to apply corrective scaling based on focal scale
  /// See TriplanePackage::focalScale
  /// @return the focal scale strength
  float FocalScaleStrength() const;

 public:
  /// @ingroup RenderableTriplaneComponentProperties
  /// @{
  core::properties::Property<float> focal_scale_strength = {
      this,
      "focal_scale_strength",
      "Factor with which to apply corrective scaling based on focal scale",
      1.0f,
  };
  /// @}

 private:
  NvCVTriplaneVolume* m_triplaneVolumePtr = nullptr;
  core::util::filter::OneEuroFilter<float> m_confidenceFilter;
};

}  // namespace components
}  // namespace rendermodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_RENDERMODULE_COMPONENTS_RENDERABLETRIPLANECOMPONENT_H_
