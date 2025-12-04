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

#ifndef SRC_MODULES_CAPTUREMODULE_COMPONENTS_WEBCAMERACOMPONENT_H_
#define SRC_MODULES_CAPTUREMODULE_COMPONENTS_WEBCAMERACOMPONENT_H_

#include <memory>
#include <string>

#include "Core/EntityComponentSystem/Component.h"
#include "Core/Error.h"
#include "Modules/CaptureModule/CaptureDevice/CaptureDevice.h"
#include "Modules/CommonModule/CameraCalibration.h"
#include "Modules/CommonModule/Components/CameraCalibrationComponent.h"
#include "glm/glm.hpp"
#include "nvCVImage.h"

namespace nv3dvc {
namespace modules {
namespace capturemodule {
namespace components {

/// @defgroup WebCameraComponentProperties WebCameraComponent
/// @ingroup  ComponentProperties
/// @brief    This component represents a physical camera device for tracking
///
/// The component is acted upon by CameraCaptureSystem, if the entity which carries the WebCameraComponent also carries
/// a VideoFrameComponent. In that case, the camera capture system will use the GetFrame function of the
/// WebCameraComponent to capture the latest frame and write it to the VideoFrameComponent in question.

/// See @ref WebCameraComponentProperties
class WebCameraComponent : public core::ecs::Component {
 public:
  /// @defgroup CameraDescriptorPropertiesProperties CameraDescriptorProperties
  /// @ingroup  ComponentProperties
  /// @brief    Descriptor for initializing capture devices

  /// See @ref CameraDescriptorPropertiesProperties
  class CameraDescriptorProperties : public core::properties::PropertyOwner {
   public:
    std::string Name() const override { return m_name; };
    CameraDescriptorProperties(core::properties::PropertyOwner* owner, const std::string& name);
    operator CameraDescriptor() const;

    /// @ingroup CameraDescriptorPropertiesProperties
    /// @{
    core::properties::Property<int> camera_device_index = {
        this,
        "camera_device_index",
        "Typically an index to a web camera",
        0,
    };
    core::properties::Property<std::string> camera_device_path = {
        this,
        "camera_device_path",
        "Path for opening a camera stream. Typically, a file path",
        "",
    };
    core::properties::Property<int> cuda_device_index = {
        this,
        "cuda_device_index",
        "For CUDA capture devices, which CUDA device to run the capture device on",
        0,
    };
    core::properties::Property<bool> flip_horizontal = {
        this,
        "flip_horizontal",
        "Whether to flip the frame horizontally",
        false,
    };
    core::properties::Property<bool> auto_exposure = {
        this,
        "auto_exposure",
        "Whether to use auto exposure",
        true,
    };
    core::properties::Property<float> exposure = {
        this,
        "exposure",
        "The exposure level at startup",
        1.0f,
    };
    core::properties::Property<float> gain = {
        this,
        "gain",
        "The gain level at startup",
        1.0f,
    };
    core::properties::Property<float> fps = {
        this,
        "fps",
        "The frame rate, frames per second",
        30.0f,
    };
    /// @}
   private:
    const std::string m_name;
  };

  constexpr static const char* NAME = "WebCameraComponent";
  std::string Name() const override { return "WebCameraComponent"; };

  WebCameraComponent() = default;
  ~WebCameraComponent() override = default;

  core::Error Initialize(commonmodule::components::CameraCalibrationComponent* camera_calibration,
                         CUcontext cu_context);

  CameraDescriptor GetCameraDescriptor() const;

  core::Error GetFrame(NvCVImage* frame, const cudaStream_t stream);

  bool HasAudio() const;
  core::Error GetAudioFormat(int* sample_rate, int* num_channels) const;
  core::Error GetAudioData(float* dst_data, int dst_samples, int* pulled_samples);

  /// @ingroup WebCameraComponentProperties
  /// @{
  /// @brief For options, see @ref CaptureApi
  core::properties::Property<capturedevice::CaptureApi> capture_api = {
      this,
      "capture_api",
      "Determines whether to use webcam or file. For options, see documentation.",
      capturedevice::CaptureApi::OPENCV_WEBCAM,
  };
  core::properties::Property<bool> force_calibration = {
      this,
      "force_calibration",
      "If false, camera_calibration parameters will use heuristics",
      false,
  };
  CameraDescriptorProperties camera_descriptor = {
      this,
      "camera_descriptor",
  };
  /// @}

 private:
  std::unique_ptr<capturedevice::ICaptureDevice> m_captureDevice;
};

}  // namespace components
}  // namespace capturemodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_CAPTUREMODULE_COMPONENTS_WEBCAMERACOMPONENT_H_
