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

#include "AudioUtils.h"

#include <algorithm>
#include <glm/glm.hpp>

#include "Core/Util/Logger.h"

namespace nv3dvc {
namespace modules {
namespace audiomodule {
namespace utils {

core::Error TransferAudio(const float* src_data, int src_channels, int num_samples, float* dst_data, int dst_channels) {
  // We assume channels can be 1 or 2.
  if (src_channels != 1 && src_channels != 2) {
    LOG_ERROR("Unsupported number of source channels: %d", src_channels);
    return core::ERR_UNIMPLEMENTED;
  }
  if (dst_channels != 1 && dst_channels != 2) {
    LOG_ERROR("Unsupported number of destination channels: %d", dst_channels);
    return core::ERR_UNIMPLEMENTED;
  }

  if (src_channels == dst_channels) {
    std::copy(src_data, src_data + src_channels * num_samples, dst_data);
  } else if (src_channels == 1) {
    // Duplicate source audio into both destination channels.
    glm::vec2* dst_vec_data = reinterpret_cast<glm::vec2*>(dst_data);
    std::transform(src_data, src_data + num_samples, dst_vec_data, [](float src) { return glm::vec2(src); });
  } else if (dst_channels == 1) {
    // Mix source audio channels into destination.
    const glm::vec2* src_vec_data = reinterpret_cast<const glm::vec2*>(src_data);
    std::transform(src_vec_data, src_vec_data + num_samples, dst_data,
                   [](const glm::vec2& src) { return 0.5f * (src.x + src.y); });
  }

  return core::SUCCESS;
}

}  // namespace utils
}  // namespace audiomodule
}  // namespace modules
}  // namespace nv3dvc
