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

#ifndef SRC_MODULES_CAPTUREMODULE_CAPTUREDEVICE_NVWEBCAMERA_H_
#define SRC_MODULES_CAPTUREMODULE_CAPTUREDEVICE_NVWEBCAMERA_H_

#include <cuda.h>
#include <nppi.h>
#include <nvCVImage.h>

#include "CaptureDevice.h"

// Forward declaration
class ICaptureWebcam;
class FrameDesc;

namespace nv3dvc {
namespace modules {
namespace capturemodule {
namespace capturedevice {

/// @brief NVIDIA web camera capture device
class NvWebCamera : public ICaptureDevice {
 public:
  explicit NvWebCamera(CUcontext cu_context);
  ~NvWebCamera() override;

  core::Error Initialize(const CameraDescriptor& camera_descriptor, uint32_t width, uint32_t height) override;
  CaptureApi GetCaptureApi() const override;
  core::Error GetFrame(NvCVImage* frame, cudaStream_t stream) override;
  core::Error GetFramerate(float* framerate) const override;
  core::Error GetResolution(int* width, int* height) const override;
  core::Error GetGainLimits(float* min_gain, float* max_gain) const override;
  core::Error GetGain(float* gain) const override;
  core::Error GetExposureLimits(float* min_exposure, float* max_exposure) const override;
  core::Error GetExposure(float* exposure) const override;
  core::Error GetGammaLimits(float* min_gamma, float* max_gamma) const override;
  core::Error GetGamma(float* gamma) const override;
  core::Error SetGain(float new_gain) override;
  core::Error SetExposure(float new_exposure) override;
  core::Error SetGamma(float new_gamma) override;

 private:
  struct ParameterRange {
    int32_t min;
    int32_t max;
    int32_t step;
    int32_t default_value;
  };
  bool m_initialized;
  CameraDescriptor m_cameraDescriptor;
  ICaptureWebcam* m_webcam;
  ParameterRange m_gainRange;
  ParameterRange m_exposureRange;
  ParameterRange m_gammaRange;
  float m_framerate;
  int m_imageWidth;
  int m_imageHeight;
  CUcontext m_cuContext;
  NppStreamContext m_nppStreamContext;
  NvCVImage m_tmpImage;
};

}  // namespace capturedevice
}  // namespace capturemodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_CAPTUREMODULE_CAPTUREDEVICE_NVWEBCAMERA_H_
