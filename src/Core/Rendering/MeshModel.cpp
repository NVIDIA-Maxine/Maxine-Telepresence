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

#include "MeshModel.h"

#include <string>

#include "Core/Error.h"
#include "Core/Rendering/Shader.h"
#include "Shader.h"
#include "opencv2/opencv.hpp"

namespace nv3dvc {
namespace core {
namespace rendering {

MeshModel::MeshModel() : m_initialized(false) {}

MeshModel::~MeshModel() { Uninitialize(); }

Error MeshModel::Initialize() {
  Error err = Error::SUCCESS;

  // Generate VAO.
  CHECK_GLGETERROR(glGenVertexArrays(1, &m_vao));

bail:
  m_initialized = err == Error::SUCCESS;
  return err;
}

Error MeshModel::Uninitialize() {
  Error err = Error::SUCCESS;

  if (m_textureId) {
    glDeleteTextures(1, &m_textureId);
    m_textureId = 0;
    if (err == Error::SUCCESS) err = static_cast<Error>(glGetError());  // Preserve first error.
  }

  if (m_positionVbo) {
    glDeleteBuffers(1, &m_positionVbo);
    m_positionVbo = 0;
    if (err == Error::SUCCESS) err = static_cast<Error>(glGetError());  // Preserve first error.
  }

  if (m_normalVbo) {
    glDeleteBuffers(1, &m_normalVbo);
    m_normalVbo = 0;
    if (err == Error::SUCCESS) err = static_cast<Error>(glGetError());  // Preserve first error.
  }

  if (m_texcoordVbo) {
    glDeleteBuffers(1, &m_texcoordVbo);
    m_texcoordVbo = 0;
    if (err == Error::SUCCESS) err = static_cast<Error>(glGetError());  // Preserve first error.
  }

  if (m_vao) {
    glDeleteVertexArrays(1, &m_vao);
    m_vao = 0;
    if (err == Error::SUCCESS) err = static_cast<Error>(glGetError());  // Preserve first error.
  }

bail:
  m_initialized = false;
  return err;
}

Error MeshModel::SetTexture(const std::string& texture_path) {
  Error err = Error::SUCCESS;
  GLenum pixel_format = GL_RGB;
  auto pixel_data_type = GL_UNSIGNED_BYTE;
  GLenum internal_format = 0;

  cv::Mat img = cv::imread(texture_path);
  const int width = img.cols;
  const int height = img.rows;
  const int num_channels = img.channels();
  CHECK_NONNULL(img.data, Error::ERR_FILE);
  CHECK_TRUE(img.type() == CV_8UC1 || img.type() == CV_8UC3 || img.type() == CV_8UC4, Error::ERR_GENERAL);

  if (num_channels == 1) {
    pixel_format = GL_RED;
    internal_format = GL_R8;
  } else if (num_channels == 3) {
    pixel_format = GL_RGB;
    internal_format = GL_RGB8;
    cv::cvtColor(img, img, cv::COLOR_BGR2RGB);
  } else if (num_channels == 4) {
    pixel_format = GL_RGBA;
    internal_format = GL_RGBA8;
    cv::cvtColor(img, img, cv::COLOR_BGRA2RGBA);
  } else {
    CHECK_SUCCESS(Error::ERR_NOT_SUPPORTED);  // Pixel format
  }

  // Generates an OpenGL texture object
  CHECK_GLGETERROR(glGenTextures(1, &m_textureId));
  CHECK_GLGETERROR(glBindTexture(GL_TEXTURE_2D, m_textureId));
  CHECK_GLGETERROR(
      glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, pixel_format, pixel_data_type, img.data));
  CHECK_GLGETERROR(glGenerateMipmap(GL_TEXTURE_2D));
  CHECK_GLGETERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
  CHECK_GLGETERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
  CHECK_GLGETERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
  CHECK_GLGETERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
  CHECK_GLGETERROR(glBindTexture(GL_TEXTURE_2D, 0));

bail:
  return err;
}

Error MeshModel::SetVertexPositions(const std::vector<glm::vec3>& positions) {
  Error err = Error::SUCCESS;

  if (m_positionVbo == 0) CHECK_GLGETERROR(glGenBuffers(1, &m_positionVbo));

  CHECK_GLGETERROR(glBindVertexArray(m_vao));

  // Upload the data to the buffer.
  CHECK_GLGETERROR(glBindBuffer(GL_ARRAY_BUFFER, m_positionVbo));
  CHECK_GLGETERROR(
      glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(glm::vec3), positions.data(), GL_STATIC_DRAW));

