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

#include "RenderProperties.h"

#include <string>

#include "Core/EntityComponentSystem/Entity.h"
#include "Core/Util/Types.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/type_ptr.inl"
#include "glm/gtx/euler_angles.hpp"
#include "imgui.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Macros                                                                                                           ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define TOOL_TIP(prop)                      \
  {                                         \
    const char* desc = prop->description(); \
    if (ImGui::IsItemHovered() && desc) {   \
      ImGui::BeginTooltip();                \
      ImGui::TextUnformatted(desc);         \
      ImGui::EndTooltip();                  \
    }                                       \
  }

// Atomic scalar properties
#define GUI_INPUT_ATOMIC_SCALAR(Type, imgui_data_type)               \
  {                                                                  \
    auto props = property_owner->GetProperties<std::atomic<Type>>(); \
    for (auto prop : props) {                                        \
      Type val = static_cast<Type>(prop->get()->load());             \
      if (ImGui::InputScalar(prop->name(), imgui_data_type, &val)) { \
        prop->get()->store(val);                                     \
        prop->OnChange();                                            \
      }                                                              \
      TOOL_TIP(prop)                                                 \
    }                                                                \
  }

// Scalar properties of n components
#define GUI_INPUT_SCALAR_N(Type, imgui_data_type, num_components)                            \
  {                                                                                          \
    auto props = property_owner->GetProperties<Type>();                                      \
    for (auto prop : props) {                                                                \
      if (ImGui::InputScalarN(prop->name(), imgui_data_type, prop->get(), num_components)) { \
        prop->OnChange();                                                                    \
      }                                                                                      \
      TOOL_TIP(prop)                                                                         \
    }                                                                                        \
  }

// Drag scalar properties of n components (typically more convenient for floats)
#define GUI_DRAG_SCALAR_N(Type, imgui_data_type, num_components)                                   \
  {                                                                                                \
    auto props = property_owner->GetProperties<Type>();                                            \
    for (auto prop : props) {                                                                      \
      if (ImGui::DragScalarN(prop->name(), imgui_data_type, prop->get(), num_components, 0.01f)) { \
        prop->OnChange();                                                                          \
      }                                                                                            \
      TOOL_TIP(prop)                                                                               \
    }                                                                                              \
  }

// Slider scalar properties of n components, useful when the min and max values are known
#define GUI_SLIDER_SCALAR_N(Type, imgui_data_type, num_components, minval, maxval)                            \
  {                                                                                                           \
    auto props = property_owner->GetProperties<Type>();                                                       \
    for (auto prop : props) {                                                                                 \
      if (ImGui::SliderScalarN(prop->name(), imgui_data_type, prop->get(), num_components, minval, maxval)) { \
        prop->OnChange();                                                                                     \
      }                                                                                                       \
      TOOL_TIP(prop)                                                                                          \
    }                                                                                                         \
  }

