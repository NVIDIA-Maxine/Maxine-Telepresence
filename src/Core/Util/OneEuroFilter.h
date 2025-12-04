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

#ifndef SRC_CORE_UTIL_ONEEUROFILTER_H_
#define SRC_CORE_UTIL_ONEEUROFILTER_H_

#include <cstdlib>
#include <type_traits>

#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"
#include "glm/gtc/quaternion.hpp"

namespace nv3dvc {
namespace core {
namespace util {

/// @brief Utilities for temporal filtering of observations and parameters.
namespace filter {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Filter function implementations for floating-point sample types.                                                 ///
/// To support a new sample type, implement these functions in the same namespace as the type.                       ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Estimate velocity from two sample values and the duration of time between them.
///
/// @tparam  T     The sample type
/// @param   prev  Starting sample value
/// @param   curr  Ending sample value
/// @param   dt    Duration of time (in seconds) between the two samples
/// @return  The velocity required to move from `prev` to `curr` in time `dt`, assuming constant velocity
template <typename T, std::enable_if_t<std::is_floating_point_v<T>, bool> = true>
inline T ComputeVelocity(const T& prev, const T& curr, float dt) {
  const float inv_dt = dt != 0.0f ? 1.0f / dt : 0.0f;
  return inv_dt * (curr - prev);
}

/// @brief Interpolate between two values.
///
/// @tparam  T      The type of values to interpolate
/// @param   x      The first value
/// @param   y      The second value
/// @param   alpha  The interpolation factor, in [0, 1]
/// @return  The interpolated value, which is equal to `x` when `alpha == 0`, `y` when `alpha == 1`, and interpolates
///          smoothly between them for other intermediate values of `alpha`.
template <typename T, std::enable_if_t<std::is_floating_point_v<T>, bool> = true>
inline T Interpolate(const T& x, const T& y, float alpha) {
  return x * (1.0f - alpha) + y * alpha;
}

/// @brief Compute the magnitude of a velocity value.
///
/// @tparam T  The velocity type
/// @param x   The velocity value
/// @return The magnitude of the velocity
template <typename T, std::enable_if_t<std::is_floating_point_v<T>, bool> = true>
inline float Magnitude(const T& x) {
  return static_cast<float>(std::abs(x));
}

/// @brief 1€ filter: a simple speed-based low-pass filter for noisy input in interactive systems
///
/// Authors: Géry Casiez, Nicolas Roussel, Daniel Vogel
/// CHI '12: Proceedings of the SIGCHI Conference on Human Factors in Computing Systems
/// May 2012 Pages 2527-2530
/// https://doi.org/10.1145/2207676.2208639
///
/// Tuning the filter
///   To minimize jitter and lag when tracking human motion, the two parameters (min_cutoff_freq and cutoff_slope) can
///   be set using a simple two-step procedure. First cutoff_slope is set to 0 and min_cutoff_freqto a reasonable
///   middle-ground value such as 1 Hz. Then the body part is held steady or moved at a very low speed while
///   min_cutoff_freq is adjusted to remove jitter and preserve an acceptable lag during these slow movements
///   (decreasing min_cutoff_freq reduces jitter but increases lag, min_cutoff_freq must be > 0). Next, the body part is
///   moved quickly in different directions while cutoff_slope is increased with a focus on minimizing lag. First find
///   the right order of magnitude to tune cutoff_slope, which depends on the kind of data you manipulate and their
///   units: do not hesitate to start with values like 0.001 or 0.0001. You can first multiply and divide cutoff_slope
///   by factor 10 until you notice an effect on latency when moving quickly. Note that parameters min_cutoff_freq and
///   cutoff_slope have clear conceptual relationships: if high speed lag is a problem, increase cutoff_slope; if slow
///   speed jitter is a problem, decrease min_cutoff_freq.
///
/// @tparam T The sample type to be filtered. In order to use the OneEuroFilter for your own sample type `T`, you must
/// overload the functions @ref ComputeVelocity, @ref Magnitude and @ref Interpolate in the same namespace as `T`.
template <typename T>
class OneEuroFilter {
 public:
  using VelocityType = decltype(ComputeVelocity(T{}, T{}, 0.0f));

  /// @brief Default constructor
  OneEuroFilter() { Reset(1.0f /* Hz */, 0.0f /* Hz/unit(VelocityType) */, 1.0f /* Hz */); }

