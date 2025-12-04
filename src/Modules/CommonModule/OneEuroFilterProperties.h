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

#ifndef SRC_MODULES_COMMONMODULE_ONEEUROFILTERPROPERTIES_H_
#define SRC_MODULES_COMMONMODULE_ONEEUROFILTERPROPERTIES_H_

#include <string>

#include "Core/Properties/Property.h"

namespace nv3dvc {
namespace modules {
namespace commonmodule {

/// @defgroup OneEuroFilterPropertiesProperties OneEuroFilterProperties
/// @ingroup  ComponentProperties
/// @brief    Parameters for one euro filter
///
/// See core::util::filter::OneEuroFilter
///

/// See @ref OneEuroFilterPropertiesProperties

class OneEuroFilterProperties : public core::properties::PropertyOwner {
 public:
  std::string Name() const override { return m_name; };

  /// @brief Constructor
  /// @param[in,out] owner             The parent property owner for these properties
  /// @param[in]     name              The name of this property owner
  /// @param[in]     min_cutoff_freq   Minimum cutoff frequency [Hz]
  /// @param[in]     cutoff_slope      Cutoff slope
  /// @param[in]     deriv_cutoff_freq Derivative's cutoff frequency [Hz]
  OneEuroFilterProperties(PropertyOwner* owner, const std::string& name, float init_min_cutoff_freq,
                          float init_cutoff_slope, float init_deriv_cutoff_freq);
  ~OneEuroFilterProperties() override = default;

 public:
  /// @ingroup OneEuroFilterPropertiesProperties
  /// @{
  core::properties::Property<float> min_cutoff_freq = {
      this,
      "min_cutoff_freq",
      "Minimum cutoff frequency [Hz]",
      1.0f,
  };
  core::properties::Property<float> cutoff_slope = {
      this,
      "cutoff_slope",
      "Cutoff slope",
      0.0f,
  };
  core::properties::Property<float> deriv_cutoff_freq = {
      this,
      "deriv_cutoff_freq",
      "Derivative's cutoff frequency [Hz]",
      1.0f,
  };
  /// @}

 private:
  const std::string m_name;
};

}  // namespace commonmodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_COMMONMODULE_ONEEUROFILTERPROPERTIES_H_
