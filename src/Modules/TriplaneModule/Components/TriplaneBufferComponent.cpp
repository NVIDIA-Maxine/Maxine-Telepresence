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

#include "TriplaneBufferComponent.h"

#include <cuda_runtime.h>

namespace nv3dvc {
namespace modules {
namespace triplanemodule {
namespace components {

TriplaneBufferComponent::TriplaneBufferComponent()
    : m_triplaneRingBufferReadIndex(0), m_triplaneRingBufferWriteIndex(0) {
  fp32_precision.SetOnChangeFunction([this]() { Initialize(); });
}

void TriplaneBufferComponent::Initialize() {
  NvCVImage_ComponentType component_type = fp32_precision ? NVCV_F32 : NVCV_U8;
  bool minmax_required = !fp32_precision;
  uint32_t memspace_minmax = minmax_required ? NVCV_CPU : NVCV_GPU;
  m_ringBuffer.resize(ring_buffer_size);
  for (TriplaneFrame& triplane_frame : m_ringBuffer) {
    if (triplane_frame.triplane_volume.triplanes.pixels != nullptr) {
      NvCVVolume_DeallocateTriplaneVolume(&triplane_frame.triplane_volume);
      triplane_frame.triplane_volume.triplanes.pixels = nullptr;
    }
    triplane_frame.triplane_volume.config_params = {
        96,               // Number of effective triplane channels within the structure
        4,                // Used to splay out the triplane channels in a grid
        6,                // Used to splay out the triplane channels in a grid
        256,              // Width of one triplane
        256,              // Width of one triplane
        NVCV_RGBA,        // The format of the pixels in the triplanes
        component_type,   // The type of the components of the pixels.
        NVCV_CHUNKY,      // NVCV_CHUNKY, or NVCV_PLANAR
        NVCV_GPU,         // Location of the triplane buffer: one of { NVCV_CPU, NVCV_GPU, NVCV_CUDA }
        memspace_minmax,  // Location of the mins, maxs buffers: one of { NVCV_CPU, NVCV_GPU, NVCV_CUDA }
        1,                // Row byte alignment.
    };
    NvCV_Status err = NvCVVolume_AllocateTriplaneVolume(&triplane_frame.triplane_volume);
    cudaMemset(triplane_frame.triplane_volume.triplanes.pixels, 0, triplane_frame.triplane_volume.triplanes.pixelBytes);
    if (err != NVCV_SUCCESS) {
      printf("Error: %s\n", NvCV_GetErrorStringFromCode(err));
    }
  }
}

TriplaneFrame* TriplaneBufferComponent::GetReadableTriplaneFrame() {
  const int read_index = m_triplaneRingBufferReadIndex.load();
  if (read_index >= m_triplaneRingBufferWriteIndex.load()) {
    // Empty.
    return nullptr;
  }
  return &m_ringBuffer[read_index % ring_buffer_size];
}

void TriplaneBufferComponent::SignalConsume() { m_triplaneRingBufferReadIndex++; }

TriplaneFrame* TriplaneBufferComponent::GetWriteableTriplaneFrame() {
  const int write_index = m_triplaneRingBufferWriteIndex.load();
  if (m_triplaneRingBufferReadIndex.load() + ring_buffer_size <= write_index) {
    // Full.
    return nullptr;
  }
  return &m_ringBuffer[write_index % ring_buffer_size];
}

void TriplaneBufferComponent::SignalProvide() { m_triplaneRingBufferWriteIndex++; }

float TriplaneBufferComponent::FilterConfidence(float confidence, float dt) {
  return m_confidenceFilter.Filter(confidence, dt);
}

}  // namespace components
}  // namespace triplanemodule
}  // namespace modules
}  // namespace nv3dvc
