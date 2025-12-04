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

#ifndef SRC_MODULES_CAPTUREMODULE_CAPTUREDEVICE_CAPTUREDEVICE_H_
#define SRC_MODULES_CAPTUREMODULE_CAPTUREDEVICE_CAPTUREDEVICE_H_

#include <cuda.h>
#include <driver_types.h>

#include <memory>

#include "Core/Error.h"
#include "Modules/CaptureModule/CameraDescriptor.h"

// Forward declaration
class NvCVImage;

namespace nv3dvc {
namespace modules {
namespace capturemodule {
namespace capturedevice {

/// @ingroup EnumProperties
/// @defgroup CaptureApi CaptureApi
/// @brief Camera capture API
/// @{
enum CaptureApi {
  UNKNOWN,          ///< Not defined
  OPENCV_WEBCAM,    ///< OpenCV web camera capture API
  NV_WEBCAM,        ///< NVIDIA web camera capture API
  VIDEO,            ///< OpenCV video capture API, no audio
  MULTIMEDIA_FILE,  ///< Gstreamer video file capture API with audio support
};
/// @}

/// @brief Abstract interface for capture devices
/// This capture device interface works by fetching frames rather than usage of a callback function
class ICaptureDevice {
 public:
  virtual ~ICaptureDevice() = default;

  /// @brief Open the stream for the device specified by the descriptor and calibration parameters
  /// @param[in] camera_descriptor        See CameraDescriptor
  /// @param[in] width                    The width of the frame in pixels
  /// @param[in] height                   The height of the frame in pixels
  /// @return    core::Error::ERR_CAMERA  If the camera failed to initialize
  ///            core::Error::SUCCESS     If successful
  virtual core::Error Initialize(const CameraDescriptor& camera_descriptor, uint32_t width, uint32_t height) = 0;

  /// @brief Get the capture API for specific implementation
  /// @return UNKNOWN          For other extensions
  ///         OPENCV_WEBCAM    For OpenCVCamera
  ///         NV_WEBCAM        For NvWebCamera
  ///         VIDEO            For OpenCVVideo
  ///         MULTIMEDIA_FILE  For MultimediaFile
  virtual CaptureApi GetCaptureApi() const = 0;

  /// @brief Get the latest frame from the device
  /// @param[out] frame                  An uninitialized frame which will be a view into the read pixel data
  /// @param[in]  stream                 The CUDA stream on which the frame transfer happens
  /// @return     core::Error::ERR_READ  If reading failed
  ///             core::Error::SUCCESS   If successful
  virtual core::Error GetFrame(NvCVImage* frame, cudaStream_t stream = nullptr) = 0;

  /// @brief Get the frame rate [frames per second]
  /// @param[out] framerate                      A location of where the frame rate will be written
  /// @return     core::Error::ERR_NULL_POINTER  If a null pointer was supplied
  /// @return     core::Error::ERR_CAMERA        If the capture device has not been properly initialized
  /// @return     core::Error::ERR_CAMERA_API    If the API function does not exist, or failed for other reason
  /// @return     core::Error::SUCCESS           If successful
  virtual core::Error GetFramerate(float* framerate) const = 0;

  /// @brief Get the resolution of the target frame in pixels
  /// @param[out] width                          A location of where the width will be written
  /// @param[out] height                         A location of where the height will be written
  /// @return     core::Error::ERR_NULL_POINTER  If a null pointer was supplied
  /// @return     core::Error::ERR_CAMERA        If the capture device has not been properly initialized
  /// @return     core::Error::ERR_CAMERA_API    If the API function does not exist, or failed for other reason
  /// @return     core::Error::SUCCESS           If successful
  virtual core::Error GetResolution(int* width, int* height) const = 0;

  /// @brief Get the gain limits
  /// This is an optional API function
  /// @param[out] min_gain                       A location of where the minimum gain will be written
  /// @param[out] max_gain                       A location of where the maximum gain will be written
  /// @return     core::Error::ERR_NULL_POINTER  If a null pointer was supplied
  /// @return     core::Error::ERR_CAMERA        If the capture device has not been properly initialized
  /// @return     core::Error::ERR_CAMERA_API    If the API function does not exist, or failed for other reason
  /// @return     core::Error::SUCCESS           If successful
  virtual core::Error GetGainLimits(float* min_gain, float* max_gain) const { return core::Error::ERR_CAMERA_API; }

