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

#ifndef SRC_API_REFRAMING_REFRAMINGDEFS_H_
#define SRC_API_REFRAMING_REFRAMINGDEFS_H_

#define NV3DVC_REFRAMING_SUBJECT_SPECTATOR 0      // Subject
#define NV3DVC_REFRAMING_SUBJECT_PARTICIPANT_1 1  // Subject

#include <cstdint>

namespace nv3dvc {
namespace api {
namespace reframing {

/// @brief Scene preset for adding subjects to scene
enum class ScenePresetV1 {
  NONE = 0,                     /// < No scene preset
  STATICREFRAMING = 1,          /// < Subject remains fixed. Its transform is not tracked
  VIDEOCONFERENCING = 2,        /// < Subjects transform is tracked
  DEFAULT = VIDEOCONFERENCING,  /// < Default preset
};

typedef ScenePresetV1 ScenePreset;

/// @brief Initialization parameters version 1
struct InitParamsV1 {
  uint32_t version;
  int32_t multithreading;  /// < 0: off, 1: on
};

typedef InitParamsV1 InitParams;

/// @brief Entity selector version 1
///
/// Use to select a specific entity associated with a subject
enum class EntitySelectorV1 {
  ROOT,
  DISPLAY,
  WEBCAM,
  VIDEO_OUT,
  TRIPLANE,
};

typedef EntitySelectorV1 EntitySelector;

}  // namespace reframing
}  // namespace api
}  // namespace nv3dvc

#endif  // SRC_API_REFRAMING_REFRAMINGDEFS_H_
