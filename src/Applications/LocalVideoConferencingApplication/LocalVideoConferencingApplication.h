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

#ifndef SRC_APPLICATIONS_LOCALVIDEOCONFERENCINGAPPLICATION_LOCALVIDEOCONFERENCINGAPPLICATION_H_
#define SRC_APPLICATIONS_LOCALVIDEOCONFERENCINGAPPLICATION_LOCALVIDEOCONFERENCINGAPPLICATION_H_

#include "Core/Application/Application.h"

namespace nv3dvc {
namespace applications {

/// @brief Application for performing 1:1 video conferencing with local processing
///
/// @anchor LocalVideoConferencingApplication
/// The application sets up one incoming and one outgoing audio/video stream, thereby acting as client and as server,
/// enabling peer-to-peer connection between two instances without the use of a separate server application. The
/// processing of both the local video stream (for view dependent display) and the incoming video stream (for head pose
/// tracking and volumetric encoding + rendering) is all done locally. By default, the application will set up a scene
/// where a single instance acts as its own sender and receiver, connecting to itself over localhost.
class LocalVideoConferencingApplication : public core::application::Application {
 public:
  LocalVideoConferencingApplication();
  core::Error BuildDefaultScene() override;
};

}  // namespace applications
}  // namespace nv3dvc

#endif  // SRC_APPLICATIONS_LOCALVIDEOCONFERENCINGAPPLICATION_LOCALVIDEOCONFERENCINGAPPLICATION_H_
