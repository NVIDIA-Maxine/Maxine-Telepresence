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

#ifndef SRC_CORE_PROPERTIES_PROPERTY_H_
#define SRC_CORE_PROPERTIES_PROPERTY_H_

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Core/Util/TypeMap.h"

namespace nv3dvc {
namespace core {
namespace properties {

// Forward declaration
class PropertyOwner;

/// @brief Holds a value of a certain type
///
/// This wrapper class around property values allows object properties to be seamlessly added to property owners. A
/// property owner should register each property using AddProperty. See PropertyOwner
/// @tparam T The type of the value
template <class T>
class Property {
 public:
  /// @brief Disallow default constructor. Should not be able to create property without a name
  Property() = delete;

  /// @brief Constructor
  /// @param[in] name        The name of the property
  /// @param[in] description The description of the property
  Property(const char* name, const char* description) : m_name(name), m_description(description), m_value({}) {}

  /// @brief Constructor
  /// @tparam    ...Args Template arguments for constructing the value of type T
  /// @param[in] name        The name of the property
  /// @param[in] description The description of the property
  /// @param[in] ...args     Forward arguments for constructing the value of type T
  template <typename... Args>
  Property(const char* name, const char* description, Args&&... args)
      : m_name(name), m_description(description), m_value(std::forward<Args>(args)...) {}

  /// @brief Constructor
  /// @param[in] name        The name of the property
  /// @param[in] description The description of the property
  /// @param[in] v           The value of the property
  Property(const char* name, const char* description, const T& v)
      : m_name(name), m_description(description), m_value(v) {}

  /// @brief Constructor allowing for implicit adding of property to property owner
  ///
  /// With this constructor, AddProperty should not be called explicitly on the property owner
  /// @tparam        ...Args     Template arguments for constructing the value of type T
  /// @param[in,out] owner       The property owner to which this property will be added implicitly
  /// @param[in]     name        The name of the property
  /// @param[in]     description The description of the property
  /// @param[in]     ...args     Forward arguments for constructing the value of type T
  Property(PropertyOwner* owner, const char* name, const char* description);

  /// @brief Constructor allowing for implicit adding of property to property owner
  ///
  /// With this constructor, AddProperty should not be called explicitly on the property owner
  /// @tparam        ...Args     Template arguments for constructing the value of type T
  /// @param[in,out] owner       The property owner to which this property will be added implicitly
  /// @param[in]     name        The name of the property
  /// @param[in]     description The description of the property
  /// @param[in]     ...args     Forward arguments for constructing the value of type T
  template <typename... Args>
  Property(PropertyOwner* owner, const char* name, const char* description, Args&&... args);

  /// @brief Constructor allowing for implicit adding of property to property owner
  ///
  /// With this constructor, AddProperty should not be called explicitly on the property owner
  /// @param[in,out] owner       The property owner to which this property will be added implicitly
  /// @param[in]     name        The name of the property
  /// @param[in]     description The description of the property
  /// @param[in]     v           The value of the property
  Property(PropertyOwner* owner, const char* name, const char* description, const T& v);

  /// @brief Destructor
  ~Property() = default;

  /// @brief Copy constructor
  /// @param[in] other Other property
  Property(const Property& other)
      : m_name(other.m_name), m_value(other.m_value), m_onChangeFunction(other.m_onChangeFunction) {}

  /// @brief Move constructor
  /// @param other Other property
  Property(Property&& other) noexcept
      : m_name(std::move(other.m_name)),
        m_value(std::move(other.m_value)),
        m_onChangeFunction(std::move(other.m_onChangeFunction)) {}

  Property<T>& operator=(const Property<T>& other) = delete;
  Property<T>& operator=(Property<T>&& other) = delete;

  /// @brief Set the on-change-function
  /// @param on_change_function The function that will be called when the value changes
  void SetOnChangeFunction(const std::function<void()>& on_change_function) { m_onChangeFunction = on_change_function; }

  /// @brief Call the on change function
  void OnChange() {
    if (m_onChangeFunction) {
      m_onChangeFunction();
    }
  }

  /// @brief Get a pointer to the value.
  ///
  /// Note that if the value changes on variable received by this function, the on-change-function will not be called
  /// @return A pointer to the value
  T* get() { return &m_value; }

  /// @brief Get a pointer to the value.
  ///
  /// Note that if the value changes on variable received by this function, the on-change-function will not be called
  /// @return A pointer to the value
  const T* get() const { return &m_value; }

  /// @brief Get the name of the property
  /// @return The name of the property
  const char* name() { return m_name; }

  /// @brief Get the description of the property
  /// @return The description of the property
  const char* description() { return m_description; }

