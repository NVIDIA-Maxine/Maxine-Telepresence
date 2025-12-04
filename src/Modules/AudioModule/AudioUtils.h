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

#ifndef SRC_MODULES_AUDIOMODULE_AUDIOUTILS_H_
#define SRC_MODULES_AUDIOMODULE_AUDIOUTILS_H_

#include <portaudio.h>

#include "Core/Error.h"

namespace nv3dvc {
namespace modules {
namespace audiomodule {

#ifdef WIN32
static constexpr PaHostApiTypeId kDefaultAudioAPI = PaHostApiTypeId::paWASAPI;
#else
static constexpr PaHostApiTypeId kDefaultAudioAPI = PaHostApiTypeId::paALSA;
#endif

namespace utils {

core::Error TransferAudio(const float* src_data, int src_channels, int num_samples, float* dst_data, int dst_channels);

}  // namespace utils
}  // namespace audiomodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_AUDIOMODULE_AUDIOUTILS_H_
