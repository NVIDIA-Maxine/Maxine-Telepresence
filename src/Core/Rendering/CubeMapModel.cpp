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

#include "CubeMapModel.h"

#include <memory>
#include <string>

#include "Core/Util/Logger.h"
#include "CubeMapTexture.h"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/quaternion.hpp"

namespace nv3dvc {
namespace core {
namespace rendering {

const float CubeMapModel::kCubeVertexData[108] = {
    -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, 0.5f,  -0.5f, 0.5f,  0.5f, 0.5f,  0.5f,  -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
    0.5f,  -0.5f, 0.5f,  -0.5f, 0.5f,  -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f,  0.5f,  -0.5f, 0.5f,  -0.5f,
    -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, 0.5f, 0.5f,  -0.5f, 0.5f,  -0.5f, 0.5f,  -0.5f, 0.5f,
    -0.5f, -0.5f, 0.5f,  -0.5f, -0.5f, -0.5f, -0.5f, 0.5f,  0.5f, -0.5f, -0.5f, 0.5f,  0.5f,  -0.5f, 0.5f,  0.5f,
    0.5f,  0.5f,  0.5f,  -0.5f, -0.5f, 0.5f,  0.5f,  -0.5f, 0.5f, -0.5f, -0.5f, 0.5f,  0.5f,  0.5f,  0.5f,  -0.5f,
    0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  -0.5f, -0.5f, 0.5f, -0.5f, 0.5f,  0.5f,  0.5f,  -0.5f, 0.5f,  -0.5f,
    -0.5f, 0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  -0.5f, 0.5f,  0.5f, 0.5f,  -0.5f, 0.5f};

CubeMapModel::~CubeMapModel() { Delete(); }

Error CubeMapModel::Load(const std::string& skybox_directory) {
  Error err = Error::SUCCESS;
  if (m_cubeVao == 0) {
    // Prepare cube rendering
    CHECK_GLGETERROR(glGenVertexArrays(1, &m_cubeVao));
    CHECK_GLGETERROR(glGenBuffers(1, &m_cubeVbo));
    CHECK_GLGETERROR(glBindVertexArray(m_cubeVao));
    CHECK_GLGETERROR(glBindBuffer(GL_ARRAY_BUFFER, m_cubeVbo));
    CHECK_GLGETERROR(glBufferData(GL_ARRAY_BUFFER, sizeof(kCubeVertexData), &kCubeVertexData, GL_STATIC_DRAW));
    CHECK_GLGETERROR(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr));
    CHECK_GLGETERROR(glEnableVertexAttribArray(0));  // only position attribute
  }
  LOG_DEBUG("Loading skybox from directory \"%s\"", skybox_directory.c_str());
  CHECK_SUCCESS(m_skybox.Load(skybox_directory + "/px.png", skybox_directory + "/nx.png", skybox_directory + "/py.png",
                              skybox_directory + "/ny.png", skybox_directory + "/pz.png",
                              skybox_directory + "/nz.png"));
  m_initialized = true;
bail:
  return err;
}

Error CubeMapModel::Draw(Shader* shader, const glm::mat4& model_transform) {
  Error err = Error::SUCCESS;
  const GLuint texture_unit = 0;
  GLint front_face_mode;
  GLint depth_test_mode;
  CHECK_TRUE(m_initialized, Error::ERR_INITIALIZATION, "CubeMapModel has not been initialized");
  // set model matrix uniform
  shader->SetUniform("modelMatrix", &model_transform);

  // bind triplane texture
  CHECK_SUCCESS(m_skybox.TexUnit(shader, "skybox", texture_unit));
  CHECK_SUCCESS(m_skybox.Bind(texture_unit));

  // Get current state
  CHECK_GLGETERROR(glGetIntegerv(GL_FRONT_FACE, &front_face_mode));
  CHECK_GLGETERROR(glGetIntegerv(GL_DEPTH_TEST, &depth_test_mode));

  // Change state
  CHECK_GLGETERROR(glDisable(GL_DEPTH_TEST));  // prevents framebuffer rectangle from being discarded
  CHECK_GLGETERROR(glFrontFace(GL_CW));        // Render inside of cube, cull front side

  // draw cube
  CHECK_GLGETERROR(glBindVertexArray(m_cubeVao));
  for (int trIdx = 0; trIdx < kNumTriangles; trIdx++) {
    CHECK_GLGETERROR(glDrawArrays(GL_TRIANGLES, trIdx * 3, 3));
  }

  // Reset state
  CHECK_GLGETERROR(glFrontFace(front_face_mode));
  if (depth_test_mode) {
    CHECK_GLGETERROR(glEnable(GL_DEPTH_TEST));
  }
bail:
  return err;
}

Error CubeMapModel::Delete() {
  Error err = Error::SUCCESS;
  if (m_cubeVao != 0) {
    CHECK_GLGETERROR(glDeleteVertexArrays(1, &m_cubeVao));
    m_cubeVao = 0;
  }
  if (m_cubeVbo != 0) {
    CHECK_GLGETERROR(glDeleteBuffers(1, &m_cubeVbo));
    m_cubeVbo = 0;
  }
  CHECK_SUCCESS(m_skybox.Delete());
bail:
  return err;
}

}  // namespace rendering
}  // namespace core
}  // namespace nv3dvc
