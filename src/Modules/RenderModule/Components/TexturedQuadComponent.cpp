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

#include "TexturedQuadComponent.h"

#include "Core/Error.h"

namespace nv3dvc {
namespace modules {
namespace rendermodule {
namespace components {

TexturedQuadComponent::TexturedQuadComponent() {
  texture_scale.SetOnChangeFunction([&] {
    m_meshModel.SetVertexTextureCoordinates({
        {0.0f, texture_scale},
        {texture_scale, texture_scale},
        {0.0f, 0.0f},
        {texture_scale, 0.0f},
    });
  });
}

core::Error TexturedQuadComponent::Load() {
  core::Error err = core::Error::SUCCESS;

  CHECK_SUCCESS(m_meshModel.Initialize());
  CHECK_SUCCESS(m_meshModel.SetTexture(texture_path));
  CHECK_SUCCESS(m_meshModel.SetVertexPositions({
      {-1.0f, -1.0f, 0.0f},
      {1.0f, -1.0f, 0.0f},
      {-1.0f, 1.0f, 0.0f},
      {1.0f, 1.0f, 0.0f},
  }));
  CHECK_SUCCESS(m_meshModel.SetTriangleVertexIndices({
      {0, 1, 2},
      {3, 2, 1},
  }));
  texture_scale.OnChange();

bail:
  return err;
}

core::rendering::MeshModel* TexturedQuadComponent::GetMeshModelPtr() { return &m_meshModel; }

}  // namespace components
}  // namespace rendermodule
}  // namespace modules
}  // namespace nv3dvc
