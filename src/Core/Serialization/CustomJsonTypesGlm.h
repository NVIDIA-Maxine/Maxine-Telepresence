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

#ifndef SRC_CORE_SERIALIZATION_CUSTOMJSONTYPESGLM_H_
#define SRC_CORE_SERIALIZATION_CUSTOMJSONTYPESGLM_H_

#include <vector>

#include "glm/detail/type_quat.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "nlohmann/json.hpp"

namespace glm {

/// @brief Serialization of column vector
/// @tparam T      Data type
/// @tparam Length Number of elements in the vector
/// @param j       The destination json object
/// @param p       The source vector
template <length_t Length, typename T>
void to_json(nlohmann::json& j,  // NOLINT(runtime/references) (owned by nlohmann::json)
             const vec<Length, T>& p) {
  j = std::vector<T>(p.length());
  for (size_t i = 0; i < p.length(); i++) {
    j[i] = p[i];
  }
}

/// @brief Deserialization of column vector
/// @tparam T      Data type
/// @tparam Length Number of elements in the vector
/// @param j       The source json object
/// @param p       The destination vector
template <length_t Length, typename T>
void from_json(const nlohmann::json& j,  //
               vec<Length, T>& p) {      // NOLINT(runtime/references) (owned by nlohmann::json)
  for (size_t i = 0; i < p.length(); i++) {
    p[i] = j[i];
  }
}

/// @brief Linear serialization. Since glm matrices are column major, serialization will be column by column
/// @tparam T    Data type
/// @tparam Rows Number of rows in the matrix
/// @tparam Cols Number of columns in the matrix
/// @param j     The destination json object
/// @param p     The source matrix
template <length_t Cols, length_t Rows, typename T>
void to_json(nlohmann::json& j,  // NOLINT(runtime/references) (owned by nlohmann::json)
             const mat<Cols, Rows, T>& p) {
  static constexpr uint32_t length = Cols * Rows;
  j = std::array<T, length>();
  nlohmann::json::iterator dst = j.begin();
  const T* src = glm::value_ptr(p);
  while (src != src + length && dst != j.end()) {
    *dst++ = *src++;
  }
}

/// @brief Linear deserialization. Since glm matrices are column major, serialization will be column by column
/// @tparam T    Data type
/// @tparam Rows Number of rows in the matrix
/// @tparam Cols Number of columns in the matrix
/// @param j     The source json object
/// @param p     The destination matrix
template <length_t Cols, length_t Rows, typename T>
void from_json(const nlohmann::json& j,  //
               mat<Cols, Rows, T>& p) {  // NOLINT(runtime/references) (owned by nlohmann::json)
  static constexpr uint32_t length = Cols * Rows;
  nlohmann::json::const_iterator src = j.begin();
  T* dst = glm::value_ptr(p);
  while (src != j.end() && dst != dst + length) {
    *dst++ = *src++;
  }
}

/// @brief Serialization of quaternion
/// @tparam T    Data type
/// @param j     The destination json object
/// @param p     The source quaternion
template <typename T>
void to_json(nlohmann::json& j,  // NOLINT(runtime/references) (owned by nlohmann::json)
             const qua<T>& p) {
  j = std::vector<T>(p.length());
  for (size_t i = 0; i < p.length(); i++) {
    j[i] = p[i];
  }
}

/// @brief Deserialization of quaternion
/// @tparam T      Data type
/// @param j       The source json object
/// @param p       The destination quaternion
template <typename T>
void from_json(const nlohmann::json& j,  //
               qua<T>& p) {              // NOLINT(runtime/references) (owned by nlohmann::json)
  for (size_t i = 0; i < p.length(); i++) {
    p[i] = j[i];
  }
}

}  // namespace glm

#endif  // SRC_CORE_SERIALIZATION_CUSTOMJSONTYPESGLM_H_
