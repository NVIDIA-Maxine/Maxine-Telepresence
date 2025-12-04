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

#include "TriplaneVolumeComponent.h"

namespace nv3dvc {
namespace modules {
namespace triplanemodule {
namespace components {

TriplaneVolumeComponent::TriplaneVolumeComponent() {}

NvCV_Status TriplaneVolumeComponent::Allocate() {
  m_triplaneVolume.config_params = {
      96,           // Number of effective triplane channels within the structure
      4,            // Used to splay out the triplane channels in a grid
      6,            // Used to splay out the triplane channels in a grid
      256,          // Width of one triplane
      256,          // Width of one triplane
      NVCV_RGBA,    // The format of the pixels in the triplanes
      NVCV_F32,     // The type of the components of the pixels.
      NVCV_CHUNKY,  // NVCV_CHUNKY, or NVCV_PLANAR
      NVCV_GPU,     // Location of the triplane buffer: one of { NVCV_CPU, NVCV_GPU, NVCV_CUDA }
      NVCV_GPU,     // Location of the mins, maxs buffers: one of { NVCV_CPU, NVCV_GPU, NVCV_CUDA }
      1,            // Row byte alignment.
  };
  return NvCVVolume_AllocateTriplaneVolume(&m_triplaneVolume);
}

NvCVTriplaneVolume* TriplaneVolumeComponent::GetTriplaneVolumePtr() { return &m_triplaneVolume; }

}  // namespace components
}  // namespace triplanemodule
}  // namespace modules
}  // namespace nv3dvc
