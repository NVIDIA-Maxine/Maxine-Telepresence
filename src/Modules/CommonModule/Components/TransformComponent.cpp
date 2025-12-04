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

#include "TransformComponent.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Static function declarations                                                                                     ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Compose a transform matrix using its components
/// @param[in] center        The center of rotation. Only used if rotation is provided
/// @param[in] translation   The translation component
/// @param[in] rotation      The rotation component
/// @param[in] offset_scale  Scaling of offset (center point). Only used if center and rotation are provided.
///                          Typically same as matrix_scale. Can be provided to generate a proper RT matrix even if
///                          matrix_scale is not provided.
/// @param[in] matrix_scale  Scaling of object. Typically same as offset_scale.
/// @return                  The composit matrix as T * R * S
static glm::mat4 ComposeTransformMatrix(glm::vec3* center, glm::vec3* translation, glm::quat* rotation,
                                        glm::vec3* offset_scale, glm::vec3* matrix_scale);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Static function definitions                                                                                      ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace nv3dvc {
namespace core {
namespace util {
namespace filter {
static glm::vec3 extrapolate(const glm::vec3& a, const glm::vec3& b, float alpha) { return glm::mix(a, b, alpha + 1); }

static glm::quat extrapolate(const glm::quat& a, const glm::quat& b, float alpha) {
  // This is a cheat as we can't extrapolate quaternions yet
  return b;
}

}  // namespace filter
}  // namespace util
}  // namespace core
}  // namespace nv3dvc

static glm::mat4 ComposeTransformMatrix(glm::vec3* center, glm::vec3* translation, glm::quat* rotation,
                                        glm::vec3* offset_scale, glm::vec3* matrix_scale) {
  glm::mat4 ret = glm::mat4(1.0f);
  if (translation) {
    ret = ret * glm::translate(glm::fmat4(1.0f), *translation);
  }
  if (rotation) {
    const glm::vec3 scaled_center =
        (center ? *center : glm::vec3(0.0f)) * (offset_scale ? *offset_scale : glm::vec3(1.0f));
    ret = ret * glm::translate(glm::fmat4(1.0f), scaled_center) * glm::toMat4(*rotation) *
          glm::translate(glm::fmat4(1.0f), -scaled_center);
  }
  if (matrix_scale) {
    ret = ret * glm::scale(glm::fmat4(1.0f), *matrix_scale);
  }
  return ret;
}

