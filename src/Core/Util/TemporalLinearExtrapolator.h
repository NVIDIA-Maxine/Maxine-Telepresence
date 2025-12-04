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

#ifndef SRC_CORE_UTIL_TEMPORALLINEAREXTRAPOLATOR_H_
#define SRC_CORE_UTIL_TEMPORALLINEAREXTRAPOLATOR_H_

namespace nv3dvc {
namespace core {
namespace util {
namespace filter {

/// @brief Templated extrapolator class for linearly extrapolating over time
///
/// The extrapolator makes use of the two latest observations to simulate future observations.
/// For usage, the extrapolate function needs to be implemented. A simple example using linear interpolation of
/// floating point values follows.
///   @code
///   // General case needs to be implemented for type T
///   T extrapolate(T a, T b, float alpha);
///
///   // Sample implementation for float
///   float extrapolate(float a, float b, float alpha) {
///     return a * (1 - (alpha + 1)) + b * (alpha + 1);
///   }
///   @endcode
/// @tparam T The type of the object to extrapolate.
template <typename T>
class TemporalLinearExtrapolator {
 public:
  explicit TemporalLinearExtrapolator(T default_val)
      : m_isFirst(true),
        m_prevVal(default_val),
        m_lastVal(default_val),
        m_timeSincePrevObservationMs(0.0f),
        m_timeSinceLastObservationMs(0.0f) {}
  ~TemporalLinearExtrapolator() = default;

  /// @brief Observe a new ground truth value
  /// @param val The value to observe
  void Observe(const T& val) {
    m_prevVal = m_isFirst ? val : m_lastVal;
    m_lastVal = val;
    m_timeSincePrevObservationMs = m_timeSinceLastObservationMs;
    m_timeSinceLastObservationMs = 0;
    m_isFirst = false;
  }

  /// @brief Extrapolate dt seconds into the future based on previous observations and previous extrapolations
  ///
  /// This function is expected to run for every update step. Calling this function extrapolates the value dt seconds
  /// from the previous extrapolation (or the previous observation, if Extrapolate has not been called since the last
  /// observation).
  /// This function accumulates time differentials (dt), meaning the dt argument passed does not have to increase
  /// monitonically when Extrapolate is called multiple times between observations.
  /// @param dt Delta time [seconds]. The time differential to extrapolate into the future
  /// @return The extrapolated value
  T Extrapolate(float dt) {
    m_timeSinceLastObservationMs += dt;
    const float extrapolation_factor =
        m_timeSincePrevObservationMs == 0.0f ? 0.0f : m_timeSinceLastObservationMs / m_timeSincePrevObservationMs;
    const T val = filter::extrapolate(m_prevVal, m_lastVal, extrapolation_factor);
    return val;
  }

  /// @brief Get the last set value
  /// @return the last set value
  T GetLastVal() const { return m_lastVal; }

  /// @brief Any observation has been made
  /// @return true if Observe has been called at least once, otherwise false
  bool HadObservation() const { return !m_isFirst; }

 private:
  bool m_isFirst;
  T m_prevVal;
  T m_lastVal;
  float m_timeSincePrevObservationMs;
  float m_timeSinceLastObservationMs;
};

}  // namespace filter
}  // namespace util
}  // namespace core
}  // namespace nv3dvc

#endif  // SRC_CORE_UTIL_TEMPORALLINEAREXTRAPOLATOR_H_
