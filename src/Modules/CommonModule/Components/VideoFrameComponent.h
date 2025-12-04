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

#ifndef SRC_MODULES_COMMONMODULE_COMPONENTS_VIDEOFRAMECOMPONENT_H_
#define SRC_MODULES_COMMONMODULE_COMPONENTS_VIDEOFRAMECOMPONENT_H_

#include <string>

#include "Core/EntityComponentSystem/Component.h"
#include "Core/Error.h"
#include "Core/Util/Timeable.h"
#include "cuda_runtime.h"
#include "glm/glm.hpp"
#include "nvCVImage.h"

namespace nv3dvc {
namespace modules {
namespace commonmodule {
namespace components {

/// @defgroup VideoFrameComponentProperties VideoFrameComponent
/// @ingroup  ComponentProperties
/// @brief    Component containing data for storing a video frame in GPU memory

/// See @ref VideoFrameComponentProperties
class VideoFrameComponent : public core::ecs::Component, public core::util::Timeable {
 public:
  constexpr static const char* NAME = "VideoFrameComponent";
  std::string Name() const override { return "VideoFrameComponent"; };

  VideoFrameComponent() = default;

  /// @brief Reallocate memory for internal image buffer
  ///
  /// See NvCVImage_Realloc
  /// @param[in] width     The desired width  of the image, in pixels.
  /// @param[in] height    The desired height of the image, in pixels.
  /// @param[in] format    The format of the pixels.
  /// @param[in] type      The type of the components of the pixels.
  /// @param[in] is_planar One of { NVCV_CHUNKY, NVCV_PLANAR } or one of the YUV layouts
  /// @param[in] on_gpu    Location of the buffer: one of { NVCV_CPU, NVCV_CPU_PINNED, NVCV_GPU, NVCV_CUDA }
  /// @param[in] alignment Alignment row byte alignment. Choose 0 or a power of 2.
  ///                      1: yields no gap whatsoever between scanlines;
  ///                      0: default alignment: 4 on CPU, and cudaMallocPitch's choice on GPU.
  ///                      Other common values are 16 or 32 for cache line size.
  /// @return core::Error::SUCCESS If the operation was successful
  /// @return NVCV_ERR_PIXELFORMAT If the pixel format is not accommodated
  /// @return NVCV_ERR_MEMORY      If there is not enough memory to allocate the buffer
  core::Error AllocateImageBuffer(unsigned width, unsigned height, NvCVImage_PixelFormat format,
                                  NvCVImage_ComponentType type, unsigned is_planar, unsigned on_gpu,
                                  unsigned alignment);

  /// @brief Initialize using a preallocated image
  ///
  /// Does not take ownership of the input image. Expects the lifetime to exceed the VideoFrameComponent or the next
  /// call to Init
  /// @param[in] full_image The image used to initialize the padded image of the video frame component
  /// @param[in] x_offset   The horizontal offset to the view of the image. 0 if not padded [pixels]
  /// @param[in] y_offset   The vertical offset to the view of the image. 0 if not padded [pixels]
  /// @param[in] width      The width of the view of the image. full_img->width if not padded [pixels]
  /// @param[in] height     The height of the view of the image. full_img->width if not padded [pixels]
  /// @return core::Error::SUCCESS If the operation was successful
  core::Error Init(NvCVImage* full_image, int x_offset, int y_offset, unsigned width, unsigned height);

  /// @brief Uninitialize the video frame
  ///
  /// Deallocates the video frame data if the buffer is owned by the VideoFrameComponent. If the buffer is not owned by
  /// by the VideoFrameComponent, it will be released. After uninitialization, either a new allocation or a new view
  /// init needs to be done before the video frame component can be used.
  /// @return core::Error::SUCCESS If successful
  core::Error UnInit();

  /// @brief Hint to users of the video frame component that its image is mirrored
  /// @param is_mirrored Whether to specify that the image is mirrored, false if not mirrored
  void SetIsMirrored(bool is_mirrored);

  /// @brief Configures whether an image transfer should take place during UpdateRgbaImage
  /// @param should_update_rgba_image Whether to configure the rgba image to be updated
  void SetShouldUpdateRgbaImage(bool should_update_rgba_image);

  /// @brief Hint to GUI System to show video
  /// @param should_show_video Whether video should be shown
  void SetShouldShowVideo(bool should_show_video);

  /// @brief Whether the image has been set to be mirrored using SetIsMirrored
  /// @return Whether the image is mirrored
  bool IsMirrored() const;

  /// @brief Whether the rgba image has been configured to be updated during UpdateRgbaImage
  /// @return Whether the rgba image has been condfigured to be updated
  bool ShouldUpdateRgbaImage() const;

  /// @brief Hint to GUI System to show video
  /// @return Whether video should be shown
  bool ShouldShowVideo() const;

  /// @brief Get the horizontal padding in units of pixels
  ///
  /// The padding refers to one side. The total horizontal padding will be twice this value as the padding is
  /// symmetrical.
  /// @return The horizontal padding
  uint32_t GetHorizontalPadding() const;

  /// @brief Get the vertical padding in units of pixels
  ///
  /// The padding refers to one side. The total vertical padding will be twice this value as the padding is symmetrical.
  /// @return The vertical padding
  uint32_t GetVerticalPadding() const;

  /// @brief Get a pointer to the padded image
  /// @return A pointer to the padded image
  NvCVImage* GetPaddedImagePtr();

  /// @brief Get a pointer to the unpadded image
  ///
  /// This is a view into the padded image
  /// @return A pointer to the unpadded image
  NvCVImage* GetImagePtr();

  /// @brief Get a pointer to the RGBA image
  ///
  /// This is an additionally allocated image, if it exists
  /// @return A pointer to the RGBA image
  NvCVImage* GetRgbaImagePtr();

  /// @brief Update the rgba image corresponding to the image
  /// @param stream The CUDA stream on which image transfer should take place
  /// @param tmp    A temporary image for transferring
  /// @return       core::Error::SUCCESS if successful
  core::Error UpdateRgbaImage(cudaStream_t stream, NvCVImage* tmp);

 public:
  /// @ingroup VideoFrameComponentProperties
  /// @{
  core::properties::Property<bool> pad_square = {
      this,
      "pad_square",
      "Whether the image should be padded to a square",
      false,
  };
  core::properties::Property<glm::uvec2> padding = {
      this,
      "padding",
      "Additional image padding added symmetrically both sides [pixels]",
      glm::uvec2(0, 0),
  };
  core::properties::Property<std::atomic<bool>> show_video = {
      this,
      "show_video",
      "Whether the video frame should be shown in the GUI",
      false,
  };
  /// @}

 private:
  bool m_isMirrored = false;
  std::atomic<bool> m_shouldUpdateRgbaImage = false;
  NvCVImage m_imagePadded = {};
  NvCVImage m_image = {};      // View into m_imagePadded
  NvCVImage m_imageRgba = {};  // For GL display
};

}  // namespace components
}  // namespace commonmodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_COMMONMODULE_COMPONENTS_VIDEOFRAMECOMPONENT_H_
