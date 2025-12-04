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

#ifndef SRC_MODULES_TRIPLANEMODULE_COMPONENTS_TRIPLANELOADERBEHAVIORCOMPONENT_H_
#define SRC_MODULES_TRIPLANEMODULE_COMPONENTS_TRIPLANELOADERBEHAVIORCOMPONENT_H_

#include <string>

#include "Modules/BehaviorModule/Components/BehaviorComponent.h"
#include "nvCVImage.h"
#include "opencv2/opencv.hpp"

namespace nv3dvc {
namespace modules {
namespace triplanemodule {
namespace components {

/// @defgroup TriplaneLoaderBehaviorComponentProperties TriplaneLoaderBehaviorComponent
/// @ingroup ComponentProperties
/// @brief Behavior for loading a static triplane volume object at startup
///
/// Acts on TriplaneVolumeComponent if one exist at this components owner. During scene load, the image will be loaded
/// from disk, along with the quantization parameters, which are required to correctly represent the object. Values are
/// written to the TriplaneVolumeComponents internal static triplane volume buffer.

/// See @ref TriplaneLoaderBehaviorComponentProperties
class TriplaneLoaderBehaviorComponent : public behaviormodule::components::BehaviorComponent {
 public:
  constexpr static const char* NAME = "TriplaneLoaderBehaviorComponent";
  std::string Name() const override { return NAME; };

  TriplaneLoaderBehaviorComponent();
  ~TriplaneLoaderBehaviorComponent();

  nv3dvc::core::Error OnUpdate(float dt) override;
  nv3dvc::core::Error OnInitialize() override;

 public:
  /// @ingroup TriplaneLoaderBehaviorComponentProperties
  /// @{
  core::properties::Property<std::string> image_file_path = {
      this,
      "image_file_path",
      "Path to image file of encoded 10x10 triplane. Should be single channel of type U8.",
      "",
  };
  core::properties::Property<std::string> mins_file_path = {
      this,
      "mins_file_path",
      "Path to quantization mins stored as a text file of comma separated values.",
      "",
  };
  core::properties::Property<std::string> maxs_file_path = {
      this,
      "maxs_file_path",
      "Path to quantization maxs stored as a text file of comma separated values.",
      "",
  };
  /// @}

 private:
  cv::Mat m_cvImage;  // OpenCV buffer where internal memory will be stored after reading file
};

}  // namespace components
}  // namespace triplanemodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_TRIPLANEMODULE_COMPONENTS_TRIPLANELOADERBEHAVIORCOMPONENT_H_
