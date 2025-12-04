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

#include "RenderableTriplaneComponent.h"

#include <glm/glm.hpp>

namespace nv3dvc {
namespace modules {
namespace rendermodule {
namespace components {

void RenderableTriplaneComponent::SetPtr(NvCVTriplaneVolume* triplane_volume_ptr) {
  m_triplaneVolumePtr = triplane_volume_ptr;
}

NvCVTriplaneVolume* RenderableTriplaneComponent::GetPtr() { return m_triplaneVolumePtr; }

float RenderableTriplaneComponent::FilterConfidence(float confidence, float dt) {
  return m_confidenceFilter.Filter(confidence, dt);
}

float RenderableTriplaneComponent::FocalScaleStrength() const { return focal_scale_strength; }

}  // namespace components
}  // namespace rendermodule
}  // namespace modules
}  // namespace nv3dvc
