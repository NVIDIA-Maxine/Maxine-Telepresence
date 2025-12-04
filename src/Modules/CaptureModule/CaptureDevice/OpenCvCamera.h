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

#ifndef SRC_MODULES_CAPTUREMODULE_CAPTUREDEVICE_OPENCVCAMERA_H_
#define SRC_MODULES_CAPTUREMODULE_CAPTUREDEVICE_OPENCVCAMERA_H_

#include <driver_types.h>

#include <memory>

#include "CaptureDevice.h"
#include "nvCVImage.h"

// Forward declaration
namespace cv {
class Mat;
class VideoCapture;
}  // namespace cv

namespace nv3dvc {
namespace modules {
namespace capturemodule {
namespace capturedevice {

/// @brief OpenCV camera capture device
class OpenCVCamera : public ICaptureDevice {
 public:
  OpenCVCamera();
  ~OpenCVCamera() override;

  core::Error Initialize(const CameraDescriptor& camera_descriptor, uint32_t width, uint32_t height) override;

  CaptureApi GetCaptureApi() const override;
  core::Error GetFrame(NvCVImage* frame, cudaStream_t stream) override;
  core::Error GetFramerate(float* framerate) const override;
  core::Error GetResolution(int* width, int* height) const override;
  core::Error GetGain(float* gain) const override;
  core::Error GetExposure(float* exposure) const override;
  core::Error GetGamma(float* gamma) const override;
  core::Error SetGain(float new_gain) override;
  core::Error SetExposure(float new_exposure) override;
  core::Error SetGamma(float new_gamma) override;
  core::Error StopCapture() override;

 private:
  bool m_initialized;
  CameraDescriptor m_cameraDescriptor;
  int m_imageWidth;
  int m_imageHeight;
  std::unique_ptr<cv::VideoCapture> m_cap;
  std::unique_ptr<cv::Mat> m_latestFrameBgrOcv;
  NvCVImage m_latestFrameBgrPinned;
};

}  // namespace capturedevice
}  // namespace capturemodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_CAPTUREMODULE_CAPTUREDEVICE_OPENCVCAMERA_H_
