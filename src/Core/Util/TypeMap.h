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

#ifndef SRC_CORE_UTIL_TYPEMAP_H_
#define SRC_CORE_UTIL_TYPEMAP_H_

#include <atomic>
#include <unordered_map>
#include <utility>

namespace nv3dvc {
namespace core {
namespace util {

/// @brief Maps types to values
///
/// Reference : https://gpfault.net/posts/mapping-types-to-values.txt.html
/// @tparam ValueType The type
template <class ValueType>
class TypeMap {
  // Internally, we'll use a hash table to store mapping from type IDs to the values.
  typedef std::unordered_map<int, ValueType> InternalMap;

 public:
  typedef typename InternalMap::iterator iterator;
  typedef typename InternalMap::const_iterator const_iterator;
  typedef typename InternalMap::value_type value_type;

  const_iterator begin() const { return m_map.begin(); }
  const_iterator end() const { return m_map.end(); }
  iterator begin() { return m_map.begin(); }
  iterator end() { return m_map.end(); }

  // Finds the value associated with the type "Key" in the type map.
  template <class Key>
  iterator find() {
    return m_map.find(GetTypeId<Key>());
  }

  // Same as above, const version
  template <class Key>
  const_iterator find() const {
    return m_map.find(GetTypeId<Key>());
  }

  // Associates a value with the type "Key"
  template <class Key>
  void put(ValueType&& value) {
    m_map[GetTypeId<Key>()] = std::forward<ValueType>(value);
  }

  /// @brief Update the type map by merging it with another type map of the same ValueType
  /// @param[in] other The other TypeMap to add/replace this TypeMap's values with
  void Merge(const TypeMap<ValueType>& other) {
    for (const auto& [key, value] : other.m_map) {
      m_map[key] = value;
    }
  }

  /// @brief Update the type map by merging it with another type map of the same ValueType
  ///
  /// This function only acts on type maps where the ValueType is itself a standard container, i.e. defines insert, end
  /// and begin functions for value iterators. Example usage may be where ValueType is specified as a
  /// std::vector<InternalValueType>.
  /// @code
  ///
  /// util::TypeMap<std::vector<int>> first_map;
  /// util::TypeMap<std::vector<int>> second_map;
  /// first_map.put<MyType1>(std::vector({1, 2, 3}));      // size == 3
  /// first_map.put<MyType2>(std::vector({1, 2}));         // size == 2
  /// second_map.put<MyType1>(std::vector({1, 2, 3, 4}));  // size == 4
  /// first_map.MergeWithConcatenation(second_map);
  /// const size_t the_size = (*first_map.find<MyType1>()).second.size();  // the_size == 7 (3 + 4)
  ///
  /// @endcode
  /// @param[in] other The other TypeMap to update this TypeMap's values with
  void MergeWithConcatenation(const TypeMap<ValueType>& other) {
    for (const auto& [key, value] : other.m_map) {
      if (m_map.count(key) != 0) {
        // Merge containers
        m_map[key].insert(m_map[key].end(), value.begin(), value.end());
      } else {
        // Add container
        m_map[key] = value;
      }
    }
  }

 private:
  InternalMap m_map;
  template <class Key>
  static int GetTypeId() {
    static const int id = m_lastTypeId++;
    return id;
  }

  static std::atomic_int m_lastTypeId;
};

template <class ValueType>
std::atomic_int TypeMap<ValueType>::m_lastTypeId(0);

}  // namespace util
}  // namespace core
}  // namespace nv3dvc

#endif  // SRC_CORE_UTIL_TYPEMAP_H_
