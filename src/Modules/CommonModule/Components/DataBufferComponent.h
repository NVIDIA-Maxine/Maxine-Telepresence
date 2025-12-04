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

#ifndef SRC_MODULES_COMMONMODULE_COMPONENTS_DATABUFFERCOMPONENT_H_
#define SRC_MODULES_COMMONMODULE_COMPONENTS_DATABUFFERCOMPONENT_H_

#include <string>
#include <vector>

#include "Core/EntityComponentSystem/Component.h"

namespace nv3dvc {
namespace modules {
namespace commonmodule {
namespace components {

/// @brief Data buffer component base class
///
/// For delegation data handling using the general class, use DataBufferComponent. The base class can be
/// extended for particular usage to other components, which will share the base functionality of appending, and
/// consuming data buffers.
class _DataBufferComponent : public core::ecs::Component {
 public:
  /// @brief Append one data packet
  /// @param[in,out] packet The data packet which will be moved to the data buffer component
  void Append(std::vector<uint8_t>&& packet);

  /// @brief Append multiple data packets
  /// @param[in,out] packets The data packets which will be moved to the data buffer component
  void Append(std::vector<std::vector<uint8_t>>&& packets);

  /// @brief Get a reference to the data buffers of all packages
  /// @return A reference to the data buffers
  const std::vector<std::vector<uint8_t>>& GetBuffers();

  /// @brief Clears the data buffers
  void Consume();

 protected:
  _DataBufferComponent();

 private:
  std::vector<std::vector<uint8_t>> m_dataBuffers;
};

/// @defgroup DataBufferComponentProperties DataBufferComponent
/// @ingroup  ComponentProperties
/// @brief    General data buffer component
///
/// The purpose of the component may depend in which context it is used. A given system or behavior may access the data
/// buffer to read and / or write to it under certain conditions. For specifically delegated buffer types, prefer using
/// non-general extensions of the base class.

/// See @ref DataBufferComponentProperties
class DataBufferComponent : public _DataBufferComponent {
 public:
  constexpr static const char* NAME = "DataBufferComponent";
  std::string Name() const override { return NAME; };
};

/// @defgroup EncodedVideoBufferComponentProperties EncodedVideoBufferComponent
/// @ingroup  ComponentProperties
/// @brief    Data buffer specifically delegated to encoded video buffers
///
/// Encoder and decoder systems should expect this buffer to contain data packets of a known shared format

/// See @ref EncodedVideoBufferComponentProperties
class EncodedVideoBufferComponent : public _DataBufferComponent {
 public:
  constexpr static const char* NAME = "EncodedVideoBufferComponent";
  std::string Name() const override { return NAME; };
};

}  // namespace components
}  // namespace commonmodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_COMMONMODULE_COMPONENTS_DATABUFFERCOMPONENT_H_
