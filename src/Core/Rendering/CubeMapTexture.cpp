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

#include "CubeMapTexture.h"

#include <string>

#include "Core/Util/Logger.h"
#include "opencv2/opencv.hpp"

namespace nv3dvc {
namespace core {
namespace rendering {

CubeMapTexture::~CubeMapTexture() { Delete(); }

Error CubeMapTexture::Load(const std::string& image_px, const std::string& image_nx, const std::string& image_py,
                           const std::string& image_ny, const std::string& image_pz, const std::string& image_nz) {
  Error err = Error::SUCCESS;
  GLenum pixel_format = GL_RGB;
  int width;
  int height;
  int num_channels;
  int cv_type;

  const cv::Mat im_px = cv::imread(image_px);  // Right cube map image (positive x)
  const cv::Mat im_nx = cv::imread(image_nx);  // Left cube map image (negative x)
  const cv::Mat im_py = cv::imread(image_py);  // Top cube map image (positive y)
  const cv::Mat im_ny = cv::imread(image_ny);  // Bottom cube map image (negative y)
  const cv::Mat im_pz = cv::imread(image_pz);  // Front cube map image (positive z)
  const cv::Mat im_nz = cv::imread(image_nz);  // Back cube map image (negative z)

  CHECK_NONNULL(im_px.data, Error::ERR_FILE, "No data for image %s", image_px.c_str());
  CHECK_NONNULL(im_nx.data, Error::ERR_FILE, "No data for image %s", image_nx.c_str());
  CHECK_NONNULL(im_py.data, Error::ERR_FILE, "No data for image %s", image_py.c_str());
  CHECK_NONNULL(im_ny.data, Error::ERR_FILE, "No data for image %s", image_ny.c_str());
  CHECK_NONNULL(im_pz.data, Error::ERR_FILE, "No data for image %s", image_pz.c_str());
  CHECK_NONNULL(im_nz.data, Error::ERR_FILE, "No data for image %s", image_nz.c_str());

  width = im_px.cols;
  height = im_px.rows;
  num_channels = im_px.channels();
  cv_type = im_px.type();

  CHECK_TRUE(cv_type == CV_8UC1 || cv_type == CV_8UC3 || cv_type == CV_8UC4, Error::ERR_GENERAL,
             "Unsupported cube map data type.");

  CHECK_TRUE(width == im_nx.cols, Error::ERR_GENERAL, "Incompatible cube map resolutions.");
  CHECK_TRUE(width == im_py.cols, Error::ERR_GENERAL, "Incompatible cube map resolutions.");
  CHECK_TRUE(width == im_ny.cols, Error::ERR_GENERAL, "Incompatible cube map resolutions.");
  CHECK_TRUE(width == im_pz.cols, Error::ERR_GENERAL, "Incompatible cube map resolutions.");
  CHECK_TRUE(width == im_nz.cols, Error::ERR_GENERAL, "Incompatible cube map resolutions.");
  CHECK_TRUE(height == im_nx.rows, Error::ERR_GENERAL, "Incompatible cube map resolutions.");
  CHECK_TRUE(height == im_py.rows, Error::ERR_GENERAL, "Incompatible cube map resolutions.");
  CHECK_TRUE(height == im_ny.rows, Error::ERR_GENERAL, "Incompatible cube map resolutions.");
  CHECK_TRUE(height == im_pz.rows, Error::ERR_GENERAL, "Incompatible cube map resolutions.");
  CHECK_TRUE(height == im_nz.rows, Error::ERR_GENERAL, "Incompatible cube map resolutions.");
  CHECK_TRUE(num_channels == im_nx.channels(), Error::ERR_GENERAL, "Incompatible cube map data formats.");
  CHECK_TRUE(num_channels == im_py.channels(), Error::ERR_GENERAL, "Incompatible cube map data formats.");
  CHECK_TRUE(num_channels == im_ny.channels(), Error::ERR_GENERAL, "Incompatible cube map data formats.");
  CHECK_TRUE(num_channels == im_pz.channels(), Error::ERR_GENERAL, "Incompatible cube map data formats.");
  CHECK_TRUE(num_channels == im_nz.channels(), Error::ERR_GENERAL, "Incompatible cube map data formats.");
  CHECK_TRUE(cv_type == im_nx.type(), Error::ERR_GENERAL, "Incompatible cube map data types.");
  CHECK_TRUE(cv_type == im_py.type(), Error::ERR_GENERAL, "Incompatible cube map data types.");
  CHECK_TRUE(cv_type == im_ny.type(), Error::ERR_GENERAL, "Incompatible cube map data types.");
  CHECK_TRUE(cv_type == im_pz.type(), Error::ERR_GENERAL, "Incompatible cube map data types.");
  CHECK_TRUE(cv_type == im_nz.type(), Error::ERR_GENERAL, "Incompatible cube map data types.");

  if (num_channels == 1) {
    pixel_format = GL_RED;
  } else if (num_channels == 3) {
    pixel_format = GL_RGB;
    cv::cvtColor(im_px, im_px, cv::COLOR_BGR2RGB);
    cv::cvtColor(im_nx, im_nx, cv::COLOR_BGR2RGB);
    cv::cvtColor(im_py, im_py, cv::COLOR_BGR2RGB);
    cv::cvtColor(im_ny, im_ny, cv::COLOR_BGR2RGB);
    cv::cvtColor(im_pz, im_pz, cv::COLOR_BGR2RGB);
    cv::cvtColor(im_nz, im_nz, cv::COLOR_BGR2RGB);
  } else if (num_channels == 4) {
    pixel_format = GL_RGBA;
    cv::cvtColor(im_px, im_px, cv::COLOR_BGRA2RGBA);
    cv::cvtColor(im_nx, im_nx, cv::COLOR_BGRA2RGBA);
    cv::cvtColor(im_py, im_py, cv::COLOR_BGRA2RGBA);
    cv::cvtColor(im_ny, im_ny, cv::COLOR_BGRA2RGBA);
    cv::cvtColor(im_pz, im_pz, cv::COLOR_BGRA2RGBA);
    cv::cvtColor(im_nz, im_nz, cv::COLOR_BGRA2RGBA);
  } else {
    LOG_ERROR("Unable to convert pixel format for cube map");
    return Error::ERR_GENERAL;
  }
  CHECK_SUCCESS(Initialize(width, height, num_channels, pixel_format, GL_UNSIGNED_BYTE, im_px.data, im_nx.data,
                           im_py.data, im_ny.data, im_pz.data, im_nz.data));
bail:
  return err;
}

Error CubeMapTexture::Initialize(const int width, const int height, const int num_channels, const GLenum pixel_format,
                                 const GLenum pixelDataType, const unsigned char* bytes_right,
                                 const unsigned char* bytes_left, const unsigned char* bytes_top,
                                 const unsigned char* bytes_bottom, const unsigned char* bytes_front,
                                 const unsigned char* bytes_back) {
  Error err = Error::SUCCESS;
  const unsigned char* data_pointers[6];
  GLenum internal_format = 0;
  m_pixelDataType = pixelDataType;

  m_width = width;
  m_height = height;
  m_numChannels = num_channels;

  CHECK_GLGETERROR(glGenTextures(1, &m_id));
  CHECK_GLGETERROR(glBindTexture(GL_TEXTURE_CUBE_MAP, m_id));

  CHECK_GLGETERROR(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
  CHECK_GLGETERROR(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
  CHECK_GLGETERROR(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
  CHECK_GLGETERROR(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
  CHECK_GLGETERROR(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));

  if (m_pixelDataType == GL_UNSIGNED_BYTE) {
    if (pixel_format == GL_RED) {
      internal_format = GL_R8;
    } else if (pixel_format == GL_RGB) {
      internal_format = GL_RGB8;
    } else if (pixel_format == GL_RGBA) {
      internal_format = GL_RGBA8;
    }
  } else if (m_pixelDataType == GL_FLOAT) {
    if (pixel_format == GL_RED) {
      internal_format = GL_R32F;
    } else if (pixel_format == GL_RGB) {
      internal_format = GL_RGB32F;
    } else if (pixel_format == GL_RGBA) {
      internal_format = GL_RGBA32F;
    }
  }
  if (internal_format == 0) {
    LOG_ERROR("Automatic Texture type recognition failed");
    return Error::ERR_GENERAL;
  }

  data_pointers[0] = bytes_right;
  data_pointers[1] = bytes_left;
  data_pointers[2] = bytes_top;
  data_pointers[3] = bytes_bottom;
  data_pointers[4] = bytes_front;
  data_pointers[5] = bytes_back;
  for (unsigned int i = 0; i < 6; i++) {
    CHECK_GLGETERROR(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internal_format, m_width, m_height, 0,
                                  pixel_format, m_pixelDataType, data_pointers[i]));
  }
  CHECK_GLGETERROR(glBindTexture(GL_TEXTURE_CUBE_MAP, 0));
bail:
  return err;
}

Error CubeMapTexture::Bind(GLuint unit) {
  Error err = Error::SUCCESS;
  CHECK_GLGETERROR(glActiveTexture(GL_TEXTURE0 + unit));
  CHECK_GLGETERROR(glBindTexture(GL_TEXTURE_CUBE_MAP, m_id));
bail:
  return err;
}

Error CubeMapTexture::Unbind() {
  Error err = Error::SUCCESS;
  CHECK_GLGETERROR(glBindTexture(GL_TEXTURE_CUBE_MAP, 0));
bail:
  return err;
}

Error CubeMapTexture::TexUnit(Shader* shader, const char* uniform, GLuint unit) {
  Error err = Error::SUCCESS;
  CHECK_SUCCESS(shader->Activate());
  CHECK_SUCCESS(shader->SetUniform(uniform, GLint(unit)));
bail:
  return err;
}

Error CubeMapTexture::Delete() {
  Error err = Error::SUCCESS;
  if (m_id) {
    CHECK_GLGETERROR(glDeleteTextures(1, &m_id));
  }
bail:
  return err;
}

}  // namespace rendering
}  // namespace core
}  // namespace nv3dvc
