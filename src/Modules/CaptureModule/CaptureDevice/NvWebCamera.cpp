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

#include "NvWebCamera.h"

#include <nppi.h>

#include <chrono>  // NOLINT(build/c++11)
#include <thread>  // NOLINT(build/c++11)

#include "Core/Error.h"
#include "Core/Util/Logger.h"
#include "nvCVImage.h"

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#include <d3d11.h>
#include <webcam.h>
#endif

namespace nv3dvc {
namespace modules {
namespace capturemodule {
namespace capturedevice {

NvWebCamera::NvWebCamera(CUcontext cu_context)
    : m_initialized{false},
      m_cameraDescriptor{},
      m_webcam{nullptr},
      m_gainRange{0, 0, 0, 0},
      m_exposureRange{0, 0, 0, 0},
      m_gammaRange{0, 0, 0, 0},
      m_framerate{0.0f},
      m_imageWidth{0},
      m_imageHeight{0},
      m_cuContext(cu_context),
      m_nppStreamContext({0}) {}

NvWebCamera::~NvWebCamera() {
  m_initialized = false;

  if (m_webcam) {
    WC_DestroyWebcam(m_webcam);
    m_webcam = nullptr;
  }
}

core::Error NvWebCamera::Initialize(const CameraDescriptor& camera_descriptor, const uint32_t width,
                                    const uint32_t height) {
  WebcamErr cam_err = WebcamErr::ErrOk;
  core::Error err = core::Error::SUCCESS;

  m_cameraDescriptor = camera_descriptor;

  // Find list of webcams
  uint32_t num_cams = 0;
  ICaptureWebcamDesc** cameras = nullptr;
  cam_err = WC_FindWebcams(cameras, num_cams);
  CHECK_TRUE(cam_err == WebcamErr::ErrOk, core::Error::ERR_CAMERA, "No webcams through NvWebcam found.");
  LOG_INFO("Found %u webcams through NvWebcam", num_cams);
  CHECK_TRUE(m_cameraDescriptor.camera_device_index < static_cast<int>(num_cams), core::Error::ERR_CAMERA,
             "Invalid requested zero-based device index %d", m_cameraDescriptor.camera_device_index);

  // Find stream within selected webcam
  uint32_t num_streams = 0;
  ICaptureWebcamStream** streams = nullptr;
  ICaptureWebcamDesc* chosen_desc = cameras[m_cameraDescriptor.camera_device_index];

  constexpr int kNumAttempts = 5;
  constexpr auto kRetryWait = std::chrono::seconds(1);
  for (int attempt = 0; attempt < kNumAttempts; ++attempt) {
    LOG_DEBUG("Trying to get webcam streams for camera %d", m_cameraDescriptor.camera_device_index);
    if (attempt > 0) {
      // If this is a retry, wait for a short time.
      LOG_WARNING("Waiting before retry %d", attempt);
      std::this_thread::sleep_for(kRetryWait);
    }
    // Try to get camera streams.
    cam_err = WC_GetWebcamStreams(chosen_desc, streams, num_streams);
    if (cam_err == WebcamErr::ErrOk) break;
  }

  CHECK_TRUE(cam_err == WebcamErr::ErrOk, core::Error::ERR_CAMERA, "Error getting webcam streams.");

  const float min_framerate = camera_descriptor.fps;

  constexpr WebcamFormat buffer_format = WebcamFormat::NV12;  // H264
  constexpr WebcamOutput output_buffer_format = WebcamOutput::CUDA_PITCH2D;

  int buffer_size = 0;

  // Select wanted stream
  chosen_desc->stream = nullptr;
  for (uint32_t s = 0; s < num_streams; s++) {
    ICaptureWebcamStream* stream = streams[s];
    if (stream->fps >= min_framerate && stream->fps <= min_framerate + 1.f && stream->format == buffer_format &&
        stream->width == width && stream->height == height) {
      chosen_desc->stream = stream;
      break;
    }
  }
  CHECK_NONNULL(chosen_desc->stream, core::Error::ERR_CAMERA, "No suitable stream found.");
  m_framerate = chosen_desc->stream->fps;

  // Fill in extra info for webcam init
  chosen_desc->params.bufferSize = buffer_size;
  chosen_desc->params.d3dDevice = nullptr;
  chosen_desc->params.launchMode = WebcamMode::DEFAULT;
  chosen_desc->params.outputType = output_buffer_format;
  chosen_desc->params.cuContext = m_cuContext;
  cuCtxSetCurrent(m_cuContext);

  // Create an execution context for npp
  int dev_id;
  int shared_mem_per_block;
  CHECK_CUDA_SUCCESS(cudaGetDevice(&dev_id));
  m_nppStreamContext.nCudaDeviceId = dev_id;

  CHECK_CUDA_SUCCESS(cudaDeviceGetAttribute(&m_nppStreamContext.nCudaDevAttrComputeCapabilityMajor,
                                            cudaDevAttrComputeCapabilityMajor, dev_id));
  CHECK_CUDA_SUCCESS(cudaDeviceGetAttribute(&m_nppStreamContext.nCudaDevAttrComputeCapabilityMinor,
                                            cudaDevAttrComputeCapabilityMinor, dev_id));
  CHECK_CUDA_SUCCESS(
      cudaDeviceGetAttribute(&m_nppStreamContext.nMultiProcessorCount, cudaDevAttrMultiProcessorCount, dev_id));
  CHECK_CUDA_SUCCESS(cudaDeviceGetAttribute(&m_nppStreamContext.nMaxThreadsPerMultiProcessor,
                                            cudaDevAttrMaxThreadsPerMultiProcessor, dev_id));
  CHECK_CUDA_SUCCESS(
      cudaDeviceGetAttribute(&m_nppStreamContext.nMaxThreadsPerBlock, cudaDevAttrMaxThreadsPerBlock, dev_id));
  CHECK_CUDA_SUCCESS(cudaDeviceGetAttribute(&shared_mem_per_block, cudaDevAttrMaxSharedMemoryPerBlock, dev_id));

  m_nppStreamContext.nSharedMemPerBlock = shared_mem_per_block;

  // Init webcam state
  cam_err = WC_CreateICaptureWebcam(m_webcam, chosen_desc);
  CHECK_TRUE(cam_err == WebcamErr::ErrOk, core::Error::ERR_CAMERA, "Failed to create capture webcam");
  cam_err = m_webcam->Init();
  CHECK_TRUE(cam_err == WebcamErr::ErrOk, core::Error::ERR_CAMERA, "Failed to initialize webcam");

  // Clean up webcam descs + streams used for initialization
  WC_DestroyDescs(cameras, num_cams);
  WC_DestroyStreams(streams, num_streams);

  // retrieve resolution
  m_imageWidth = static_cast<int>(m_webcam->GetWidth());
  m_imageHeight = static_cast<int>(m_webcam->GetHeight());

  CHECK_FALSE(m_imageWidth == 0, core::Error::ERR_CAMERA, "Zero camera frame size is invalid");
  CHECK_FALSE(m_imageHeight == 0, core::Error::ERR_CAMERA, "Zero camera frame size is invalid");
  // Retrieve ranges
  m_webcam->GetParameterRange(WebcamControlParameter::CameraGamma, m_gammaRange.min, m_gammaRange.max,
                              m_gammaRange.step, m_gammaRange.default_value);
  m_webcam->GetParameterRange(WebcamControlParameter::CameraExposure, m_exposureRange.min, m_exposureRange.max,
                              m_exposureRange.step, m_exposureRange.default_value);
  m_webcam->GetParameterRange(WebcamControlParameter::CameraGain, m_gainRange.min, m_gainRange.max, m_gainRange.step,
                              m_gainRange.default_value);

  m_initialized = true;
bail:
  return err;
}

CaptureApi NvWebCamera::GetCaptureApi() const { return CaptureApi::NV_WEBCAM; }

core::Error NvWebCamera::GetFrame(NvCVImage* frame, cudaStream_t stream) {
  core::Error err = core::Error::SUCCESS;
  WebcamErr cam_err = WebcamErr::ErrOk;
  NppStatus npp_err = NppStatus::NPP_SUCCESS;

  FrameDesc frame_desc;

  // return false if not initialized
  if (!m_initialized) return core::Error::ERR_INITIALIZATION;
  CHECK_CU_SUCCESS(cuCtxSetCurrent(m_cuContext), "Failed to set cuda context");

  // Read and display latest frame
  frame_desc.m_timeStamp = 0LL;
  cam_err = m_webcam->ReadFrame(frame_desc);
  if (cam_err != WebcamErr::ErrOk || frame_desc.m_numFrames == 0) {
    LOG_WARNING("No frame received by NvWebCam! This is expected behavior for a few frames during startup.");
    return core::Error::ERR_DATA_UNAVAILABLE;
  }
  CHECK_NVCV_SUCCESS(NvCVImage_Init(frame, m_imageWidth, m_imageHeight, m_imageWidth * 4,
                                    reinterpret_cast<void*>(frame_desc.m_cuPitch2D), NVCV_RGBA, NVCV_U8, NVCV_CHUNKY,
                                    NVCV_CUDA));

  if (m_cameraDescriptor.flip_horizontal) {
    if (frame->width != m_tmpImage.width || frame->height != m_tmpImage.height ||
        frame->pixelFormat != m_tmpImage.pixelFormat || frame->componentType != m_tmpImage.componentType ||
        frame->planar != m_tmpImage.planar) {
      CHECK_NVCV_SUCCESS(NvCVImage_Realloc(&m_tmpImage, frame->width, frame->height, frame->pixelFormat,
                                           frame->componentType, frame->planar, frame->gpuMem, 1));
    }
    NppiSize oSrcRectROI = {static_cast<int>(frame->width), static_cast<int>(frame->height)};
    const Npp8u* pSrc = static_cast<Npp8u*>(frame->pixels);
    Npp8u* pDst = static_cast<Npp8u*>(m_tmpImage.pixels);
    int pitch = frame->pitch;
    m_nppStreamContext.hStream = stream;
    // Note: NPP_VERTICAL_AXIS is flip AROUND the vertical axis, i.e. ALONG the horizontal axis
    npp_err =
        nppiMirror_8u_C4R_Ctx(pSrc, pitch, pDst, pitch, oSrcRectROI, NppiAxis::NPP_VERTICAL_AXIS, m_nppStreamContext);
    CHECK_TRUE(npp_err == NppStatus::NPP_SUCCESS, core::Error::ERR_CAMERA,
               "nppiMirror_8u_C4R_Ctx failed with error code %d", npp_err);
    // Reinit frame with flipped image
    CHECK_NVCV_SUCCESS(NvCVImage_Init(frame, m_imageWidth, m_imageHeight, m_imageWidth * 4, m_tmpImage.pixels,
                                      NVCV_RGBA, NVCV_U8, NVCV_CHUNKY, NVCV_CUDA));
  }

bail:
  return err;
}

core::Error NvWebCamera::GetFramerate(float* framerate) const {
  if (!framerate) return core::Error::ERR_NULL_POINTER;
  if (!m_initialized) return core::Error::ERR_CAMERA;
  *framerate = m_framerate;
  return core::Error::SUCCESS;
}

core::Error NvWebCamera::GetResolution(int* width, int* height) const {
  if (!width || !height) return core::Error::ERR_NULL_POINTER;
  if (!m_initialized) return core::Error::ERR_CAMERA;
  *width = m_imageWidth;
  *height = m_imageHeight;
  return core::Error::SUCCESS;
}

core::Error NvWebCamera::GetGainLimits(float* min_gain, float* max_gain) const {
  if (!min_gain || !max_gain) return core::Error::ERR_NULL_POINTER;
  if (!m_initialized) return core::Error::ERR_CAMERA;
  *min_gain = static_cast<float>(m_gainRange.min);
  *max_gain = static_cast<float>(m_gainRange.max);
  return core::Error::SUCCESS;
}

core::Error NvWebCamera::GetGain(float* gain) const {
  if (!gain) return core::Error::ERR_NULL_POINTER;
  if (!m_initialized) return core::Error::ERR_CAMERA;
  int32_t value;
  m_webcam->GetParameterValue(WebcamControlParameter::CameraGain, value);
  *gain = static_cast<float>(value);
  return core::Error::SUCCESS;
}

core::Error NvWebCamera::GetExposureLimits(float* min_exposure, float* max_exposure) const {
  if (!min_exposure || !max_exposure) return core::Error::ERR_NULL_POINTER;
  if (!m_initialized) return core::Error::ERR_CAMERA;
  *min_exposure = static_cast<float>(m_exposureRange.min);
  *max_exposure = static_cast<float>(m_exposureRange.max);
  return core::Error::SUCCESS;
}

core::Error NvWebCamera::GetExposure(float* exposure) const {
  if (!exposure) return core::Error::ERR_NULL_POINTER;
  if (!m_initialized) return core::Error::ERR_CAMERA;
  int32_t value;
  m_webcam->GetParameterValue(WebcamControlParameter::CameraExposure, value);
  *exposure = static_cast<float>(value);
  return core::Error::SUCCESS;
}

core::Error NvWebCamera::GetGammaLimits(float* min_gamma, float* max_gamma) const {
  if (!min_gamma || !max_gamma) return core::Error::ERR_NULL_POINTER;
  if (!m_initialized) return core::Error::ERR_CAMERA;
  *min_gamma = static_cast<float>(m_gammaRange.min);
  *max_gamma = static_cast<float>(m_gammaRange.max);
  return core::Error::SUCCESS;
}

core::Error NvWebCamera::GetGamma(float* gamma) const {
  if (!gamma) return core::Error::ERR_NULL_POINTER;
  if (!m_initialized) return core::Error::ERR_CAMERA;
  int32_t value;
  m_webcam->GetParameterValue(WebcamControlParameter::CameraGamma, value);
  *gamma = static_cast<float>(value);
  return core::Error::SUCCESS;
}

core::Error NvWebCamera::SetGain(const float new_gain) {
  if (!m_initialized) return core::Error::ERR_CAMERA;
  const int32_t value = static_cast<int32_t>(new_gain);
  return m_webcam->SetParameterValue(WebcamControlParameter::CameraGain, value) ? core::Error::SUCCESS
                                                                                : core::Error::ERR_CAMERA_API;
}

core::Error NvWebCamera::SetExposure(const float new_exposure) {
  if (!m_initialized) return core::Error::ERR_CAMERA;
  const int32_t value = static_cast<int32_t>(new_exposure);
  return m_webcam->SetParameterValue(WebcamControlParameter::CameraExposure, value) ? core::Error::SUCCESS
                                                                                    : core::Error::ERR_CAMERA_API;
}

core::Error NvWebCamera::SetGamma(const float new_gamma) {
  if (!m_initialized) return core::Error::ERR_CAMERA;
  const int32_t value = static_cast<int32_t>(new_gamma);
  return m_webcam->SetParameterValue(WebcamControlParameter::CameraGamma, value) ? core::Error::SUCCESS
                                                                                 : core::Error::ERR_CAMERA_API;
}

}  // namespace capturedevice
}  // namespace capturemodule
}  // namespace modules
}  // namespace nv3dvc
