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

#include "TrackingModule.h"

namespace nv3dvc {
namespace modules {
namespace trackingmodule {

TrackingModule::TrackingModule(core::engine::Engine* engine) : m_engine(engine) {
  RegisterSystem<systems::HeadTrackingSystem>(m_engine);
  RegisterComponent<components::TrackedHeadComponent>();
}

nv3dvc::core::Error TrackingModule::Initialize() { return nv3dvc::core::SUCCESS; }

nv3dvc::core::Error TrackingModule::Uninitialize() { return nv3dvc::core::SUCCESS; }

nv3dvc::core::Error TrackingModule::Update(float dt) { return nv3dvc::core::SUCCESS; }

}  // namespace trackingmodule
}  // namespace modules
}  // namespace nv3dvc
