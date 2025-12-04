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

#include "CubeMapComponent.h"

#include "Core/Engine/Engine.h"

namespace nv3dvc {
namespace modules {
namespace rendermodule {
namespace components {

core::Error CubeMapComponent::Load() { return m_cubeMapModel.Load(skybox_directory); }

core::rendering::CubeMapModel* CubeMapComponent::GetCubeMapModelPtr() { return &m_cubeMapModel; }

}  // namespace components
}  // namespace rendermodule
}  // namespace modules
}  // namespace nv3dvc