  operator T&() { return m_value; }
  operator const T&() const { return m_value; }
  T operator+(const T& value) { return m_value + value; }
  T operator-(const T& value) { return m_value - value; }
  T operator*(const T& value) { return m_value * value; }
  Property<T>& operator+=(const T& value) {
    m_value += value;
    if (m_onChangeFunction) m_onChangeFunction();
    return *this;
  }
  Property<T>& operator*=(const T& value) {
    m_value *= value;
    if (m_onChangeFunction) m_onChangeFunction();
    return *this;
  }
  Property<T>& operator=(const T& value) {
    m_value = value;
    if (m_onChangeFunction) m_onChangeFunction();
    return *this;
  }
  Property<T>& operator=(T&& value) {
    m_value = std::move(value);
    if (m_onChangeFunction) m_onChangeFunction();
    return *this;
  }

 private:
  const char* m_name;
  const char* m_description = nullptr;
  T m_value;
  std::function<void()> m_onChangeFunction;
};

/// @brief Allows for a map of property containers without specifying the type.
///
/// Each typed container based will be down cast from this
struct PropertyContainerBase {
  virtual ~PropertyContainerBase() = default;
};

/// @brief Typed property container
/// @tparam T The type of the property stored in this container
template <class T>
struct PropertyContainer : public PropertyContainerBase {
  std::vector<Property<T>*> properties;
};

/// @brief Owner of properties
///
/// Extend this class with all owners of properties. This allows for structured retrieving of properties of all types.
/// All properties should be registered in the constructor of the property owner, by calling AddProperty for all
/// properties.
class PropertyOwner {
  constexpr static const char* NAME = "PropertyOwner";  // Required for implicit component registration

 public:
  PropertyOwner()
      : m_propertyContainers(std::make_unique<nv3dvc::core::util::TypeMap<std::unique_ptr<PropertyContainerBase>>>()) {}

  PropertyOwner(const PropertyOwner& other) = delete;
  PropertyOwner(PropertyOwner&& other) = delete;
  PropertyOwner& operator=(const PropertyOwner& other) = delete;
  PropertyOwner& operator=(PropertyOwner&& other) = delete;
  virtual ~PropertyOwner() {}

  /// @brief Registers the property for this property owner
  /// @tparam T The type of the property
  /// @param prop A pointer to the property to be registered
  template <typename T>
  void AddProperty(Property<T>* prop);

  /// @brief Get a list of all properties of the given type
  /// @tparam T The type of the properties to restore
  /// @return A vector of all properties of the given type, registered for this property owner
  template <typename T>
  std::vector<Property<T>*> GetProperties() const;

  virtual std::string Name() const { return NAME; }

  /// @brief Add a property subowner
  ///
  /// The added property owner must have a unique name
  /// @param[in] property_owner The property subowner to add
  void AddSubOwner(PropertyOwner* property_owner) { m_subOwners.insert({property_owner->Name(), property_owner}); }

  /// @brief Remove a property subowner
  /// @param property_owner The property subowner to remove
  void RemoveSubOwner(PropertyOwner* property_owner) { m_subOwners.erase(property_owner->Name()); }

  /// @brief Get all subowners registered for this property owner
  /// @return A map of all property owners, keyed by name
  const std::unordered_map<std::string, PropertyOwner*> SubOwners() const { return m_subOwners; }

 private:
  std::unique_ptr<nv3dvc::core::util::TypeMap<std::unique_ptr<PropertyContainerBase>>> m_propertyContainers;
  std::unordered_map<std::string, PropertyOwner*> m_subOwners;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Template function definitions                                                                                      //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
void PropertyOwner::AddProperty(Property<T>* prop) {
  auto it = m_propertyContainers->find<T>();
  if (it == m_propertyContainers->end()) {
    std::unique_ptr<PropertyContainer<T>> container = std::make_unique<PropertyContainer<T>>();
    container->properties.push_back(prop);
    m_propertyContainers->put<T>(std::move(container));
  } else {
    static_cast<PropertyContainer<T>*>(it->second.get())->properties.push_back(prop);
  }
}

template <typename T>
std::vector<Property<T>*> PropertyOwner::GetProperties() const {
  auto it = m_propertyContainers->find<T>();
  if (it == m_propertyContainers->end()) {
    return std::vector<Property<T>*>();
  } else {
    return static_cast<PropertyContainer<T>*>(it->second.get())->properties;
  }
}

template <class T>
Property<T>::Property(PropertyOwner* owner, const char* name, const char* description)
    : m_name(name), m_description(description), m_value({}) {
  owner->AddProperty(this);
}

template <class T>
template <typename... Args>
Property<T>::Property(PropertyOwner* owner, const char* name, const char* description, Args&&... args)
    : m_name(name), m_description(description), m_value(std::forward<Args>(args)...) {
  owner->AddProperty(this);
}

template <class T>
Property<T>::Property(PropertyOwner* owner, const char* name, const char* description, const T& v)
    : m_name(name), m_description(description), m_value(v) {
  owner->AddProperty(this);
}

}  // namespace properties
}  // namespace core
}  // namespace nv3dvc

#endif  // SRC_CORE_PROPERTIES_PROPERTY_H_
