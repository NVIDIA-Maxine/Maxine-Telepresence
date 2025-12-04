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

#ifndef SRC_MODULES_COMMONMODULE_COMPONENTS_TRANSFORMCOMPONENT_H_
#define SRC_MODULES_COMMONMODULE_COMPONENTS_TRANSFORMCOMPONENT_H_

#include <mutex>  // NOLINT(build/c++11) (cpplint outside Chromium: https://github.com/google/styleguide/issues/483)
#include <string>

#include "Core/EntityComponentSystem/Component.h"
#include "Core/Properties/Property.h"
#include "Core/Util/OneEuroFilter.h"
#include "Core/Util/TemporalLinearExtrapolator.h"
#include "Modules/CommonModule/OneEuroFilterProperties.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

namespace nv3dvc {
namespace modules {
namespace commonmodule {
namespace components {

/// @defgroup TransformComponentProperties TransformComponent
/// @ingroup  ComponentProperties
/// @brief    Matrix transform component corresponding to rigid transform plus non-uniform scaling

/// See @ref TransformComponentProperties
class TransformComponent : public core::ecs::Component {
 public:
  constexpr static const char* NAME = "TransformComponent";
  std::string Name() const override { return NAME; };

  TransformComponent();

  /// @brief Get the full transform matrix
  ///
  /// The order of operation is scale, rotation, translation, in other words, the matrix is made up using sub matrices
  /// TranslationMatrix * RotationMatrix * ScaleMatrix
  /// @return The full transform matrix
  glm::mat4 GetLocalMatrix();

  /// @brief Get the full transform matrix temporally extrapolated and filtered
  ///
  /// The order of operation is scale, rotation, translation, in other words, the matrix is made up using sub matrices
  /// TranslationMatrix * RotationMatrix * ScaleMatrix
  /// @return The full transform matrix
  glm::mat4 GetLocalMatrixFiltered();

  /// @brief Get the rotation, translation (rigid) transform matrix
  ///
  /// The order of operation is rotation, translation, in other words, the matrix is made up using sub matrices
  /// TranslationMatrix * RotationMatrix
  /// @return The rigid transform matrix
  glm::mat4 GetRt();

  /// @brief Get the rotation, translation (rigid) transform matrix temporally extrapolated and filtered
  ///
  /// The order of operation is rotation, translation, in other words, the matrix is made up using sub matrices
  /// TranslationMatrix * RotationMatrix
  /// @return The rigid transform matrix
  glm::mat4 GetRtFiltered();

  /// @brief Thread safe getter of transform components
  /// @param[out] center      the center of rotation
  /// @param[out] translation the translation component
  /// @param[out] rotation    the rotation component
  /// @param[out] scale       the scale component
  void GetTransform(glm::vec3* center, glm::vec3* translation, glm::quat* rotation, glm::vec3* scale);

  /// @brief Thread safe getter of rotation matrix
  /// @return the rotation matrix
  glm::fmat4 GetRotationMatrix();

  /// @brief Thread safe getter of translation matrix
  /// @return the translation matrix
  glm::fmat4 GetTranslationMatrix();

  /// @brief Thread safe getter of scale matrix
  /// @return the scale matrix
  glm::fmat4 GetScaleMatrix();

  /// @brief Thread safe getter of center
  /// @return the center of rotation
  glm::vec3 GetCenter();

  /// @brief Thread safe getter of translation
  /// @return the translation component
  glm::vec3 GetTranslation();

  /// @brief Thread safe getter of rotation
  /// @return the rotation component
  glm::quat GetRotation();

  /// @brief Thread safe getter of scale
  /// @return the scale component
  glm::vec3 GetScale();

  /// @brief Getter of filtered translation
  /// @return the filtered translation component
  glm::vec3 GetTranslationFiltered() const;

  /// @brief Getter of filtered rotation
  /// @return the filtered rotation component
  glm::quat GetRotationFiltered() const;

  /// @brief Thread safe setter of center
  /// @param center the center of rotation
  void SetCenter(const glm::vec3& center);

  /// @brief Thread safe setter of translation
  /// @param translation the translation component
  void SetTranslation(const glm::vec3& translation);

  /// @brief Thread safe setter of rotation
  /// @param rotation the rotation component
  void SetRotation(const glm::quat& rotation);

  /// @brief Thread safe setter of scale
  /// @param scale the scale component
  void SetScale(const glm::vec3& scale);

  /// @brief Update filtered value based on both temporal extrapolation and filtering
  /// @param dt Delta time [seconds]
  void Update(float dt);

 public:
  /// @ingroup TransformComponentProperties
  /// @{
  core::properties::Property<glm::vec3> center = {
      this,
      "center",
      "The center of rotation and scaling [meters]",
      glm::vec3(0.0f, 0.0f, 0.0f),
  };
  core::properties::Property<glm::vec3> translation = {
      this,
      "translation",
      "Translation [meters]",
      glm::vec3(0.0f, 0.0f, 0.0f),
  };
  core::properties::Property<glm::quat> rotation = {
      this,
      "rotation",
      "Rotation quaternion",
      glm::identity<glm::quat>(),
  };
  core::properties::Property<glm::vec3> scale = {
      this,
      "scale",
      "Scaling factors in x, y, and z dimensions respectively",
      glm::vec3(1.0f, 1.0f, 1.0f),
  };
  core::properties::Property<bool> relative = {
      this,
      "relative",
      "Whether this transform is relative to its parent",
      true,
  };
  core::properties::Property<bool> filter_pose = {
      this,
      "filter_pose",
      "Whether the pose should be temporally filtered",
      true,
  };
  core::properties::Property<bool> extrapolate_pose = {
      this,
      "extrapolate_pose",
      "Whether the pose should be extrapolated linearly in time",
      false,
  };
  OneEuroFilterProperties translation_one_euro_filter_params = {
      this, "translation_one_euro_filter_params", 2.0f, 10.0f, 2.0f,
  };
  OneEuroFilterProperties rotation_one_euro_filter_params = {
      this, "rotation_one_euro_filter_params", 2.0f, 10.0f, 2.0f,
  };
  /// @}

 private:
  bool m_isTemporal = false;
  glm::vec3 m_translationFiltered = glm::vec3(0.0f, 0.0f, 0.0f);
  glm::quat m_rotationFiltered = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
  core::util::filter::OneEuroFilter<glm::vec3> m_translationFilter;
  core::util::filter::OneEuroFilter<glm::quat> m_rotationFilter;
  core::util::filter::TemporalLinearExtrapolator<glm::vec3> m_translationExtrapolator{glm::vec3(0.0f, 0.0f, 0.0f)};
  core::util::filter::TemporalLinearExtrapolator<glm::quat> m_rotationExtrapolator{glm::quat(0.0f, 0.0f, 0.0f, 1.0f)};
  std::mutex m_dataAccessMutex;
};

}  // namespace components
}  // namespace commonmodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_COMMONMODULE_COMPONENTS_TRANSFORMCOMPONENT_H_
