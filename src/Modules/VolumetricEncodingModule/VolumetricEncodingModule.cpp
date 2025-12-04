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

#include "VolumetricEncodingModule.h"

namespace nv3dvc {
namespace modules {
namespace volumetricencodingmodule {

VolumetricEncodingModule::VolumetricEncodingModule() {
  RegisterComponent<components::EncodedTriplaneComponent>();
  RegisterSystem<systems::TriplaneEncoderSystem>();
}

nv3dvc::core::Error VolumetricEncodingModule::Initialize() { return nv3dvc::core::SUCCESS; }

nv3dvc::core::Error VolumetricEncodingModule::Uninitialize() { return nv3dvc::core::SUCCESS; }

nv3dvc::core::Error VolumetricEncodingModule::Update(float dt) { return nv3dvc::core::SUCCESS; }

}  // namespace volumetricencodingmodule
}  // namespace modules
}  // namespace nv3dvc
