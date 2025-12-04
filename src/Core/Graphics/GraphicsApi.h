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

#ifndef SRC_CORE_GRAPHICS_GRAPHICSAPI_H_

#define SRC_CORE_GRAPHICS_GRAPHICSAPI_H_

#include "Core/Error.h"

namespace nv3dvc {
namespace core {
namespace graphics {
class Context {};

/// @brief Interface to graphics context
///
/// Currently only supports OpenGL
class GraphicsApi {
 public:
  GraphicsApi();
  ~GraphicsApi();

  /// @brief Initialize OpenGL
  /// @return Error::SUCCESS if successful
  Error Initialize();
  Error Uninitialize();

  void* GetContextPtr();

 private:
  Context* m_context;
};
}  // namespace graphics
}  // namespace core
}  // namespace nv3dvc

#endif  // SRC_CORE_GRAPHICS_GRAPHICSAPI_H_
