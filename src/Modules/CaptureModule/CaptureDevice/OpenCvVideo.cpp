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

#include "OpenCvVideo.h"

#include <memory>

#include "Core/Error.h"
#include "Core/Util/Logger.h"
#include "cuda_runtime_api.h"
#include "nvCVImage.h"
#include "nvCVOpenCV.h"
#include "opencv2/opencv.hpp"

namespace nv3dvc {
namespace modules {
namespace capturemodule {
namespace capturedevice {

OpenCVVideo::OpenCVVideo()
    : m_loopVideo{true},
      m_limitFrameRate(true),
      m_initialized{false},
      m_cameraDescriptor{},
      m_imageWidth{0},
      m_imageHeight{0},
      m_frameIntervalMs{0},
      m_cap{nullptr},
      m_latestFrameBgr{nullptr},
      m_timeStart({}),
      m_dt({}) {}

OpenCVVideo::~OpenCVVideo() {
  LOG_DEBUG("destroying OpenCVVideo\n");
  m_initialized = false;
  OpenCVVideo::StopCapture();
}

core::Error OpenCVVideo::Initialize(const CameraDescriptor& camera_descriptor, const uint32_t width,
                                    const uint32_t height) {
  core::Error err = core::Error::SUCCESS;

  m_cap = std::make_unique<cv::VideoCapture>();
  m_latestFrameBgr = std::make_unique<cv::Mat>();
  m_cameraDescriptor = camera_descriptor;

  CHECK_TRUE(m_cap->open(m_cameraDescriptor.camera_device_path), core::Error::ERR_CAMERA,
             "Unable to open CV Capture device %s", m_cameraDescriptor.camera_device_path.c_str());

  m_imageWidth = static_cast<int>(m_cap->get(cv::CAP_PROP_FRAME_WIDTH));
  m_imageHeight = static_cast<int>(m_cap->get(cv::CAP_PROP_FRAME_HEIGHT));

  CHECK_NVCV_SUCCESS(NvCVImage_Alloc(&m_latestFrameBgrPinned, m_imageWidth, m_imageHeight, NVCV_BGR, NVCV_U8,
                                     NVCV_CHUNKY, NVCV_CPU_PINNED, 0));
  CVWrapperForNvCVImage(&m_latestFrameBgrPinned, m_latestFrameBgr.get());

  {
    const float framerate = static_cast<float>(m_cap->get(cv::CAP_PROP_FPS));
    m_frameIntervalMs = 1000.f / framerate;
    LOG_INFO("Opened video %s of resolution %u x %u and framerate %f", m_cameraDescriptor.camera_device_path.c_str(),
             m_imageWidth, m_imageHeight, framerate);
  }

  m_initialized = true;
bail:
  return err;
}

CaptureApi OpenCVVideo::GetCaptureApi() const { return CaptureApi::VIDEO; }

core::Error OpenCVVideo::GetFrame(NvCVImage* frame, cudaStream_t stream) {
  bool ok = true;
  NvCV_Status nv_err = NVCV_SUCCESS;
  core::Error err = core::Error::SUCCESS;

  if (!m_initialized) return core::Error::ERR_CAMERA;
  if (m_limitFrameRate) {
    m_dt = std::chrono::high_resolution_clock::now() - m_timeStart;
    float elapsed_ms = std::chrono::duration<float>(m_dt).count();
    while (elapsed_ms < m_frameIntervalMs) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      m_dt = std::chrono::high_resolution_clock::now() - m_timeStart;
      elapsed_ms = std::chrono::duration<float>(m_dt).count() * 1000.f;
    }
    m_timeStart = std::chrono::high_resolution_clock::now();
  }

  // get frame
  ok = m_cap->read(*m_latestFrameBgr);
  if (m_latestFrameBgr->empty() && m_loopVideo) {
    m_cap->set(cv::CAP_PROP_POS_FRAMES, 0);
    m_cap->read(*m_latestFrameBgr);
    CVWrapperForNvCVImage(&m_latestFrameBgrPinned, m_latestFrameBgr.get());

    if (m_latestFrameBgr->empty()) return core::Error::ERR_CAMERA;
  }

  if (m_cameraDescriptor.flip_horizontal) {
    cv::flip(*m_latestFrameBgr, *m_latestFrameBgr, 1);
  }

