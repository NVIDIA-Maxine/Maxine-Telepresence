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

#ifndef SRC_CORE_UTIL_TIMEABLE_H_
#define SRC_CORE_UTIL_TIMEABLE_H_

#include <atomic>

namespace nv3dvc {
namespace core {
namespace util {

/// @brief A base class used to associate updates with a timestamp
class Timeable {
 public:
  Timeable() = default;
  ~Timeable() = default;

  /// @brief Set the timestamp of the object used to associate its updates
  /// @param[in] timestamp The timestamp
  void SetTimeStamp(int64_t timestamp);

  /// @brief Get the timestamp of the object
  /// @return The timestamp
  int64_t GetTimeStamp() const;

 private:
  std::atomic<int64_t> m_timeStamp = 0ll;
};

}  // namespace util
}  // namespace core
}  // namespace nv3dvc

#endif  // SRC_CORE_UTIL_TIMEABLE_H_