namespace nv3dvc {
namespace modules {
namespace commonmodule {
namespace components {

TransformComponent::TransformComponent() {
  translation.SetOnChangeFunction([this]() { m_translationFiltered = translation; });
  rotation.SetOnChangeFunction([this]() { m_rotationFiltered = rotation; });

  auto reset_trans_filter_fn = [this]() {
    m_translationFilter.Reset(translation_one_euro_filter_params.min_cutoff_freq,
                              translation_one_euro_filter_params.cutoff_slope,
                              translation_one_euro_filter_params.deriv_cutoff_freq);
  };
  auto reset_rot_filter_fn = [this]() {
    m_rotationFilter.Reset(rotation_one_euro_filter_params.min_cutoff_freq,
                           rotation_one_euro_filter_params.cutoff_slope,
                           rotation_one_euro_filter_params.deriv_cutoff_freq);
  };
  translation_one_euro_filter_params.min_cutoff_freq.SetOnChangeFunction(reset_trans_filter_fn);
  translation_one_euro_filter_params.cutoff_slope.SetOnChangeFunction(reset_trans_filter_fn);
  translation_one_euro_filter_params.deriv_cutoff_freq.SetOnChangeFunction(reset_trans_filter_fn);

  rotation_one_euro_filter_params.min_cutoff_freq.SetOnChangeFunction(reset_rot_filter_fn);
  rotation_one_euro_filter_params.cutoff_slope.SetOnChangeFunction(reset_rot_filter_fn);
  rotation_one_euro_filter_params.deriv_cutoff_freq.SetOnChangeFunction(reset_rot_filter_fn);

  reset_trans_filter_fn();
  reset_rot_filter_fn();
}

glm::mat4 TransformComponent::GetLocalMatrix() {
  glm::vec3 center;
  glm::vec3 translation;
  glm::quat rotation;
  glm::vec3 scale;
  GetTransform(&center, &translation, &rotation, &scale);
  return ComposeTransformMatrix(&center, &translation, &rotation, &scale, &scale);
}

glm::mat4 TransformComponent::GetLocalMatrixFiltered() {
  glm::vec3 center;
  glm::vec3& translation = m_translationFiltered;
  glm::quat& rotation = m_rotationFiltered;
  glm::vec3 scale;
  GetTransform(&center, m_isTemporal ? nullptr : &translation, m_isTemporal ? nullptr : &rotation, &scale);
  return ComposeTransformMatrix(&center, &translation, &rotation, &scale, &scale);
}

glm::mat4 TransformComponent::GetRt() {
  glm::vec3 center;
  glm::vec3 translation;
  glm::quat rotation;
  glm::vec3 scale;
  GetTransform(&center, &translation, &rotation, &scale);
  return ComposeTransformMatrix(&center, &translation, &rotation, &scale, nullptr);
}

glm::mat4 TransformComponent::GetRtFiltered() {
  glm::vec3 center;
  glm::vec3& translation = m_translationFiltered;
  glm::quat& rotation = m_rotationFiltered;
  glm::vec3 scale;
  GetTransform(&center, m_isTemporal ? nullptr : &translation, m_isTemporal ? nullptr : &rotation, &scale);
  return ComposeTransformMatrix(&center, &translation, &rotation, &scale, nullptr);
}

void TransformComponent::GetTransform(glm::vec3* center, glm::vec3* translation, glm::quat* rotation,
                                      glm::vec3* scale) {
  std::lock_guard lock(m_dataAccessMutex);
  if (center) *center = this->center;
  if (translation) *translation = this->translation;
  if (rotation) *rotation = this->rotation;
  if (scale) *scale = this->scale;
}

glm::fmat4 TransformComponent::GetRotationMatrix() {
  glm::vec3 center;
  glm::quat rotation;
  glm::vec3 scale;
  GetTransform(&center, nullptr, &rotation, &scale);

  const glm::vec3 scaled_center = center * scale;
  glm::mat4 rotation_matrix = glm::translate(glm::fmat4(1.0f), scaled_center) * glm::toMat4(rotation) *
                              glm::translate(glm::fmat4(1.0f), -scaled_center);
  return rotation_matrix;
}

glm::fmat4 TransformComponent::GetTranslationMatrix() {
  glm::vec3 translation;
  GetTransform(nullptr, &translation, nullptr, nullptr);

  glm::mat4 translation_matrix = glm::translate(glm::fmat4(1.0f), translation);
  return translation_matrix;
}

glm::fmat4 TransformComponent::GetScaleMatrix() {
  glm::vec3 scale;
  GetTransform(nullptr, nullptr, nullptr, &scale);

  glm::mat4 scale_matrix = glm::scale(glm::fmat4(1.0f), scale);
  return scale_matrix;
}

glm::vec3 TransformComponent::GetCenter() {
  std::lock_guard lock(m_dataAccessMutex);
  return center;
}

glm::vec3 TransformComponent::GetTranslation() {
  std::lock_guard lock(m_dataAccessMutex);
  return translation;
}

glm::quat TransformComponent::GetRotation() {
  std::lock_guard lock(m_dataAccessMutex);
  return rotation;
}

glm::vec3 TransformComponent::GetScale() {
  std::lock_guard lock(m_dataAccessMutex);
  return scale;
}

glm::vec3 TransformComponent::GetTranslationFiltered() const { return m_translationFiltered; }

glm::quat TransformComponent::GetRotationFiltered() const { return m_rotationFiltered; }

void TransformComponent::SetCenter(const glm::vec3& center) {
  std::lock_guard lock(m_dataAccessMutex);
  this->center = center;
}
void TransformComponent::SetTranslation(const glm::vec3& translation) {
  m_translationExtrapolator.Observe(translation);
  std::lock_guard lock(m_dataAccessMutex);
  this->translation = translation;
}
void TransformComponent::SetRotation(const glm::quat& rotation) {
  m_rotationExtrapolator.Observe(rotation);
  std::lock_guard lock(m_dataAccessMutex);
  this->rotation = rotation;
}
void TransformComponent::SetScale(const glm::vec3& scale) {
  std::lock_guard lock(m_dataAccessMutex);
  this->scale = scale;
}

void TransformComponent::Update(float dt) {
  m_isTemporal = true;
  m_translationFiltered = m_translationExtrapolator.GetLastVal();
  m_rotationFiltered = m_rotationExtrapolator.GetLastVal();
  if (extrapolate_pose && m_translationExtrapolator.HadObservation() && m_rotationExtrapolator.HadObservation()) {
    m_translationFiltered = m_translationExtrapolator.Extrapolate(dt);
    m_rotationFiltered = m_rotationExtrapolator.Extrapolate(dt);
  }

  if (filter_pose) {
    m_translationFiltered = m_translationFilter.Filter(m_translationFiltered, dt);
    m_rotationFiltered = m_rotationFilter.Filter(m_rotationFiltered, dt);
  }
}

}  // namespace components
}  // namespace commonmodule
}  // namespace modules
}  // namespace nv3dvc
