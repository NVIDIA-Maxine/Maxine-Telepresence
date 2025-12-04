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

#include "CallbackComponent.h"

#include <mutex>  // NOLINT(build/c++11) (cpplint outside Chromium: https://github.com/google/styleguide/issues/483)

namespace nv3dvc {
namespace modules {
namespace commonmodule {
namespace components {

void CallbackComponent::SetOnFiredCallback(OnFiredCallback callback) {
  std::lock_guard lock{m_callbackMutex};
  m_onFired = callback;
}

bool CallbackComponent::HasOnFiredCallback() const {
  std::lock_guard lock{m_callbackMutex};
  return m_onFired != nullptr;
}

core::Error CallbackComponent::Fire(void* data_ptr, size_t data_size, size_t* processed_size, int data_type) {
  std::lock_guard lock{m_callbackMutex};
  if (!m_onFired) return core::Error::ERR_INITIALIZATION;
  return m_onFired(data_ptr, data_size, processed_size, data_type);
}

}  // namespace components
}  // namespace commonmodule
}  // namespace modules
}  // namespace nv3dvc
