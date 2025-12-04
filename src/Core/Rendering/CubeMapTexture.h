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

#ifndef SRC_CORE_RENDERING_CUBEMAPTEXTURE_H_
#define SRC_CORE_RENDERING_CUBEMAPTEXTURE_H_

#include <string>

#include "Core/Error.h"
#include "Shader.h"

namespace nv3dvc {
namespace core {
namespace rendering {

/// @brief Cube map texture
class CubeMapTexture {
 public:
  CubeMapTexture() = default;
  ~CubeMapTexture();

  /// @brief Load all six sides of the cube map texture
  /// @param[in] image_px The file path to the inner right texture of the cube map
  /// @param[in] image_nx The file path to the inner left texture of the cube map
  /// @param[in] image_py The file path to the inner top texture of the cube map
  /// @param[in] image_ny The file path to the inner bottom texture of the cube map
  /// @param[in] image_pz The file path to the inner front texture of the cube map
  /// @param[in] image_nz The file path to the inner back texture of the cube map
  /// @return    Error::SUCCESS If successful
  Error Load(const std::string& image_px, const std::string& image_nx, const std::string& image_py,
             const std::string& image_ny, const std::string& image_pz, const std::string& image_nz);

  /// @brief Set the texture unit to be used for this texture in the shader
  /// @param[in,out] shader  The shader
  /// @param[in]     uniform The name of the texture uniform in the shader
  /// @param[in]     unit    The texture unit to bind to this texture
  /// @return        Error::SUCCESS If successful
  Error TexUnit(Shader* shader, const char* uniform, GLuint unit);

  /// @brief Bind the texture to this texture unit
  /// @param[in] unit The texture unit to bind to this texture
  /// @return    Error::SUCCESS If successful
  Error Bind(GLuint unit);

  /// @brief Unbind the cube map texture
  /// @return Error::SUCCESS If successful
  Error Unbind();

  /// @brief Delete the cube map texture if it exists
  /// @return Error::SUCCESS If successful
  Error Delete();

 private:
  /// @brief Initialize the cube map texture with raw data
  /// @param[in] width_img       The width of each of the cube map textures
  /// @param[in] height_img      The height of each of the cube map textures
  /// @param[in] num_col_ch      The number of color channels in each of the cube map textures
  /// @param[in] pixel_format    The GL pixel format in each of the cube map textures
  /// @param[in] pixel_data_type The GL pixel data type in each of the cube map textures
  /// @param[in] bytes_right     Raw bytes pointer for the right image
  /// @param[in] bytes_left      Raw bytes pointer for the left image
  /// @param[in] bytes_top       Raw bytes pointer for the top image
  /// @param[in] bytes_bottom    Raw bytes pointer for the bottom image
  /// @param[in] bytes_front     Raw bytes pointer for the front image
  /// @param[in] bytes_back      Raw bytes pointer for the back image
  /// @return Error::SUCCESS If successful
  Error Initialize(int width_img, int height_img, int num_col_ch, GLenum pixel_format, GLenum pixel_data_type,
                   const unsigned char* bytes_right, const unsigned char* bytes_left, const unsigned char* bytes_top,
                   const unsigned char* bytes_bottom, const unsigned char* bytes_front,
                   const unsigned char* bytes_back);

  GLuint m_id = -1;
  GLenum m_pixelDataType = 0;
  int m_width = 0;
  int m_height = 0;
  int m_numChannels = 0;
};

}  // namespace rendering
}  // namespace core
}  // namespace nv3dvc

#endif  // SRC_CORE_RENDERING_CUBEMAPTEXTURE_H_