  nv_err = NvCVImage_Init(frame, m_latestFrameBgrPinned.width, m_latestFrameBgrPinned.height,
                          m_latestFrameBgrPinned.pitch, m_latestFrameBgrPinned.pixels,
                          m_latestFrameBgrPinned.pixelFormat, m_latestFrameBgrPinned.componentType,
                          m_latestFrameBgrPinned.planar, m_latestFrameBgrPinned.gpuMem);
  BAIL_IF_NVCVERR(nv_err, err);
bail:
  if (!ok) {
    LOG_WARNING("Unable to read frame from OpenCV video");
    return core::Error::ERR_DATA_UNAVAILABLE;
  }
  return err;
}

core::Error OpenCVVideo::GetFramerate(float* framerate) const {
  if (!framerate) return core::Error::ERR_NULL_POINTER;
  if (!m_cap) return core::Error::ERR_CAMERA;
  double val = m_cap->get(cv::CAP_PROP_FPS);
  if (val == 0.0f) {
    return core::Error::ERR_CAMERA_API;
  }
  *framerate = static_cast<float>(val);
  return core::Error::SUCCESS;
}

core::Error OpenCVVideo::GetResolution(int* width, int* height) const {
  if (!width) return core::Error::ERR_NULL_POINTER;
  if (!height) return core::Error::ERR_NULL_POINTER;
  if (!m_cap) return core::Error::ERR_CAMERA;
  *width = m_imageWidth;
  *height = m_imageHeight;
  return core::Error::SUCCESS;
}

core::Error OpenCVVideo::GetGain(float* gain) const {
  if (!gain) return core::Error::ERR_NULL_POINTER;
  if (!m_cap) return core::Error::ERR_CAMERA;
  double val = m_cap->get(cv::CAP_PROP_GAIN);
  if (val == 0.0f) {
    return core::Error::ERR_CAMERA_API;
  }
  *gain = static_cast<float>(val);
  return core::Error::SUCCESS;
}

core::Error OpenCVVideo::GetExposure(float* exposure) const {
  if (!exposure) return core::Error::ERR_NULL_POINTER;
  if (!m_cap) return core::Error::ERR_CAMERA;
  double val = m_cap->get(cv::CAP_PROP_EXPOSURE);
  if (val == 0.0f) {
    return core::Error::ERR_CAMERA_API;
  }
  *exposure = static_cast<float>(val);
  return core::Error::SUCCESS;
}

core::Error OpenCVVideo::GetGamma(float* gamma) const {
  if (!gamma) return core::Error::ERR_NULL_POINTER;
  if (!m_cap) return core::Error::ERR_CAMERA;
  double val = m_cap->get(cv::CAP_PROP_GAMMA);
  if (val == 0.0f) {
    return core::Error::ERR_CAMERA_API;
  }
  *gamma = static_cast<float>(val);
  return core::Error::SUCCESS;
}

core::Error OpenCVVideo::GetFrameCount(unsigned* frame_count) const {
  if (!frame_count) return core::Error::ERR_NULL_POINTER;
  if (!m_cap) return core::Error::ERR_CAMERA;
  double val = m_cap->get(cv::CAP_PROP_FRAME_COUNT);
  if (val == 0.0f) {
    return core::Error::ERR_CAMERA_API;
  }
  *frame_count = static_cast<unsigned>(val);
  return core::Error::SUCCESS;
}

core::Error OpenCVVideo::SetGain(const float new_gain) {
  if (!m_cap) return core::Error::ERR_CAMERA;
  const double value = new_gain;
  if (m_cap->set(cv::CAP_PROP_GAMMA, value)) {
    return core::Error::SUCCESS;
  } else {
    return core::Error::ERR_CAMERA_API;
  }
}

core::Error OpenCVVideo::SetExposure(const float new_exposure) {
  if (!m_cap) return core::Error::ERR_CAMERA;
  const double value = new_exposure;
  if (m_cap->set(cv::CAP_PROP_EXPOSURE, value)) {
    return core::Error::SUCCESS;
  } else {
    return core::Error::ERR_CAMERA_API;
  }
}

core::Error OpenCVVideo::SetGamma(const float new_gamma) {
  if (!m_cap) return core::Error::ERR_CAMERA;
  const double value = new_gamma;
  if (m_cap->set(cv::CAP_PROP_GAMMA, value)) {
    return core::Error::SUCCESS;
  } else {
    return core::Error::ERR_CAMERA_API;
  }
}

core::Error OpenCVVideo::StopCapture() {
  if (!m_cap) return core::Error::ERR_CAMERA;
  if (m_cap->isOpened()) m_cap->release();
  return core::Error::SUCCESS;
}

}  // namespace capturedevice
}  // namespace capturemodule
}  // namespace modules
}  // namespace nv3dvc
