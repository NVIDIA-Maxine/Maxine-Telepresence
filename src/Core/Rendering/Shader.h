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

#ifndef SRC_CORE_RENDERING_SHADER_H_
#define SRC_CORE_RENDERING_SHADER_H_

#include <map>
#include <string>

#include "Core/Error.h"
#include "glad/gl.h"
#include "glm/glm.hpp"

namespace nv3dvc {
namespace core {
namespace rendering {

/// @brief OpenGL implementation of shader program
class Shader {
 public:
  Shader() = default;
  ~Shader();

  /// @brief Create shader from GLSL source
  /// @param[in] vertex_source_code   Vertex shader source
  /// @param[in] fragment_source_code Fragment shader source
  /// @return    Error::SUCCESS   If successful
  ///            Error::ERR_PARSE If shaders did not compile successfully
  Error CreateShaderFromSource(const std::string& vertex_source_code, const std::string& fragment_source_code);

  /// @brief Activate the shader program
  /// @return Error::SUCCESS If successful
  Error Activate();

  /// @brief Deactivate any shader program
  /// @return Error::SUCCESS If successful
  Error Deactivate();

  /// @brief Delete the shader program
  /// @return Error::SUCCESS If successful
  Error Delete();

  /// @brief Set a float attribute to use in the shader
  /// @param[in] name      The name of the attribute
  /// @param[in] size      The number of components per generic vertex attribute. Must be 1, 2, 3, 4
  /// @param[in] stride    The byte offset between consecutive generic vertex attributes
  /// @param[in] offset    The byte offset from the start of the attribute array
  /// @param[in] normalize Whether fixed-point data values should be normalized
  /// @return    Error::SUCCESS If successful
  Error SetFloatAttribute(const std::string& name, int size, int stride, uint32_t offset, bool normalize = false);

  Error SetUniform(const std::string& name, uint32_t val);
  Error SetUniform(const std::string& name, int val);
  Error SetUniform(const std::string& name, float val);
  Error SetUniform(const std::string& name, float x, float y);
  Error SetUniform(const std::string& name, float x, float y, float z);
  Error SetUniform(const std::string& name, float x, float y, float z, float w);
  Error SetUniform(const std::string& name, const glm::vec2* v, int count = 1);
  Error SetUniform(const std::string& name, const glm::vec3* v, int count = 1);
  Error SetUniform(const std::string& name, const glm::vec4* v, int count = 1);
  Error SetUniform(const std::string& name, const glm::mat4* m, int count = 1);
  Error SetUniform(const std::string& name, const glm::mat3* m, int count = 1);

 private:
  /// @brief Get a uniform location and cache it if new
  /// @param[in] name The name of the uniform in the shader program
  /// @return    The uniform location ID
  /// @return    -1 if the uniform does not exist in the program
  GLint Uniform(const std::string& name);

  /// @brief Get an attribute location and cache it if new
  /// @param[in] name The name of the attribute in the shader program
  /// @return    -1 if the attribute does not exist in the program
  GLint Attribute(const std::string& name);

  GLuint m_handle = 0;
  std::string m_vertexSource;
  std::string m_fragmentSource;
  std::map<std::string, GLint> m_uniforms;
  std::map<std::string, GLint> m_attributes;
};

}  // namespace rendering
}  // namespace core
}  // namespace nv3dvc

#endif  // SRC_CORE_RENDERING_SHADER_H_
