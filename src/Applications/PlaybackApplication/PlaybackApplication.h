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

#ifndef SRC_APPLICATIONS_PLAYBACKAPPLICATION_PLAYBACKAPPLICATION_H_
#define SRC_APPLICATIONS_PLAYBACKAPPLICATION_PLAYBACKAPPLICATION_H_

#include "Core/Application/Application.h"

namespace nv3dvc {
namespace applications {

/// @brief Application for playing back video locally to simulate 1:1 video communication without networking
///
/// @anchor PlaybackApplication
/// The application works with local processing only. The incoming video stream, coming from a file read from disk, acts
/// as the simulated second client, while the local stream, typically from webcam input, is used for head tracking for
/// view dependent display. The stream from the simulated second client will be used for volumetric encoding, pose
/// tracking and rendering so that the rendered object can be inspected from different views.
class PlaybackApplication : public core::application::Application {
 public:
  PlaybackApplication();
  core::Error BuildDefaultScene() override;
};

}  // namespace applications
}  // namespace nv3dvc

#endif  // SRC_APPLICATIONS_PLAYBACKAPPLICATION_PLAYBACKAPPLICATION_H_
