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

#include "Core/Graphics/GraphicsApi.h"

#include "Core/Error.h"
#include "glad/gl.h"
#include "glfw/glfw3.h"

namespace nv3dvc {
namespace core {
namespace graphics {

GraphicsApi::GraphicsApi() : m_context(nullptr) {}

GraphicsApi::~GraphicsApi() {}

Error GraphicsApi::Initialize() {
  int res = gladLoadGL((GLADloadfunc)glfwGetProcAddress);
  if (res == 0) {
    return Error::ERR_GL_CONTEXT_LOST;
  }
  return Error::SUCCESS;
}

Error GraphicsApi::Uninitialize() {
  // OpenGL does not get uninitialized
  return Error::SUCCESS;
}

void* GraphicsApi::GetContextPtr() {
  // OpenGL uses global context
  return nullptr;
}

}  // namespace graphics
}  // namespace core
}  // namespace nv3dvc
