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

#include "OpenCvCamera.h"

#include <memory>

#include "Core/Error.h"
#include "nvCVImage.h"
#include "nvCVOpenCV.h"
#include "opencv2/opencv.hpp"

namespace nv3dvc {
namespace modules {
namespace capturemodule {
namespace capturedevice {

OpenCVCamera::OpenCVCamera()
    : m_initialized{false},
      m_cameraDescriptor{},
      m_imageWidth{0},
      m_imageHeight{0},
      m_cap{nullptr},
      m_latestFrameBgrOcv{nullptr} {}

OpenCVCamera::~OpenCVCamera() {
  m_initialized = false;
  OpenCVCamera::StopCapture();
}

core::Error OpenCVCamera::Initialize(const CameraDescriptor& camera_descriptor, const uint32_t width,
                                     const uint32_t height) {
  m_cameraDescriptor = camera_descriptor;
  m_cap = std::make_unique<cv::VideoCapture>();
  m_latestFrameBgrOcv = std::make_unique<cv::Mat>();
  core::Error err = core::Error::SUCCESS;

  CHECK_TRUE(m_cap->open(camera_descriptor.camera_device_index, cv::CAP_DSHOW), core::Error::ERR_CAMERA,
             "Unable to open CV Capture device %d", camera_descriptor.camera_device_index);

  // set resolution
  CHECK_TRUE(m_cap->set(cv::CAP_PROP_FRAME_WIDTH, width), core::Error::ERR_CAMERA, "Unable to set camera width : %d",
             width);
  CHECK_TRUE(m_cap->set(cv::CAP_PROP_FRAME_HEIGHT, height), core::Error::ERR_CAMERA, "Unable to set camera height : %d",
             height);

  // get received resolution
  m_imageWidth = static_cast<int>(m_cap->get(cv::CAP_PROP_FRAME_WIDTH));
  m_imageHeight = static_cast<int>(m_cap->get(cv::CAP_PROP_FRAME_HEIGHT));
  LOG_INFO("Camera resolution: %d x %d", m_imageWidth, m_imageHeight);

  CHECK_NVCV_SUCCESS(NvCVImage_Alloc(&m_latestFrameBgrPinned, m_imageWidth, m_imageHeight, NVCV_BGR, NVCV_U8,
                                     NVCV_CHUNKY, NVCV_CPU_PINNED, 0),
                     "Unable to allocate image of size %d x %d", m_imageWidth, m_imageHeight);

  if (!m_cap->set(cv::CAP_PROP_EXPOSURE, camera_descriptor.exposure)) {
    LOG_WARNING("Unable to set camera exposure : %f", camera_descriptor.exposure);
  }
  if (!m_cap->set(cv::CAP_PROP_AUTO_EXPOSURE, camera_descriptor.auto_exposure)) {
    LOG_WARNING("Unable to set reset auto exposure");
  }
  if (!m_cap->set(cv::CAP_PROP_GAIN, camera_descriptor.gain)) {
    LOG_WARNING("Unable to set camera gain : %f", camera_descriptor.gain);
  }
  if (!m_cap->set(cv::CAP_PROP_FPS, camera_descriptor.fps)) {
    LOG_WARNING("Unable to set camera framerate : %f", camera_descriptor.fps);
  }

  m_initialized = true;
bail:
  return err;
}

CaptureApi OpenCVCamera::GetCaptureApi() const { return CaptureApi::OPENCV_WEBCAM; }

core::Error OpenCVCamera::GetFrame(NvCVImage* frame, const cudaStream_t stream) {
  bool ok = true;
  core::Error err = core::Error::SUCCESS;

  if (!m_initialized) return core::Error::ERR_INITIALIZATION;

  // get frame
  CVWrapperForNvCVImage(&m_latestFrameBgrPinned, m_latestFrameBgrOcv.get());
  ok = m_cap->read(*m_latestFrameBgrOcv);
  if (m_latestFrameBgrOcv->empty()) {
    // loop
    CHECK_TRUE(m_cap->set(cv::CAP_PROP_POS_FRAMES, 0), core::Error::ERR_CAMERA_API, "Unable to set frame position");
    m_cap->read(*m_latestFrameBgrOcv);

    if (m_latestFrameBgrOcv->empty()) return core::Error::ERR_CAMERA;
  }

  if (m_cameraDescriptor.flip_horizontal) {
    cv::flip(*m_latestFrameBgrOcv, *m_latestFrameBgrOcv, 1);
  }

  CHECK_NVCV_SUCCESS(NvCVImage_Init(frame, m_latestFrameBgrPinned.width, m_latestFrameBgrPinned.height,
                                    m_latestFrameBgrPinned.pitch, m_latestFrameBgrPinned.pixels,
                                    m_latestFrameBgrPinned.pixelFormat, m_latestFrameBgrPinned.componentType,
                                    m_latestFrameBgrPinned.planar, m_latestFrameBgrPinned.gpuMem),
                     "Unable to initialize video frame");
bail:
  if (!ok) {
    LOG_WARNING("Unable to read frame from OpenCV webcam");
    return core::Error::ERR_DATA_UNAVAILABLE;
  }
  return err;
}

core::Error OpenCVCamera::GetFramerate(float* framerate) const {
  if (!framerate) return core::Error::ERR_NULL_POINTER;
  if (!m_cap) return core::Error::ERR_CAMERA;
  double val = m_cap->get(cv::CAP_PROP_FPS);
  if (val == 0.0f) {
    return core::Error::ERR_CAMERA_API;
  }
  *framerate = static_cast<float>(val);
  return core::Error::SUCCESS;
}

core::Error OpenCVCamera::GetResolution(int* width, int* height) const {
  if (!width) return core::Error::ERR_NULL_POINTER;
  if (!height) return core::Error::ERR_NULL_POINTER;
  if (!m_cap) return core::Error::ERR_CAMERA;
  *width = m_imageWidth;
  *height = m_imageHeight;
  return core::Error::SUCCESS;
}

core::Error OpenCVCamera::GetGain(float* gain) const {
  if (!gain) return core::Error::ERR_NULL_POINTER;
  if (!m_cap) return core::Error::ERR_CAMERA;
  double val = m_cap->get(cv::CAP_PROP_GAIN);
  if (val == 0.0f) {
    return core::Error::ERR_CAMERA_API;
  }
  *gain = static_cast<float>(val);
  return core::Error::SUCCESS;
}

core::Error OpenCVCamera::GetExposure(float* exposure) const {
  if (!exposure) return core::Error::ERR_NULL_POINTER;
  if (!m_cap) return core::Error::ERR_CAMERA;
  double val = m_cap->get(cv::CAP_PROP_EXPOSURE);
  if (val == 0.0f) {
    return core::Error::ERR_CAMERA_API;
  }
  *exposure = static_cast<float>(val);
  return core::Error::SUCCESS;
}

core::Error OpenCVCamera::GetGamma(float* gamma) const {
  if (!gamma) return core::Error::ERR_NULL_POINTER;
  if (!m_cap) return core::Error::ERR_CAMERA;
  double val = m_cap->get(cv::CAP_PROP_GAMMA);
  if (val == 0.0f) {
    return core::Error::ERR_CAMERA_API;
  }
  *gamma = static_cast<float>(val);
  return core::Error::SUCCESS;
}

core::Error OpenCVCamera::SetGain(const float new_gain) {
  if (!m_cap) return core::Error::ERR_CAMERA;
  const double value = new_gain;
  if (m_cap->set(cv::CAP_PROP_GAMMA, value)) {
    return core::Error::SUCCESS;
  } else {
    return core::Error::ERR_CAMERA_API;
  }
}

core::Error OpenCVCamera::SetExposure(const float new_exposure) {
  if (!m_cap) return core::Error::ERR_CAMERA;
  const double value = new_exposure;
  if (m_cap->set(cv::CAP_PROP_EXPOSURE, value)) {
    return core::Error::SUCCESS;
  } else {
    return core::Error::ERR_CAMERA_API;
  }
}

core::Error OpenCVCamera::SetGamma(const float new_gamma) {
  if (!m_cap) return core::Error::ERR_CAMERA;
  const double value = new_gamma;
  if (m_cap->set(cv::CAP_PROP_GAMMA, value)) {
    return core::Error::SUCCESS;
  } else {
    return core::Error::ERR_CAMERA_API;
  }
}

core::Error OpenCVCamera::StopCapture() {
  if (!m_cap) return core::Error::ERR_CAMERA;
  if (m_cap->isOpened()) m_cap->release();
  return core::Error::SUCCESS;
}

}  // namespace capturedevice
}  // namespace capturemodule
}  // namespace modules
}  // namespace nv3dvc
