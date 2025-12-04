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

#ifndef SRC_APPLICATIONS_MAGICMIRRORLOCALAPPLICATION_MAGICMIRRORLOCALAPPLICATION_H_
#define SRC_APPLICATIONS_MAGICMIRRORLOCALAPPLICATION_MAGICMIRRORLOCALAPPLICATION_H_

#include "Core/Application/Application.h"

namespace nv3dvc {
namespace applications {

/// @brief Application for performing single user mirror like experiences
///
/// @anchor MagicMirrorLocalApplication
/// The application works with local processing only. The same video stream, typically from webcam input, is used both
/// for head tracking for view dependent display, and for for head pose tracking and volumetric encoding + rendering.
/// The user will be able to see their own face rendered volumetrically, as if the display surface was a mirror.
class MagicMirrorLocalApplication : public core::application::Application {
 public:
  MagicMirrorLocalApplication();
  core::Error BuildDefaultScene() override;
};

}  // namespace applications
}  // namespace nv3dvc

#endif  // SRC_APPLICATIONS_MAGICMIRRORLOCALAPPLICATION_MAGICMIRRORLOCALAPPLICATION_H_
