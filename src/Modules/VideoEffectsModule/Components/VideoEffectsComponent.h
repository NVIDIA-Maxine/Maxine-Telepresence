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

#ifndef SRC_MODULES_VIDEOEFFECTSMODULE_COMPONENTS_VIDEOEFFECTSCOMPONENT_H_
#define SRC_MODULES_VIDEOEFFECTSMODULE_COMPONENTS_VIDEOEFFECTSCOMPONENT_H_

#include <string>

#include "Core/EntityComponentSystem/Component.h"
#include "glm/glm.hpp"
#include "nvCVImage.h"

namespace nv3dvc {
namespace modules {
namespace videoeffectsmodule {
namespace components {

/// @brief Component used for signaling that video effects should be applied to a video frame.
///
/// Typically added to entities which also carry VideoFrameComponent. Video effects systems may act on the video frame
/// if a VideoEffectsComponent also exists.
class VideoEffectsComponent : public core::ecs::Component {
 public:
  constexpr static const char* NAME = "VideoEffectsComponent";
  std::string Name() const override { return NAME; };

  VideoEffectsComponent();

  ~VideoEffectsComponent() override;

  /// @brief Whether AI Green Screen (AIGS) should be applied to the image frame
  /// @return Whether AI Green Screen (AIGS) should be applied to the image frame
  bool ShouldPerformAigs() const;

  /// @brief Get internal pointer to image used as matte. Potentially unallocated
  /// @return Matte image pointer
  NvCVImage* GetMatteImagePtr();

  /// @brief Device pointer to single pixel containing color used for specifying background
  /// @return CUDA device pointer to a three channeled U8 pixel
  void* GetBackgroundColorDevicePtr();

 public:
  /// @defgroup VideoEffectsComponentProperties VideoEffectsComponent
  /// @ingroup ComponentProperties
  /// @{
  core::properties::Property<std::atomic<bool>> perform_aigs = {
      this,
      "perform_aigs",
      "Whether AI Green Screen (AIGS) should be applied to the image frame during execution of the AigsSystem. See "
      "documentation of AigsSystem for details.",
      true,
  };
  core::properties::Property<glm::u8vec3> background_color = {
      this,
      "background_color",
      "Background color which gets applied to the background of the image frame when AIGS is applied.",
      {32, 32, 32},
  };
  /// @}

 private:
  NvCVImage m_matteImage;
  void* m_bgColorDevicePtr;
};

}  // namespace components
}  // namespace videoeffectsmodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_VIDEOEFFECTSMODULE_COMPONENTS_VIDEOEFFECTSCOMPONENT_H_
