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

#ifndef SRC_MODULES_CAPTUREMODULE_CAPTUREDEVICE_NVCVOPENCV_H_
#define SRC_MODULES_CAPTUREMODULE_CAPTUREDEVICE_NVCVOPENCV_H_

#include "nvCVImage.h"
#include "opencv2/opencv.hpp"

/// @brief Set an OpenCV Mat image from parameters
/// @param[in,out] cv_im      The OpenCV image to set
/// @param[in]     width      The width of the image, in pixels
/// @param[in]     height     The height of the image, in pixels
/// @param[in]     num_comps  The number of components per pixel
/// @param[in]     comp_type  The component data type
/// @param[in]     comp_bytes The number of bytes for a component
/// @param[in]     pixels     The raw data pointer of pixels
/// @param[in]     row_bytes  The stride. Number of bytes per row
inline void CVImageSet(cv::Mat* cv_im, int width, int height, int num_comps, int comp_type, int comp_bytes,
                       void* pixels, size_t row_bytes) {
  size_t pix_bytes = num_comps * comp_bytes;
  size_t width_bytes = width * pix_bytes;
  cv_im->flags = cv::Mat::MAGIC_VAL + (CV_MAKETYPE(comp_type, num_comps) & cv::Mat::TYPE_MASK);
  if (row_bytes == width_bytes) cv_im->flags |= cv::Mat::CONTINUOUS_FLAG;
  cv_im->step.p = cv_im->step.buf;
  cv_im->step[0] = row_bytes;
  cv_im->step[1] = pix_bytes;
  cv_im->dims = 2;
  cv_im->size = cv::MatSize(&cv_im->rows);
  cv_im->rows = height;
  cv_im->cols = width;
  cv_im->data = reinterpret_cast<uchar*>(pixels);
  cv_im->datastart = reinterpret_cast<uchar*>(pixels);
  cv_im->datalimit = cv_im->datastart + row_bytes * height;
  cv_im->dataend = cv_im->datalimit - row_bytes + width_bytes;
  cv_im->allocator = 0;
  cv_im->u = 0;
}

/// @brief Wrap an NvCVImage in a cv::Mat
/// @param[in]     nvcv_im The source image
/// @param[in,out] cv_im   The resulting cv::Mat wrapping nvcv_im. Base object but not its data buffer should be
/// allocated
inline void CVWrapperForNvCVImage(const NvCVImage* nvcv_im, cv::Mat* cv_im) {
  static const char cvType[] = {7, 0, 2, 3, 7, 7, 4, 5, 7, 7, 6};
  CVImageSet(cv_im, nvcv_im->width, nvcv_im->height, nvcv_im->numComponents,
             cvType[static_cast<int>(nvcv_im->componentType)], nvcv_im->componentBytes, nvcv_im->pixels,
             nvcv_im->pitch);
}

/// @brief Wrap a cv::Mat in an NvCVImage
/// @param[in]     cv_im   The source image
/// @param[in,out] nvcv_im The resulting NvCVImage wrapping nvcv_im. Base object but not its data buffer should be
/// allocated
inline void NVWrapperForCVMat(const cv::Mat* cv_im, NvCVImage* nvcv_im) {
  static const NvCVImage_PixelFormat nv_format[] = {NVCV_FORMAT_UNKNOWN, NVCV_Y, NVCV_YA, NVCV_BGR, NVCV_BGRA};
  static const NvCVImage_ComponentType nv_type[] = {NVCV_U8,  NVCV_TYPE_UNKNOWN, NVCV_U16, NVCV_S16,
                                                    NVCV_S32, NVCV_F32,          NVCV_F64, NVCV_TYPE_UNKNOWN};
  nvcv_im->pixels = cv_im->data;
  nvcv_im->width = cv_im->cols;
  nvcv_im->height = cv_im->rows;
  nvcv_im->pitch = static_cast<int>(cv_im->step[0]);
  nvcv_im->pixelFormat = nv_format[cv_im->channels() <= 4 ? cv_im->channels() : 0];
  nvcv_im->componentType = nv_type[cv_im->depth() & 7];
  nvcv_im->bufferBytes = 0;
  nvcv_im->deletePtr = nullptr;
  nvcv_im->deleteProc = nullptr;
  nvcv_im->pixelBytes = static_cast<unsigned char>(cv_im->step[1]);
  nvcv_im->componentBytes = static_cast<unsigned char>(cv_im->elemSize1());
  nvcv_im->numComponents = static_cast<unsigned char>(cv_im->channels());
  nvcv_im->planar = NVCV_CHUNKY;
  nvcv_im->gpuMem = NVCV_CPU;
  nvcv_im->reserved[0] = 0;
  nvcv_im->reserved[1] = 0;
}

#endif  // SRC_MODULES_CAPTUREMODULE_CAPTUREDEVICE_NVCVOPENCV_H_
