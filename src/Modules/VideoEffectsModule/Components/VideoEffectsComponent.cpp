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

#include "VideoEffectsComponent.h"

#include <cuda_runtime.h>

#include <glm/gtc/type_ptr.hpp>

namespace nv3dvc {
namespace modules {
namespace videoeffectsmodule {
namespace components {

VideoEffectsComponent::VideoEffectsComponent() : m_bgColorDevicePtr(nullptr) {
  if (!m_bgColorDevicePtr) {
    cudaMalloc(&m_bgColorDevicePtr, sizeof(*background_color.get()));
  }
  const auto set_color_fn = [this]() {
    glm::u8vec3& color = *background_color.get();
    glm::u8vec3 color_bgr = {color.b, color.g, color.r};
    cudaMemcpy(m_bgColorDevicePtr, glm::value_ptr(color_bgr), sizeof(color_bgr), cudaMemcpyHostToDevice);
  };
  background_color.SetOnChangeFunction(set_color_fn);
  set_color_fn();
}

VideoEffectsComponent::~VideoEffectsComponent() {
  if (m_bgColorDevicePtr) {
    cudaFree(m_bgColorDevicePtr);
  }
}

bool VideoEffectsComponent::ShouldPerformAigs() const { return perform_aigs.get()->load(); }

NvCVImage* VideoEffectsComponent::GetMatteImagePtr() { return &m_matteImage; }

void* VideoEffectsComponent::GetBackgroundColorDevicePtr() { return m_bgColorDevicePtr; }

}  // namespace components
}  // namespace videoeffectsmodule
}  // namespace modules
}  // namespace nv3dvc
