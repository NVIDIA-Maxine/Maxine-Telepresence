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

#include "Shader.h"

#include "Core/Error.h"
#include "Core/Util/Logger.h"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Static function declarations                                                                                     ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Checks if different shaders have compiled or linked properly
/// @param[in]  shader    The shader handle, or the program handle
/// @param[in]  type      Use "PROGRAM" if `shader` is a linked program. Use the shader type, e.g. "VERTEX" or
/// "FRAGMENT"
///                       otherwise
/// @param[out] succeeded True if compilation or linking succeeded, else false
/// Error::SUCCESS        If no GL error. Even if the compilation failed. Use succeeded
static nv3dvc::core::Error CompileErrors(GLuint shader, const std::string& type, bool* succeeded);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Static function definitions                                                                                      ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

nv3dvc::core::Error CompileErrors(const GLuint shader, const std::string& type, bool* succeeded) {
  // Stores status of compilation
  GLint gl_succeeded;
  nv3dvc::core::Error err = nv3dvc::core::Error::SUCCESS;
  // Character array to store error message in
  char info_log[1024];
  if (type == "PROGRAM") {
    CHECK_GLGETERROR(glGetProgramiv(shader, GL_LINK_STATUS, &gl_succeeded));
    if (gl_succeeded == GL_FALSE) {
      CHECK_GLGETERROR(glGetProgramInfoLog(shader, 1024, nullptr, info_log));
    }
  } else {
    CHECK_GLGETERROR(glGetShaderiv(shader, GL_COMPILE_STATUS, &gl_succeeded));
    if (gl_succeeded == GL_FALSE) {
      CHECK_GLGETERROR(glGetShaderInfoLog(shader, 1024, nullptr, info_log));
    }
  }
bail:
  if (gl_succeeded == GL_TRUE) {
    *succeeded = true;
  } else {
    *succeeded = false;
    LOG_ERROR("SHADER_COMPILATION_ERROR for %s : %s", type.c_str(), info_log);
  }
  return err;
}

