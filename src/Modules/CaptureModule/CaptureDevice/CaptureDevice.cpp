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

#include "CaptureDevice.h"

#include "MultimediaFile.h"
#include "NvWebCamera.h"
#include "OpenCVCamera.h"
#include "OpenCVVideo.h"
#include "core/Util/Logger.h"

namespace nv3dvc {
namespace modules {
namespace capturemodule {
namespace capturedevice {

std::unique_ptr<ICaptureDevice> CreateCaptureDevice(const CaptureApi api, const CameraDescriptor& camera_descriptor,
                                                    const uint32_t width, const uint32_t height, CUcontext cu_context) {
  std::unique_ptr<ICaptureDevice> capture_device = nullptr;
  core::Error err = core::Error::SUCCESS;

  switch (api) {
    case OPENCV_WEBCAM:
      capture_device = std::make_unique<OpenCVCamera>();
      break;
    case NV_WEBCAM:
      capture_device = std::make_unique<NvWebCamera>(cu_context);
      break;
    case VIDEO:
      capture_device = std::make_unique<OpenCVVideo>();
      break;
    case MULTIMEDIA_FILE:
      capture_device = std::make_unique<MultimediaFile>(/* add_silent_audio_if_missing */ true);
      break;
    case UNKNOWN:
      capture_device = nullptr;
  }
  if (!capture_device) {
    LOG_ERROR("Capture device specification was unknown.");
  } else if (capture_device->Initialize(camera_descriptor, width, height) != core::Error::SUCCESS) {
    LOG_ERROR("Failed to initialize capture device.");
  }
  return capture_device;
}

}  // namespace capturedevice
}  // namespace capturemodule
}  // namespace modules
}  // namespace nv3dvc