  /// @brief Constructor
  ///
  /// @param min_cutoff_freq    the lowest bandwidth filter applied, in Hz.
  /// @param cutoff_slope       the rate at which the filter adapts: higher levels reduce lag, in Hz/unit(VelocityType).
  /// @param deriv_cutoff_freq  the bandwidth of the filter applied to smooth the derivative, in Hz.
  OneEuroFilter(float min_cutoff_freq, float cutoff_slope, float deriv_cutoff_freq) {
    Reset(min_cutoff_freq, cutoff_slope, deriv_cutoff_freq);
  }

  /// @brief Reset all parameters of the filter.
  ///
  /// @param min_cutoff_freq    the lowest bandwidth filter applied, in Hz.
  /// @param cutoff_slope       the rate at which the filter adapts: higher levels reduce lag, in Hz/unit(VelocityType).
  /// @param deriv_cutoff_freq  the bandwidth of the filter applied to smooth the derivative, in Hz.
  void Reset(float min_cutoff_freq, float cutoff_slope, float deriv_cutoff_freq) {
    Reset();
    m_minCutoff = min_cutoff_freq;
    m_beta = cutoff_slope;
    m_dCutoff = deriv_cutoff_freq;
    m_dxdtHatPrev = VelocityType{0};
  }

  /// @brief Reset only the initial condition of the filter, leaving parameters the same.
  void Reset() { m_firstTime = true; }

  /// @brief Apply the one euro filter to the given input.
  ///
  /// @param x  the noisy sample value.
  /// @param dt delta time (seconds) since last update. 1 / update rate
  /// @return   the filtered sample value.
  T Filter(T x, float dt) {
    if (m_firstTime) {
      m_xHatPrev = x;
      m_dxdtHatPrev = VelocityType{0};
      m_firstTime = false;
    }
    const VelocityType dxdt = ComputeVelocity(m_xHatPrev, x, dt);
    const VelocityType dxdt_hat = Interpolate(m_dxdtHatPrev, dxdt, Alpha(m_dCutoff, dt));
    const float cutoff = m_minCutoff + m_beta * Magnitude(dxdt_hat);
    const T x_hat = Interpolate(m_xHatPrev, x, Alpha(cutoff, dt));
    m_xHatPrev = x_hat;
    m_dxdtHatPrev = dxdt_hat;
    return x_hat;
  }

  /// @brief Get the current filtered estimate of velocity
  /// @return The velocity computed from the input samples
  VelocityType GetFilteredVelocity() const { return m_dxdtHatPrev; }

 private:
  static float Alpha(float cutoff, float dt) {
    // float tau = kOneOverTwoPi / cutoff, dt = 1.f / rate;
    // return 1.f / (1.f + tau / dt);
    const float scale = cutoff * dt;
    return scale / (glm::one_over_two_pi<float>() + scale);
  }

  bool m_firstTime;
  float m_minCutoff, m_dCutoff, m_beta;
  T m_xHatPrev = {};
  VelocityType m_dxdtHatPrev = {};
};

}  // namespace filter
}  // namespace util
}  // namespace core
}  // namespace nv3dvc

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Filter function implementations for GLM types                                                                    ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace glm {

/// @brief Implements nv3dvc::core::util::filter::ComputeVelocity for GLM types.
template <typename T>
inline auto ComputeVelocity(const T& prev, const T& curr, float dt) {
  const float inv_dt = dt != 0.0f ? 1.0f / dt : 0.0f;
  if constexpr (std::is_same_v<std::decay_t<T>, glm::quat>) {
    // Compute angular velocity from two normalized quaternions.
    // We seek a quaternion `delta` that can be composed with `prev` to reach `curr`, i.e. `prev * delta = curr`.
    const glm::quat delta = curr * glm::conjugate(prev);
    return inv_dt * glm::angle(delta) * glm::axis(delta);
  } else {
    return inv_dt * (curr - prev);
  }
}

/// @brief Implements nv3dvc::core::util::filter::Interpolate for GLM types.
template <typename T>
inline T Interpolate(const T& x, const T& y, float alpha) {
  if constexpr (std::is_same_v<std::decay_t<T>, glm::quat>) {
    return glm::normalize(glm::slerp(x, y, alpha));
  } else {
    return glm::mix(x, y, alpha);
  }
}

/// @brief Implements nv3dvc::core::util::filter::Magnitude for GLM types.
template <typename T>
inline float Magnitude(const T& x) {
  return glm::length(x);
}

}  // namespace glm

#endif  // SRC_CORE_UTIL_ONEEUROFILTER_H_
