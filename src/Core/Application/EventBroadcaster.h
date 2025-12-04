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

#ifndef SRC_CORE_APPLICATION_EVENTBROADCASTER_H_
#define SRC_CORE_APPLICATION_EVENTBROADCASTER_H_

#include <functional>

#include "Core/Events/Event.h"

namespace nv3dvc {
namespace core {
namespace application {

/// @brief Simple wrapper around an event callback function. This allows this type to be cast to a void pointer for use
/// as custom data in callback functions for external libraries.
struct EventBroadcaster {
  std::function<void(events::Event*)> event_callback;
};

}  // namespace application
}  // namespace core
}  // namespace nv3dvc

#endif  // SRC_CORE_APPLICATION_EVENTBROADCASTER_H_
