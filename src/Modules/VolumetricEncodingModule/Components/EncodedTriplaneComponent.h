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

#ifndef SRC_MODULES_VOLUMETRICENCODINGMODULE_COMPONENTS_ENCODEDTRIPLANECOMPONENT_H_
#define SRC_MODULES_VOLUMETRICENCODINGMODULE_COMPONENTS_ENCODEDTRIPLANECOMPONENT_H_

#include <string>

#include "Core/EntityComponentSystem/Component.h"

namespace nv3dvc {
namespace modules {
namespace volumetricencodingmodule {
namespace components {

/// @defgroup EncodedTriplaneComponentProperties EncodedTriplaneComponent
/// @ingroup ComponentProperties
/// @brief Empty component used to signal to the systems::TriplaneEncoderSystem that it should process video frames and
/// push them to the triplane buffer

/// See @ref EncodedTriplaneComponentProperties
class EncodedTriplaneComponent : public core::ecs::Component {
 public:
  constexpr static const char* NAME = "EncodedTriplaneComponent";
  std::string Name() const override { return NAME; };

  float GetHeadScale() const { return head_scale; }

 public:
  /// @ingroup EncodedTriplaneComponentProperties
  /// @{
  core::properties::Property<float> head_scale = {
      this,
      "head_scale",
      "Scaling factor applied to volume object during rendering in order to bring it to units of meters",
      0.55f,
  };
  core::properties::Property<bool> fix_translation = {
      this,
      "fix_translation",
      "Whether translation should be fixed in the resulting triplane package from TriplaneEncoderSystem",
      false,
  };
  core::properties::Property<bool> fix_rotation = {
      this,
      "fix_rotation",
      "Whether rotation should be fixed in the resulting triplane package from TriplaneEncoderSystem",
      false,
  };
  /// @}
};

}  // namespace components
}  // namespace volumetricencodingmodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_VOLUMETRICENCODINGMODULE_COMPONENTS_ENCODEDTRIPLANECOMPONENT_H_
