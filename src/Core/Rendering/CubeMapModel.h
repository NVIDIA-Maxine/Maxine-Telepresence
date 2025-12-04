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

#ifndef SRC_CORE_RENDERING_CUBEMAPMODEL_H_
#define SRC_CORE_RENDERING_CUBEMAPMODEL_H_

#include <memory>
#include <string>

#include "Core/Error.h"
#include "Core/Rendering/Shader.h"
#include "CubeMapTexture.h"
#include "glad/gl.h"

namespace nv3dvc {
namespace core {
namespace rendering {

/// @brief Cube map model. Enables loading of skybox textures and drawing cube at infinity
class CubeMapModel {
 public:
  CubeMapModel() = default;
  ~CubeMapModel();

  /// @brief Load skybox textures
  /// @param[in] skybox_directory Within this folder, the six cube map images px.png, nx.png, py.png, ny.png, pz.png,
  ///                             nz.png are expected
  /// @return    Error::SUCCESS   If successful
  Error Load(const std::string& skybox_directory);

  /// @brief Draw the six inside faces of the cube map
  /// @param[in] shader          The cube map shader to use
  /// @param[in] model_transform The model transform
  /// @return    Error::SUCCESS  If successful
  Error Draw(Shader* shader, const glm::mat4& model_transform);

  /// @brief Delete the cube map model and its internal cube map texture
  /// @return Error::SUCCESS if successful
  Error Delete();

 private:
  bool m_initialized = false;
  unsigned int m_cubeVao = 0;
  unsigned int m_cubeVbo = 0;
  static const int kNumTriangles = 12;
  static const float kCubeVertexData[108];
  CubeMapTexture m_skybox;
};

}  // namespace rendering
}  // namespace core
}  // namespace nv3dvc

#endif  // SRC_CORE_RENDERING_CUBEMAPMODEL_H_
