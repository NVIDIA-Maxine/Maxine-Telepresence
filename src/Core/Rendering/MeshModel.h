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

#ifndef SRC_CORE_RENDERING_MESHMODEL_H_
#define SRC_CORE_RENDERING_MESHMODEL_H_

#include <Core/Rendering/Shader.h>

#include <string>
#include <vector>

#include "Core/Error.h"

namespace nv3dvc {
namespace core {
namespace rendering {

class MeshModel {
 public:
  MeshModel();
  ~MeshModel();

  Error Initialize();
  Error Uninitialize();

  Error SetTexture(const std::string& texture_path);

  Error SetVertexPositions(const std::vector<glm::vec3>& positions);

  Error SetVertexNormals(const std::vector<glm::vec3>& normals);

  Error SetVertexTextureCoordinates(const std::vector<glm::vec2>& texture_coords);

  Error SetTriangleVertexIndices(const std::vector<glm::ivec3>& vertex_indices);

  Error Draw(Shader* shader, const glm::mat4& model_transform);

 private:
  GLuint m_textureId = 0;
  GLuint m_vao = 0, m_triangleEbo = 0, m_positionVbo = 0, m_normalVbo = 0, m_texcoordVbo = 0;
  GLsizei m_numTriangles = 0;
  bool m_initialized = false;
};

}  // namespace rendering
}  // namespace core
}  // namespace nv3dvc

#endif  // SRC_CORE_RENDERING_MESHMODEL_H_