namespace nv3dvc {
namespace modules {
namespace guimodule {
namespace panels {

void RenderProperties(const char* name, const core::properties::PropertyOwner* property_owner) {
  const bool opened = ImGui::TreeNodeEx(name, ImGuiTreeNodeFlags_None, "%s", name);
  if (opened) {
    //
    // Atomic integral types
    //

    // Atomic bool properties
    {
      auto props = property_owner->GetProperties<std::atomic<bool>>();
      for (auto prop : props) {
        bool val = prop->get()->load();
        if (ImGui::Checkbox(prop->name(), &val)) {
          prop->get()->store(val);
          prop->OnChange();
        }
        TOOL_TIP(prop)
      }
    }

    GUI_INPUT_ATOMIC_SCALAR(char, ImGuiDataType_S8)
    GUI_INPUT_ATOMIC_SCALAR(int8_t, ImGuiDataType_S8)
    GUI_INPUT_ATOMIC_SCALAR(uint8_t, ImGuiDataType_U8)
    GUI_INPUT_ATOMIC_SCALAR(int16_t, ImGuiDataType_S16)
    GUI_INPUT_ATOMIC_SCALAR(uint16_t, ImGuiDataType_U16)
    GUI_INPUT_ATOMIC_SCALAR(int32_t, ImGuiDataType_S32)
    GUI_INPUT_ATOMIC_SCALAR(uint32_t, ImGuiDataType_U32)
    GUI_INPUT_ATOMIC_SCALAR(int64_t, ImGuiDataType_S64)
    GUI_INPUT_ATOMIC_SCALAR(uint64_t, ImGuiDataType_U64)
    GUI_INPUT_ATOMIC_SCALAR(char16_t, ImGuiDataType_S16)
    GUI_INPUT_ATOMIC_SCALAR(char32_t, ImGuiDataType_S32)
    GUI_INPUT_ATOMIC_SCALAR(wchar_t, ImGuiDataType_U16)
    GUI_INPUT_ATOMIC_SCALAR(float, ImGuiDataType_Float)
    GUI_INPUT_ATOMIC_SCALAR(double, ImGuiDataType_Double)

    //
    // Standard types
    //

    // bool properties
    {
      auto props = property_owner->GetProperties<bool>();
      for (auto prop : props) {
        if (ImGui::Checkbox(prop->name(), prop->get())) {
          prop->OnChange();
        }
        TOOL_TIP(prop)
      }
    }

    // 1d scalars
    GUI_INPUT_SCALAR_N(char, ImGuiDataType_S8, 1)
    GUI_INPUT_SCALAR_N(int8_t, ImGuiDataType_S8, 1)
    GUI_INPUT_SCALAR_N(uint8_t, ImGuiDataType_U8, 1)
    GUI_INPUT_SCALAR_N(int16_t, ImGuiDataType_S16, 1)
    GUI_INPUT_SCALAR_N(uint16_t, ImGuiDataType_U16, 1)
    GUI_INPUT_SCALAR_N(int32_t, ImGuiDataType_S32, 1)
    GUI_INPUT_SCALAR_N(uint32_t, ImGuiDataType_U32, 1)
    GUI_INPUT_SCALAR_N(int64_t, ImGuiDataType_S64, 1)
    GUI_INPUT_SCALAR_N(uint64_t, ImGuiDataType_U64, 1)
    GUI_INPUT_SCALAR_N(char16_t, ImGuiDataType_S16, 1)
    GUI_INPUT_SCALAR_N(char32_t, ImGuiDataType_S32, 1)
    GUI_INPUT_SCALAR_N(wchar_t, ImGuiDataType_U16, 1)
    GUI_DRAG_SCALAR_N(float, ImGuiDataType_Float, 1)
    GUI_DRAG_SCALAR_N(double, ImGuiDataType_Double, 1)

    //
    // Vector and matrix types
    //
    GUI_INPUT_SCALAR_N(glm::bvec1, ImGuiDataType_U8, 1)
    GUI_INPUT_SCALAR_N(glm::bvec2, ImGuiDataType_U8, 2)
    GUI_INPUT_SCALAR_N(glm::bvec3, ImGuiDataType_U8, 3)
    GUI_INPUT_SCALAR_N(glm::bvec4, ImGuiDataType_U8, 4)
    GUI_INPUT_SCALAR_N(glm::ivec1, ImGuiDataType_S32, 1)
    GUI_INPUT_SCALAR_N(glm::ivec2, ImGuiDataType_S32, 2)
    GUI_INPUT_SCALAR_N(glm::ivec3, ImGuiDataType_S32, 3)
    GUI_INPUT_SCALAR_N(glm::ivec4, ImGuiDataType_S32, 4)
    GUI_INPUT_SCALAR_N(glm::uvec1, ImGuiDataType_U32, 1)
    GUI_INPUT_SCALAR_N(glm::uvec2, ImGuiDataType_U32, 2)
    GUI_INPUT_SCALAR_N(glm::uvec3, ImGuiDataType_U32, 3)
    GUI_INPUT_SCALAR_N(glm::uvec4, ImGuiDataType_U32, 4)
    GUI_DRAG_SCALAR_N(glm::fvec1, ImGuiDataType_Float, 1)
    GUI_DRAG_SCALAR_N(glm::fvec2, ImGuiDataType_Float, 2)
    GUI_DRAG_SCALAR_N(glm::fvec3, ImGuiDataType_Float, 3)
    GUI_DRAG_SCALAR_N(glm::fvec4, ImGuiDataType_Float, 4)
    GUI_DRAG_SCALAR_N(glm::dvec1, ImGuiDataType_Double, 1)
    GUI_DRAG_SCALAR_N(glm::dvec2, ImGuiDataType_Double, 2)
    GUI_DRAG_SCALAR_N(glm::dvec3, ImGuiDataType_Double, 3)
    GUI_DRAG_SCALAR_N(glm::dvec4, ImGuiDataType_Double, 4)

    GUI_DRAG_SCALAR_N(glm::mat2x2, ImGuiDataType_Float, 4)
    GUI_DRAG_SCALAR_N(glm::mat2x3, ImGuiDataType_Float, 6)
    GUI_DRAG_SCALAR_N(glm::mat3x2, ImGuiDataType_Float, 6)
    GUI_DRAG_SCALAR_N(glm::mat3x3, ImGuiDataType_Float, 9)
    GUI_DRAG_SCALAR_N(glm::mat3x4, ImGuiDataType_Float, 12)
    GUI_DRAG_SCALAR_N(glm::mat4x2, ImGuiDataType_Float, 8)
    GUI_DRAG_SCALAR_N(glm::mat4x3, ImGuiDataType_Float, 12)
    GUI_DRAG_SCALAR_N(glm::mat4x4, ImGuiDataType_Float, 16)

    GUI_DRAG_SCALAR_N(glm::dmat2x2, ImGuiDataType_Double, 4)
    GUI_DRAG_SCALAR_N(glm::dmat2x3, ImGuiDataType_Double, 6)
    GUI_DRAG_SCALAR_N(glm::dmat3x2, ImGuiDataType_Double, 6)
    GUI_DRAG_SCALAR_N(glm::dmat3x3, ImGuiDataType_Double, 9)
    GUI_DRAG_SCALAR_N(glm::dmat3x4, ImGuiDataType_Double, 12)
    GUI_DRAG_SCALAR_N(glm::dmat4x2, ImGuiDataType_Double, 8)
    GUI_DRAG_SCALAR_N(glm::dmat4x3, ImGuiDataType_Double, 12)
    GUI_DRAG_SCALAR_N(glm::dmat4x4, ImGuiDataType_Double, 16)

    //
    // Special types
    //

    // R and RG Color properties
    uint8_t uint8_min = 0;
    uint8_t uint8_max = 255;
    // Use sliders for R and RG
    GUI_SLIDER_SCALAR_N(glm::u8vec1, ImGuiDataType_U8, 1, &uint8_min, &uint8_max);
    GUI_SLIDER_SCALAR_N(glm::u8vec2, ImGuiDataType_U8, 2, &uint8_min, &uint8_max);

    // RGB color properties
    {
      auto props = property_owner->GetProperties<glm::u8vec3>();
      for (auto prop : props) {
        glm::u8vec3& color = *prop->get();
        glm::fvec3 color_f = glm::fvec3(color) / 255.0f;
        if (ImGui::ColorPicker3(prop->name(), glm::value_ptr(color_f), 0)) {
          color.r = static_cast<uint8_t>(color_f.r * 255);
          color.g = static_cast<uint8_t>(color_f.g * 255);
          color.b = static_cast<uint8_t>(color_f.b * 255);
          prop->OnChange();
        }
        TOOL_TIP(prop)
      }
    }
    // RGBA color properties
    {
      auto props = property_owner->GetProperties<glm::u8vec4>();
      for (auto prop : props) {
        glm::u8vec4& color = *prop->get();
        glm::fvec4 color_f = glm::fvec4(color) / 255.0f;
        if (ImGui::ColorPicker4(prop->name(), glm::value_ptr(color_f), 0)) {
          color.r = static_cast<uint8_t>(color_f.r * 255);
          color.g = static_cast<uint8_t>(color_f.g * 255);
          color.b = static_cast<uint8_t>(color_f.b * 255);
          color.a = static_cast<uint8_t>(color_f.a * 255);
          prop->OnChange();
        }
        TOOL_TIP(prop)
      }
    }
    // string properties
    {
      auto props = property_owner->GetProperties<std::string>();
      for (auto prop : props) {
        std::string& val = *prop->get();
        char buffer[256];
        memset(buffer, 0, sizeof(buffer));
        snprintf(buffer, sizeof(buffer), "%s", val.c_str());
        if (ImGui::InputText(prop->name(), buffer, sizeof(buffer))) {
          val = std::string(buffer);
          prop->OnChange();
        }
        TOOL_TIP(prop)
      }
    }
    // quat properties
    {
      auto props = property_owner->GetProperties<glm::quat>();
      for (auto prop : props) {
        glm::quat& val = *prop->get();
        if (ImGui::DragFloat4(prop->name(), glm::value_ptr(val), 0.01f, -1.0f, 1.0f)) {
          val = glm::normalize(val);
          prop->OnChange();
        }
        TOOL_TIP(prop)
      }
    }
    // dquat properties
    {
      auto props = property_owner->GetProperties<glm::dquat>();
      for (auto prop : props) {
        glm::quat val = *prop->get();
        if (ImGui::DragFloat4(prop->name(), glm::value_ptr(val), 0.01f, -1.0f, 1.0f)) {
          val = glm::normalize(val);
          *prop->get() = val;
          prop->OnChange();
        }
        TOOL_TIP(prop)
      }
    }
    // Trigger properties
    {
      auto props = property_owner->GetProperties<core::util::Trigger>();
      for (auto prop : props) {
        core::util::Trigger val = *prop->get();
        if (ImGui::Button(prop->name())) {
          prop->OnChange();
        }
        TOOL_TIP(prop)
      }
    }

    // Recursively render subowners
    for (auto& sub_owner : property_owner->SubOwners()) {
      RenderProperties(sub_owner.first.c_str(), sub_owner.second);
    }
    ImGui::TreePop();
  }
}

}  // namespace panels
}  // namespace guimodule
}  // namespace modules
}  // namespace nv3dvc
