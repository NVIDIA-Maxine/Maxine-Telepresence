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

#ifndef SRC_MODULES_GUIMODULE_PANELS_VIDEOPANEL_H_
#define SRC_MODULES_GUIMODULE_PANELS_VIDEOPANEL_H_

#include <unordered_map>

// Include GL first
#include "glad/gl.h"
// Then CUDA GL
#include "Core/EntityComponentSystem/Entity.h"
#include "Core/Error.h"
#include "cuda.h"
#include "cudaGL.h"
#include "cudaGLTypedefs.h"
#include "cuda_gl_interop.h"
#include "cuda_runtime.h"
#include "cuda_runtime_api.h"
#include "nvCVImage.h"

namespace nv3dvc {
namespace modules {
namespace guimodule {
namespace panels {

/// @brief Panel for video
/// Will render VideoFrameComponent if ShouldShowVideo is true. One panel will be rendered per VideoFrameComponent
class VideoPanel {
 public:
  VideoPanel() = default;

  /// @brief Initialize all mapped images, one per VideoFrameComponent
  /// @param[in,out] reg    The entity registry
  /// @param[in]     stream CUDA stream for mapping
  /// @return        core::Error::SUCCESS            If successful
  ///                core::Error::ERR_INITIALIZATION If any of the images have not been initialized properly
  core::Error Initialize(core::ecs::registry::EntityRegistry* reg, cudaStream_t stream);

  /// @brief Uninitialize all mapped images, one per VideoFrameComponent
  /// @param[in,out] reg    The entity registry
  /// @param[in]     stream CUDA stream for mapping
  /// @return        core::Error::SUCCESS If successful
  core::Error UnInitialize(core::ecs::registry::EntityRegistry* reg, cudaStream_t stream);

  /// @brief Display all video panels
  /// @param[in,out] reg    The entity registry
  /// @return        core::Error::SUCCESS If successful
  core::Error Render(core::ecs::registry::EntityRegistry* reg, cudaStream_t stream);

  /// @brief Inform all the VideoFrameComponents that RGBA images do not need to be updated as video should not be shown
  /// @param[in,out] reg    The entity registry
  void ConfigDoNotUpdate(core::ecs::registry::EntityRegistry* reg);

 private:
  /// @brief Struct for mapping a CUDA image to GL memory
  struct MappedImage {
    GLuint tex_id = 0;
    cudaGraphicsResource* cuda_graphics_resource = nullptr;
    cudaArray* cuda_array = nullptr;
    const NvCVImage* img_src = nullptr;
    bool initialized = false;

    /// @brief Generate GL texture and initialize the mapped image
    /// @param img    The image to map. Needs to be in RGBA format, and U8 data type
    /// @param stream The cuda stream to use for mapping resource
    /// @return       core::Error::SUCCESS if successful
    ///               core::Error::ERR_INITIALIZATION if any of the images have not been initialized properly
    core::Error Initialize(const NvCVImage* img, cudaStream_t stream);

    /// @brief Delete the GL texture and unmap resource
    /// @param stream CUDA stream for unmapping
    /// @return       core::Error::SUCCESS if successful
    core::Error UnInitialize(cudaStream_t stream);
  };
  /// Mapping from entity to mapped image
  std::unordered_map<core::ecs::Entity, MappedImage, core::ecs::EntityHash> m_mappedImages;
};

}  // namespace panels
}  // namespace guimodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_GUIMODULE_PANELS_VIDEOPANEL_H_
