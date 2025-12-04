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

#include "VideoFrameComponent.h"

#include <algorithm>

#include "Core/Error.h"
#include "Core/Util/Logger.h"

namespace nv3dvc {
namespace modules {
namespace commonmodule {
namespace components {

core::Error VideoFrameComponent::AllocateImageBuffer(const unsigned width, const unsigned height,
                                                     const NvCVImage_PixelFormat format,
                                                     const NvCVImage_ComponentType type, const unsigned is_planar,
                                                     const unsigned on_gpu, const unsigned alignment) {
  core::Error err = core::Error::SUCCESS;
  uint32_t padded_width = width;
  uint32_t padded_height = height;
  if (pad_square) {
    padded_width = glm::max(width, height);
    padded_height = padded_width;
  }
  padded_width += 2 * padding.get()->x;
  padded_height += 2 * padding.get()->y;
  CHECK_NVCV_SUCCESS(
      NvCVImage_Realloc(&m_imagePadded, padded_width, padded_height, format, type, is_planar, on_gpu, alignment));
  // Initialize with 0
  if (on_gpu == NVCV_CUDA) {
    cudaMemset(m_imagePadded.pixels, 0, m_imagePadded.bufferBytes);
  } else if (on_gpu == NVCV_CPU) {
    memset(m_imagePadded.pixels, 0, m_imagePadded.bufferBytes);
  }
  const uint32_t offset_x = (padded_width - width) / 2;
  const uint32_t offset_y = (padded_height - height) / 2;
  NvCVImage_InitView(&m_image, &m_imagePadded, offset_x, offset_y, width, height);
  // Note: m_imageRgba is not initialized here, but in UpdateRgbaImage
bail:
  return err;
}

core::Error VideoFrameComponent::Init(NvCVImage* full_image, const int x_offset, const int y_offset,
                                      const unsigned width, const unsigned height) {
  core::Error err = core::Error::SUCCESS;
  CHECK_FALSE(pad_square, core::Error::ERR_SCENE,
              "Configuration error. The property pad_square can not be true when initializing VideoFrameComponent with "
              "pre-allocated image");
  CHECK_TRUE(padding.get()->x == 0, core::Error::ERR_SCENE,
             "Configuration error. The property padding must be (0,0) when initializing VideoFrameComponent with "
             "pre-allocated image");
  CHECK_TRUE(padding.get()->y == 0, core::Error::ERR_SCENE,
             "Configuration error. The property padding must be (0,0) when initializing VideoFrameComponent with "
             "pre-allocated image");
  NvCVImage_InitView(&m_imagePadded, full_image, 0, 0, full_image->width, full_image->height);
  NvCVImage_InitView(&m_image, &m_imagePadded, x_offset, y_offset, width, height);
  // Note: m_imageRgba is not initialized here, but in UpdateRgbaImage
bail:
  return err;
}

core::Error VideoFrameComponent::UnInit() {
  NvCVImage_Dealloc(&m_imagePadded);  // Dealloc ok, even on views
  NvCVImage_Dealloc(&m_image);        // Dealloc ok, even on views
  NvCVImage_Dealloc(&m_imageRgba);    // Dealloc ok, even on views
  m_imagePadded = {};
  m_image = {};
  m_imageRgba = {};
  return core::Error::SUCCESS;
}

void VideoFrameComponent::SetIsMirrored(bool is_mirrored) { m_isMirrored = is_mirrored; }

void VideoFrameComponent::SetShouldUpdateRgbaImage(bool should_update_rgba_image) {
  m_shouldUpdateRgbaImage.store(should_update_rgba_image);
}

void VideoFrameComponent::SetShouldShowVideo(bool should_show_video) { show_video.get()->store(should_show_video); }

bool VideoFrameComponent::IsMirrored() const { return m_isMirrored; }

bool VideoFrameComponent::ShouldUpdateRgbaImage() const { return m_shouldUpdateRgbaImage.load(); }

bool VideoFrameComponent::ShouldShowVideo() const { return show_video.get()->load(); }

uint32_t VideoFrameComponent::GetHorizontalPadding() const { return (m_imagePadded.width - m_image.width) / 2; }

uint32_t VideoFrameComponent::GetVerticalPadding() const { return (m_imagePadded.height - m_image.height) / 2; }

NvCVImage* VideoFrameComponent::GetPaddedImagePtr() { return &m_imagePadded; }

NvCVImage* VideoFrameComponent::GetImagePtr() { return &m_image; }

NvCVImage* VideoFrameComponent::GetRgbaImagePtr() { return &m_imageRgba; }

core::Error VideoFrameComponent::UpdateRgbaImage(cudaStream_t stream, NvCVImage* tmp) {
  core::Error err = core::Error::SUCCESS;
  if (!m_shouldUpdateRgbaImage.load()) {
    BAIL(err, core::Error::SUCCESS);
  }
  if (m_imageRgba.width != m_image.width || m_imageRgba.height != m_image.height) {
    CHECK_NVCV_SUCCESS(NvCVImage_Realloc(&m_imageRgba, m_image.width, m_image.height, NVCV_RGBA, m_image.componentType,
                                         m_image.planar, m_image.gpuMem, 1));
  }
  CHECK_NVCV_SUCCESS(NvCVImage_Transfer(&m_image, &m_imageRgba, 1.0f, stream, tmp));
bail:
  return err;
}

}  // namespace components
}  // namespace commonmodule
}  // namespace modules
}  // namespace nv3dvc