namespace nv3dvc {
namespace core {
namespace rendering {

Shader::~Shader() { Shader::Delete(); }

core::Error Shader::CreateShaderFromSource(const std::string& vertex_source_code,
                                           const std::string& fragment_source_code) {
  core::Error err = core::Error::SUCCESS;
  bool shader_compiled = false;
  bool program_compiled = false;
  GLuint vertex_shader = 0;
  GLuint fragment_shader = 0;
  GLuint program = 0;
  m_vertexSource = std::string(vertex_source_code);
  m_fragmentSource = std::string(fragment_source_code);

  const char* v_source = m_vertexSource.c_str();
  const char* f_source = m_fragmentSource.c_str();

  // Create and compile vertex shader
  vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  CHECK_SUCCESS(static_cast<core::Error>(glGetError()));
  CHECK_GLGETERROR(glShaderSource(vertex_shader, 1, &v_source, nullptr));
  CHECK_GLGETERROR(glCompileShader(vertex_shader));
  CHECK_SUCCESS(CompileErrors(vertex_shader, "VERTEX", &shader_compiled));
  CHECK_TRUE(shader_compiled, core::Error::ERR_PARSE);

  // Create and compile fragment shader
  fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  CHECK_SUCCESS(static_cast<core::Error>(glGetError()));
  CHECK_GLGETERROR(glShaderSource(fragment_shader, 1, &f_source, nullptr));
  CHECK_GLGETERROR(glCompileShader(fragment_shader));
  CHECK_SUCCESS(CompileErrors(fragment_shader, "FRAGMENT", &shader_compiled));
  CHECK_TRUE(shader_compiled, core::Error::ERR_PARSE);

  // Create Shader Program Object and get its reference
  program = glCreateProgram();
  CHECK_SUCCESS(static_cast<core::Error>(glGetError()));

  // Attach the Vertex and Fragment Shaders to the Shader Program
  CHECK_GLGETERROR(glAttachShader(program, vertex_shader));
  CHECK_GLGETERROR(glAttachShader(program, fragment_shader));

  // Wrap-up/Link all the shaders together into the Shader Program
  CHECK_GLGETERROR(glLinkProgram(program));
  CHECK_SUCCESS(CompileErrors(program, "PROGRAM", &program_compiled));
  CHECK_TRUE(program_compiled, core::Error::ERR_PARSE);

bail:
  // Delete the now useless Vertex and Fragment Shader objects
  if (vertex_shader != 0) {
    glDeleteShader(vertex_shader);
  }
  if (fragment_shader != 0) {
    glDeleteShader(fragment_shader);
  }
  if (program_compiled) {
    Delete();  // delete previous program
    m_handle = program;
  }
  return err;
}

Error Shader::Activate() {
  Error err = Error::SUCCESS;
  CHECK_GLGETERROR(glUseProgram(m_handle));
bail:
  return err;
}

Error Shader::Deactivate() {
  Error err = Error::SUCCESS;
  CHECK_GLGETERROR(glUseProgram(0));
bail:
  return err;
}

Error Shader::Delete() {
  Error err = Error::SUCCESS;
  if (m_handle != 0) {
    CHECK_GLGETERROR(glDeleteProgram(m_handle));
    m_handle = 0;
  }
  m_attributes.clear();
  m_uniforms.clear();
bail:
  return err;
}

GLint Shader::Uniform(const std::string& name) {
  const auto it = m_uniforms.find(name);
  if (it == m_uniforms.end()) {
    const GLint loc = glGetUniformLocation(m_handle, name.c_str());
    if (loc < 0) LOG_WARNING("Uniform %s does not exist in program", name.c_str());
    // add it anyways
    m_uniforms[name] = loc;
    return loc;
  }
bail:
  return it->second;
}

GLint Shader::Attribute(const std::string& name) {
  const auto it = m_attributes.find(name);
  if (it == m_attributes.end()) {
    const GLint loc = glGetAttribLocation(m_handle, name.c_str());
    if (loc < 0) LOG_WARNING("Attribute %s does not exist in program", name.c_str());
    // add it anyways
    m_attributes[name] = loc;
    return loc;
  }
bail:
  return it->second;
}

Error Shader::SetFloatAttribute(const std::string& name, const int size, const int stride, const uint32_t offset,
                                const bool normalize) {
  Error err = Error::SUCCESS;
  const GLint loc = Attribute(name);
  CHECK_TRUE(loc >= 0, Error::ERR_GENERAL);
  CHECK_GLGETERROR(glEnableVertexAttribArray(loc));
  CHECK_GLGETERROR(glVertexAttribPointer(loc, size, GL_FLOAT, normalize, stride,
                                         reinterpret_cast<void*>(static_cast<std::ptrdiff_t>(offset))));
bail:
  return err;
}

Error Shader::SetUniform(const std::string& name, const uint32_t val) {
  Error err = Error::SUCCESS;
  CHECK_GLGETERROR(glUniform1ui(Uniform(name), val));
bail:
  return err;
}

Error Shader::SetUniform(const std::string& name, const int val) {
  Error err = Error::SUCCESS;
  CHECK_GLGETERROR(glUniform1i(Uniform(name), val));
bail:
  return err;
}

Error Shader::SetUniform(const std::string& name, const float val) {
  Error err = Error::SUCCESS;
  CHECK_GLGETERROR(glUniform1f(Uniform(name), val));
bail:
  return err;
}

Error Shader::SetUniform(const std::string& name, const float x, const float y) {
  Error err = Error::SUCCESS;
  CHECK_GLGETERROR(glUniform2f(Uniform(name), x, y));
bail:
  return err;
}

Error Shader::SetUniform(const std::string& name, const float x, const float y, const float z) {
  Error err = Error::SUCCESS;
  CHECK_GLGETERROR(glUniform3f(Uniform(name), x, y, z));
bail:
  return err;
}

Error Shader::SetUniform(const std::string& name, const float x, const float y, const float z, const float w) {
  Error err = Error::SUCCESS;
  CHECK_GLGETERROR(glUniform4f(Uniform(name), x, y, z, w));
bail:
  return err;
}

Error Shader::SetUniform(const std::string& name, const glm::vec2* v, const int count) {
  Error err = Error::SUCCESS;
  CHECK_GLGETERROR(glUniform2fv(Uniform(name), count, glm::value_ptr(*v)));
bail:
  return err;
}

Error Shader::SetUniform(const std::string& name, const glm::vec3* v, const int count) {
  Error err = Error::SUCCESS;
  CHECK_GLGETERROR(glUniform3fv(Uniform(name), count, glm::value_ptr(*v)));
bail:
  return err;
}

Error Shader::SetUniform(const std::string& name, const glm::vec4* v, const int count) {
  Error err = Error::SUCCESS;
  CHECK_GLGETERROR(glUniform4fv(Uniform(name), count, glm::value_ptr(*v)));
bail:
  return err;
}

Error Shader::SetUniform(const std::string& name, const glm::mat4* m, const int count) {
  Error err = Error::SUCCESS;
  CHECK_GLGETERROR(glUniformMatrix4fv(Uniform(name), count, GL_FALSE, glm::value_ptr(*m)));
bail:
  return err;
}

Error Shader::SetUniform(const std::string& name, const glm::mat3* m, const int count) {
  Error err = Error::SUCCESS;
  CHECK_GLGETERROR(glUniformMatrix3fv(Uniform(name), count, GL_FALSE, glm::value_ptr(*m)));
bail:
  return err;
}

}  // namespace rendering
}  // namespace core
}  // namespace nv3dvc
