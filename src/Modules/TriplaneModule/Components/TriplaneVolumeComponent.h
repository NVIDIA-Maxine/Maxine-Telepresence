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

#ifndef SRC_MODULES_TRIPLANEMODULE_COMPONENTS_TRIPLANEVOLUMECOMPONENT_H_
#define SRC_MODULES_TRIPLANEMODULE_COMPONENTS_TRIPLANEVOLUMECOMPONENT_H_

#include <string>

#include "Core/EntityComponentSystem/Component.h"
#include "nvCVTriplaneVolume.h"

namespace nv3dvc {
namespace modules {
namespace triplanemodule {
namespace components {

/// @defgroup TriplaneVolumeComponentProperties TriplaneVolumeComponent
/// @ingroup  ComponentProperties
/// @brief    Component for storing a static triplane volume object

/// See @ref TriplaneVolumeComponentProperties
class TriplaneVolumeComponent : public core::ecs::Component {
 public:
  constexpr static const char* NAME = "TriplaneVolumeComponent";
  std::string Name() const override { return NAME; };
  TriplaneVolumeComponent();

  NvCV_Status Allocate();

  NvCVTriplaneVolume* GetTriplaneVolumePtr();

 private:
  NvCVTriplaneVolume m_triplaneVolume;
};

}  // namespace components
}  // namespace triplanemodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_TRIPLANEMODULE_COMPONENTS_TRIPLANEVOLUMECOMPONENT_H_
