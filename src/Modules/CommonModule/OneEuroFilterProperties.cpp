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

#include "OneEuroFilterProperties.h"

#include <string>

namespace nv3dvc {
namespace modules {
namespace commonmodule {

OneEuroFilterProperties::OneEuroFilterProperties(PropertyOwner* owner, const std::string& name,
                                                 const float init_min_cutoff_freq, const float init_cutoff_slope,
                                                 const float init_deriv_cutoff_freq)
    : m_name(name) {
  owner->AddSubOwner(this);
  min_cutoff_freq = init_min_cutoff_freq;
  cutoff_slope = init_cutoff_slope;
  deriv_cutoff_freq = init_deriv_cutoff_freq;
}

}  // namespace commonmodule
}  // namespace modules
}  // namespace nv3dvc