  // Enable the position vertex attribute at location 0 from the buffer.
  CHECK_GLGETERROR(glEnableVertexAttribArray(0));
  CHECK_GLGETERROR(glVertexAttribPointer(0, glm::vec3::length(), GL_FLOAT, GL_FALSE, sizeof(glm::vec3), 0));

  CHECK_GLGETERROR(glBindVertexArray(0));

bail:
  return err;
}

Error MeshModel::SetVertexNormals(const std::vector<glm::vec3>& normals) {
  Error err = Error::SUCCESS;

  if (m_normalVbo == 0) CHECK_GLGETERROR(glGenBuffers(1, &m_normalVbo));

  CHECK_GLGETERROR(glBindVertexArray(m_vao));

  // Upload the data to the buffer.
  CHECK_GLGETERROR(glBindBuffer(GL_ARRAY_BUFFER, m_normalVbo));
  CHECK_GLGETERROR(glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_STATIC_DRAW));

  // Enable the normal vertex attribute at location 1 from the buffer.
  CHECK_GLGETERROR(glEnableVertexAttribArray(1));
  CHECK_GLGETERROR(glVertexAttribPointer(1, glm::vec3::length(), GL_FLOAT, GL_FALSE, sizeof(glm::vec3), 0));

  CHECK_GLGETERROR(glBindVertexArray(0));

bail:
  return err;
}

Error MeshModel::SetVertexTextureCoordinates(const std::vector<glm::vec2>& texture_coords) {
  Error err = Error::SUCCESS;

  if (m_texcoordVbo == 0) CHECK_GLGETERROR(glGenBuffers(1, &m_texcoordVbo));

  CHECK_GLGETERROR(glBindVertexArray(m_vao));

  // Upload the data to the buffer.
  CHECK_GLGETERROR(glBindBuffer(GL_ARRAY_BUFFER, m_texcoordVbo));
  CHECK_GLGETERROR(
      glBufferData(GL_ARRAY_BUFFER, texture_coords.size() * sizeof(glm::vec2), texture_coords.data(), GL_STATIC_DRAW));

  // Enable the texcoord vertex attribute at location 2 from the buffer.
  CHECK_GLGETERROR(glEnableVertexAttribArray(2));
  CHECK_GLGETERROR(glVertexAttribPointer(2, glm::vec2::length(), GL_FLOAT, GL_FALSE, sizeof(glm::vec2), 0));

  CHECK_GLGETERROR(glBindVertexArray(0));

bail:
  return err;
}

Error MeshModel::SetTriangleVertexIndices(const std::vector<glm::ivec3>& vertex_indices) {
  Error err = Error::SUCCESS;

  if (m_triangleEbo == 0) CHECK_GLGETERROR(glGenBuffers(1, &m_triangleEbo));

  CHECK_GLGETERROR(glBindVertexArray(m_vao));

  // Upload the data to the buffer.
  CHECK_GLGETERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_triangleEbo));
  CHECK_GLGETERROR(glBufferData(GL_ELEMENT_ARRAY_BUFFER, vertex_indices.size() * sizeof(glm::ivec3),
                                vertex_indices.data(), GL_STATIC_DRAW));

  CHECK_GLGETERROR(glBindVertexArray(0));

  m_numTriangles = vertex_indices.size();

bail:
  return err;
}

Error MeshModel::Draw(Shader* shader, const glm::mat4& model_transform) {
  Error err = Error::SUCCESS;

  const GLuint texture_unit = 0;

  CHECK_TRUE(m_initialized, Error::ERR_INITIALIZATION);

  if (m_numTriangles == 0) return err;

  shader->Activate();

  CHECK_GLGETERROR(shader->SetUniform("modelMatrix", &model_transform));

  CHECK_GLGETERROR(shader->SetUniform("tex", GLint(texture_unit)));
  CHECK_GLGETERROR(glActiveTexture(GL_TEXTURE0 + texture_unit));
  CHECK_GLGETERROR(glBindTexture(GL_TEXTURE_2D, m_textureId));

  CHECK_GLGETERROR(glBindVertexArray(m_vao));
  CHECK_GLGETERROR(glDrawElements(GL_TRIANGLES, m_numTriangles * glm::ivec3::length(), GL_UNSIGNED_INT, 0));

  shader->Deactivate();

bail:
  return err;
}

}  // namespace rendering
}  // namespace core
}  // namespace nv3dvc
