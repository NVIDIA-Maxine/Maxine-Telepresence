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

#ifndef SRC_MODULES_TRIPLANEMODULE_COMPONENTS_TRIPLANEBUFFERCOMPONENT_H_
#define SRC_MODULES_TRIPLANEMODULE_COMPONENTS_TRIPLANEBUFFERCOMPONENT_H_

#include <atomic>
#include <string>
#include <vector>

#include "Core/EntityComponentSystem/Component.h"
#include "Core/Serialization/CustomJsonTypesGlm.h"
#include "Core/Util/OneEuroFilter.h"
#include "Modules/TriplaneModule/TriplanePackage.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "nvCVTriplaneVolume.h"
#include "uuid.h"

namespace nv3dvc {
namespace modules {
namespace triplanemodule {
namespace components {

/// @defgroup TriplaneBufferComponentProperties TriplaneBufferComponent
/// @ingroup  ComponentProperties
/// @brief    Component used to represent a ring buffer of updatable triplane volume objects.
///
/// The component enables animatable triplane volume objects, where one thread writes to the rolling buffer, and another
/// one reads it. Typical usage involves having one system writing encoded triplane volume objects to the buffer, while
/// a renderer acts as a consumer and reads available triplane volume objects from the buffer.

/// See @ref TriplaneBufferComponentProperties
class TriplaneBufferComponent : public core::ecs::Component {
 public:
  constexpr static const char* NAME = "TriplaneBufferComponent";
  std::string Name() const override { return NAME; };

  TriplaneBufferComponent();

  void Initialize();

  /// @brief Get a pointer to a valid triplane frame if one exists
  /// @return A valid triplane frame or nullptr if none exist currently
  TriplaneFrame* GetReadableTriplaneFrame();

  /// @brief Signal that the readable triplane frame has been consumed
  void SignalConsume();

  /// @brief Get a pointer to a valid triplane frame for writing if one exists
  /// @return A valid triplane frame or nullptr if none exist currently
  TriplaneFrame* GetWriteableTriplaneFrame();

  /// @brief Signal that the writable triplane frame has been privided
  void SignalProvide();

  /// @brief Utility for filtering confidence value
  /// @param[in] confidence  The current confidence value to filter
  /// @param[in] dt          Delta time, time since last update [seconds]
  float FilterConfidence(float confidence, float dt);

 public:
  /// @ingroup TriplaneBufferComponentProperties
  /// @{
  core::properties::Property<int> ring_buffer_size = {
      this,
      "ring_buffer_size",
      "Size of the ring buffer containing triplane volume objects",
      5,
  };
  core::properties::Property<bool> fp32_precision = {
      this,
      "fp32_precision",
      "Whether to use floating point 32 precision in triplane volume buffers. If false, uint8 is used.",
      false,
  };
  /// @}

 private:
  std::vector<TriplaneFrame> m_ringBuffer;

  std::atomic<int> m_triplaneRingBufferReadIndex;
  std::atomic<int> m_triplaneRingBufferWriteIndex;
  core::util::filter::OneEuroFilter<float> m_confidenceFilter;
};

}  // namespace components
}  // namespace triplanemodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_TRIPLANEMODULE_COMPONENTS_TRIPLANEBUFFERCOMPONENT_H_