  /// @brief Get the current gain level
  /// This is an optional API function
  /// @param[out] gain                           A location of where the gain will be written
  /// @return     core::Error::ERR_NULL_POINTER  If a null pointer was supplied
  /// @return     core::Error::ERR_CAMERA        If the capture device has not been properly initialized
  /// @return     core::Error::ERR_CAMERA_API    If the API function does not exist, or failed for other reason
  /// @return     core::Error::SUCCESS           If successful
  virtual core::Error GetGain(float* gain) const { return core::Error::ERR_CAMERA_API; }

  /// @brief Get the exposure limits
  /// This is an optional API function
  /// @param[out] min_exposure                   A location of where the minimum exposure will be written
  /// @param[out] max_exposure                   A location of where the maximum exposure will be written
  /// @return     core::Error::ERR_NULL_POINTER  If a null pointer was supplied
  /// @return     core::Error::ERR_CAMERA        If the capture device has not been properly initialized
  /// @return     core::Error::ERR_CAMERA_API    If the API function does not exist, or failed for other reason
  /// @return     core::Error::SUCCESS           If successful
  virtual core::Error GetExposureLimits(float* min_exposure, float* max_exposure) const {
    return core::Error::ERR_CAMERA_API;
  }

  /// @brief Get the current exposure level
  /// This is an optional API function
  /// @param[out] exposure                       A location of where the exposure will be written
  /// @return     core::Error::ERR_NULL_POINTER  If a null pointer was supplied
  /// @return     core::Error::ERR_CAMERA        If the capture device has not been properly initialized
  /// @return     core::Error::ERR_CAMERA_API    If the API function does not exist, or failed for other reason
  /// @return     core::Error::SUCCESS           If successful
  virtual core::Error GetExposure(float* exposure) const { return core::Error::ERR_CAMERA_API; }

  /// @brief Get the gamma limits
  /// This is an optional API function
  /// @param[out] min_gamma                      A location of where the minimum gamma will be written
  /// @param[out] max_gamma                      A location of where the maximum gamma will be written
  /// @return     core::Error::ERR_NULL_POINTER  If a null pointer was supplied
  /// @return     core::Error::ERR_CAMERA        If the capture device has not been properly initialized
  /// @return     core::Error::ERR_CAMERA_API    If the API function does not exist, or failed for other reason
  /// @return     core::Error::SUCCESS           If successful
  virtual core::Error GetGammaLimits(float* min_gamma, float* max_gamma) const { return core::Error::ERR_CAMERA_API; }

  /// @brief Get the current gamma level
  /// This is an optional API function
  /// @param[out] gamma                          A location of where the gamma will be written
  /// @return     core::Error::ERR_NULL_POINTER  If a null pointer was supplied
  /// @return     core::Error::ERR_CAMERA        If the capture device has not been properly initialized
  /// @return     core::Error::ERR_CAMERA_API    If the API function does not exist, or failed for other reason
  /// @return     core::Error::SUCCESS           If successful
  virtual core::Error GetGamma(float* gamma) const { return core::Error::ERR_CAMERA_API; }

  /// @brief Get the total number of frames available
  /// Only valid for a prerecorded capture device such as a video read from file
  /// This is an optional API function
  /// @param[out] frame_count                    A location of where the frame count will be written
  /// @return     core::Error::ERR_NULL_POINTER  If a null pointer was supplied
  /// @return     core::Error::ERR_CAMERA        If the capture device has not been properly initialized
  /// @return     core::Error::ERR_CAMERA_API    If the API function does not exist, or failed for other reason
  /// @return     core::Error::SUCCESS           If successful
  virtual core::Error GetFrameCount(unsigned* frame_count) const { return core::Error::ERR_CAMERA_API; }

