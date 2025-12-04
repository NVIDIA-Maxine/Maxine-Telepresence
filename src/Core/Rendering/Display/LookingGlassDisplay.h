/*
 * SPDX-FileCopyrightText: Copyright (c) 2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: LicenseRef-NvidiaProprietary
 *
 * NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
 * property and proprietary rights in and to this material, related
 * documentation and any modifications thereto. Any use, reproduction,
 * disclosure or distribution of this material and related documentation
 * without an express license agreement from NVIDIA CORPORATION or
 * its affiliates is strictly prohibited.
 */

#ifndef SRC_CORE_RENDERING_DISPLAY_LOOKINGGLASSDISPLAY_H_
#define SRC_CORE_RENDERING_DISPLAY_LOOKINGGLASSDISPLAY_H_

#include <memory>
#include <string>
#include <vector>

#include "Core/Rendering/Shader.h"
#include "SimpleDisplay.h"

namespace nv3dvc {
namespace core {
namespace rendering {
namespace display {

class LookingGlassDisplay : public SimpleDisplay {
 public:
  constexpr static const char* NAME = "LookingGlassDisplay";
  std::string Name() const { return NAME; }

  LookingGlassDisplay();
  ~LookingGlassDisplay() override;

  core::Error Initialize(uint32_t render_width, uint32_t render_height, bool enforce_render_size,
                         uint32_t horizontal_resolution_divider, uint32_t vertical_resolution_divider) override;

  bool BlitToDisplayBuffer(uint32_t fbo, uint32_t rendered_texture_id) override;
  uint32_t QuiltCols() const override;
  uint32_t QuiltRows() const override;
  bool GetDesiredWindowPositionAndSize(int* pos_x, int* pos_y, int* width, int* height) const override;
  bool GetStereoViewPoints(std::vector<glm::vec3>* view_points) override;

  core::Error SetupQuiltLayoutFromPreset(uint32_t tile_width, uint32_t tile_height, int landscape_preset);
  core::Error LoadLightFieldShader();

 private:
  core::Error GetLookingGlassInfo();
  void LoadDummyDisplayConfiguration();
  bool LoadDisplayConfiguration();
  void UpdateLightFieldShaderSettings();

  const int kDevIndex = 0;     // Device index default is 0, the first Looking Glass detected
  float m_viewCone = 40.0;     // Full horizontal view cone of hardware, always around 40 degrees
  float m_verticalFov = 15.0;  // 38.1 for 36'' distance to 32'' display

  // light field shader settings
  float m_pitch = 0.0f;   // Lenticular lens pitch
  float m_tilt = 0.0f;    // Lenticular tilt angle
  float m_center = 0.0f;  // Lenticular center offset
  int m_invView = 0;      // Whether the lenticular shader should be inverted. (1 or 0)
  int m_quiltInvert = 0;
  float m_subp = 0.0f;    // Display subpixel size
  float m_fringe = 0.0f;  // Display fringe correction uniform. (Currently only applicable to Large / Pro units.)
  int m_ri = 0;           // 'Red index' of each lenticular subpixel. (0 or 2)
  int m_bi = 0;           // 'Blue index' of each lenticular subpixel. (0 or 2)
  int m_overscan = 0;

  float m_quiltAspectRatio = 1.0f;  // Typically same as display aspect ratio

  uint32_t m_quiltWidth = 0;
  uint32_t m_quiltHeight = 0;
  uint32_t m_quiltCols = 0;
  uint32_t m_quiltRows = 0;

  std::shared_ptr<Shader> m_lightFieldShaderProgram;
};

}  // namespace display
}  // namespace rendering
}  // namespace core
}  // namespace nv3dvc

#endif  // SRC_CORE_RENDERING_DISPLAY_LOOKINGGLASSDISPLAY_H_
