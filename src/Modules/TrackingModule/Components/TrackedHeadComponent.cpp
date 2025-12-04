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

#include "TrackedHeadComponent.h"

namespace nv3dvc {
namespace modules {
namespace trackingmodule {
namespace components {

glm::vec3 TrackedHeadComponent::GetBboxMin() const { return tracked_bbox_min_cm; }

glm::vec3 TrackedHeadComponent::GetBboxMax() const { return tracked_bbox_max_cm; }

float TrackedHeadComponent::GetFaceCropScale() const { return face_crop_scale; }

glm::vec2 TrackedHeadComponent::GetFaceCropOffset() const { return face_crop_offset; }

glm::vec3 TrackedHeadComponent::TrackedTranslationWithOffset() {
  std::lock_guard lock(m_translationOffsetMutexLock);
  return face_box_to_display_translation + *translation_offset.get();
}

void TrackedHeadComponent::SetTranslationOffset(glm::vec3 offset) {
  std::lock_guard lock(m_translationOffsetMutexLock);
  translation_offset = offset;
}

}  // namespace components
}  // namespace trackingmodule
}  // namespace modules
}  // namespace nv3dvc