  /// @brief Set the gain level
  /// This is an optional API function
  /// @param[in] new_gain                      The new gain level
  /// @return     core::Error::ERR_CAMERA      If the capture device has not been properly initialized
  /// @return     core::Error::ERR_CAMERA_API  If the API function does not exist, or failed for other reason
  /// @return     core::Error::SUCCESS         If successful
  virtual core::Error SetGain(float new_gain) { return core::Error::ERR_CAMERA_API; }

  /// @brief Set the exposure level
  /// This is an optional API function
  /// @param[in] new_exposure                  The new exposure level
  /// @return     core::Error::ERR_CAMERA      If the capture device has not been properly initialized
  /// @return     core::Error::ERR_CAMERA_API  If the API function does not exist, or failed for other reason
  /// @return     core::Error::SUCCESS         If successful
  virtual core::Error SetExposure(float new_exposure) { return core::Error::ERR_CAMERA_API; }

  /// @brief Set the gamma level
  /// This is an optional API function
  /// @param[in] new_gamma                     The new gamma level
  /// @return     core::Error::ERR_CAMERA      If the capture device has not been properly initialized
  /// @return     core::Error::ERR_CAMERA_API  If the API function does not exist, or failed for other reason
  /// @return     core::Error::SUCCESS         If successful
  virtual core::Error SetGamma(float new_gamma) { return core::Error::ERR_CAMERA_API; }

  /// @brief Report whether the device can provide audio data
  /// This is an optional API function
  /// @return    true      If device can provide audio
  virtual bool HasAudio() const { return false; }

  /// @brief Get the audio sample rate [samples per second] and number of channels
  /// @param[out] sample_rate                    A location of where the sample rate will be written
  /// @param[out] num_channels                   A location of where the number of channels will be written
  /// @return     core::Error::ERR_NULL_POINTER  If a null pointer was supplied
  /// @return     core::Error::ERR_CAMERA        If the capture device has not been properly initialized
  /// @return     core::Error::ERR_CAMERA_API    If the API function does not exist, or failed for other reason
  /// @return     core::Error::SUCCESS           If successful
  virtual core::Error GetAudioFormat(int* sample_rate, int* num_channels) const { return core::Error::ERR_CAMERA_API; }

  /// @brief Get the latest audio frames from the device
  /// @param[out] dst_data        A pointer to a buffer that will be filled with audio data
  /// @param[in]  dst_samples     The duration of the dst_data buffer, measured in samples
  /// @param[out] pulled_samples  The duration of audio that was copied into dst_data, measured in samples
  /// @return     core::Error::ERR_NULL_POINTER  If a null pointer was supplied
  /// @return     core::Error::ERR_CAMERA        If the capture device has not been properly initialized
  /// @return     core::Error::ERR_CAMERA_API    If the API function does not exist, or failed for other reason
  /// @return     core::Error::SUCCESS           If successful
  virtual core::Error PullAudioData(float* dst_data, int dst_samples, int* pulled_samples) {
    return core::Error::ERR_CAMERA_API;
  }

  /// @brief Stop the capture
  /// This is an optional API function
  /// @return     core::Error::ERR_CAMERA      If the capture device has not been properly initialized
  /// @return     core::Error::ERR_CAMERA_API  If the API function does not exist, or failed for other reason
  /// @return     core::Error::SUCCESS         If successful
  virtual core::Error StopCapture() { return core::Error::ERR_CAMERA_API; }
};

/// @brief CaptureDevice factory. Create a capture device specified by the provided arguments
/// @param[in] api                The Capture device API to use
/// @param[in] camera_descriptor  The camera descriptor for the capture device. See CameraDescriptor
/// @param[in] width              The width of the frame in pixels
/// @param[in] height             The height of the frame in pixels
/// @param[in] cu_context         CUcontext that CUDA-based capture devices should use
/// @return    The capture device initialized, or nullptr if initialization failed
std::unique_ptr<ICaptureDevice> CreateCaptureDevice(CaptureApi api, const CameraDescriptor& camera_descriptor,
                                                    uint32_t width, uint32_t height, CUcontext cu_context);

}  // namespace capturedevice
}  // namespace capturemodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_CAPTUREMODULE_CAPTUREDEVICE_CAPTUREDEVICE_H_
