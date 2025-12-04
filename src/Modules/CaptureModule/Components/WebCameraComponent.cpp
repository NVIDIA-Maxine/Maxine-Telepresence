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

#include "WebCameraComponent.h"

#include <glm/trigonometric.hpp>

#include "Core/Error.h"
#include "Core/Util/Logger.h"
#include "Modules/CommonModule/Components/CameraCalibrationComponent.h"

namespace nv3dvc {
namespace modules {
namespace capturemodule {
namespace components {

WebCameraComponent::CameraDescriptorProperties::CameraDescriptorProperties(core::properties::PropertyOwner* owner,
                                                                           const std::string& name)
    : m_name(name) {
  owner->AddSubOwner(this);
}

WebCameraComponent::CameraDescriptorProperties::operator CameraDescriptor() const {
  return {
      camera_device_index, camera_device_path, cuda_device_index, flip_horizontal, auto_exposure, exposure, gain, fps};
}

core::Error WebCameraComponent::Initialize(commonmodule::components::CameraCalibrationComponent* camera_calibration,
                                           CUcontext cu_context) {
  core::Error err = core::Error::SUCCESS;
  camera_calibration->Update();
  m_captureDevice = CreateCaptureDevice(capture_api, camera_descriptor, camera_calibration->image_width,
                                        camera_calibration->image_height, cu_context);
  int width = 0;
  int height = 0;
  CHECK_NONNULL(m_captureDevice.get(), core::Error::ERR_CAMERA, "Unable to create capture device");
  CHECK_SUCCESS(m_captureDevice->GetResolution(&width, &height));
  if (force_calibration) {
    CHECK_TRUE(camera_calibration->image_width == width && camera_calibration->image_height == height,
               core::Error::ERR_PARAMETER_MISMATCH, "Mismatch in forced resolution %d x %d, (actual %d x %d)", width,
               height, *camera_calibration->image_width.get(), *camera_calibration->image_height.get());
  } else {
    camera_calibration->image_width = width;
    camera_calibration->image_height = height;
    camera_calibration->vfov = commonmodule::components::CameraCalibrationComponent::kDefaultvFov;
    camera_calibration->fx =
        (height / 2.0f) /
        glm::tan(glm::radians(commonmodule::components::CameraCalibrationComponent::kDefaultvFov / 2.0f));
    camera_calibration->fy =
        (height / 2.0f) /
        glm::tan(glm::radians(commonmodule::components::CameraCalibrationComponent::kDefaultvFov / 2.0f));
    camera_calibration->cx = width / 2.0f;
    camera_calibration->cy = height / 2.0f;
  }
bail:
  return err;
}

CameraDescriptor WebCameraComponent::GetCameraDescriptor() const { return camera_descriptor; }

core::Error WebCameraComponent::GetFrame(NvCVImage* frame, const cudaStream_t stream) {
  core::Error err = core::Error::SUCCESS;
  BAIL_IF_NULL(m_captureDevice.get(), err, core::Error::ERR_CAMERA);
  err = m_captureDevice->GetFrame(frame, stream);
bail:
  return err;
}

bool WebCameraComponent::HasAudio() const { return m_captureDevice ? m_captureDevice->HasAudio() : false; }

core::Error WebCameraComponent::GetAudioFormat(int* sample_rate, int* num_channels) const {
  return m_captureDevice ? m_captureDevice->GetAudioFormat(sample_rate, num_channels) : core::Error::ERR_INITIALIZATION;
}

core::Error WebCameraComponent::GetAudioData(float* dst_data, int dst_samples, int* pulled_samples) {
  return m_captureDevice ? m_captureDevice->PullAudioData(dst_data, dst_samples, pulled_samples)
                         : core::Error::ERR_INITIALIZATION;
}

}  // namespace components
}  // namespace capturemodule
}  // namespace modules
}  // namespace nv3dvc
