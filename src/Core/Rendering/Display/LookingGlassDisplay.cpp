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

#ifdef NV3DVC_WITH_LOOKING_GLASS

#include "LookingGlassDisplay.h"

#include <memory>
#include <string>
#include <vector>

#include "Core/Util/Logger.h"
#include "GLFW/glfw3.h"
#include "HoloPlayCore.h"
#include "HoloPlayShaders.h"

namespace nv3dvc {
namespace core {
namespace rendering {
namespace display {

LookingGlassDisplay::LookingGlassDisplay() { m_numViews = 2; }

LookingGlassDisplay::~LookingGlassDisplay() {}

core::Error LookingGlassDisplay::Initialize(uint32_t render_width, uint32_t render_height, bool enforce_render_size,
                                            uint32_t horizontal_resolution_divider,
                                            uint32_t vertical_resolution_divider) {
  core::Error err = core::Error::SUCCESS;
  err = Display::Initialize(render_width, render_height, enforce_render_size, horizontal_resolution_divider,
                            vertical_resolution_divider);
  if (GetLookingGlassInfo() != core::Error::SUCCESS) {
    LOG_INFO("HoloplayCore Message Pipe tear down");
    hpc_TeardownMessagePipe();
    LOG_WARNING("Couldn't find looking glass. Running application with dummy display");
    BAIL(err, core::Error::ERR_DISPLAY);
  }

  if (enforce_render_size) {
    m_renderWidth = render_width;
    m_renderHeight = render_height;
  } else {
    LOG_WARNING(
        "Using display resolution for LookinGlassDisplay. You may want to enforce render size for quilted rendering.");
    m_renderWidth = m_screenResolutionX / m_horizontalResolutionDivider;
    m_renderHeight = m_screenResolutionY / m_verticalResolutionDivider;
  }
  m_renderAspectRatio = static_cast<float>(m_renderWidth) / static_cast<float>(m_renderHeight);
bail:
  return err;
}

bool LookingGlassDisplay::BlitToDisplayBuffer(uint32_t fbo, uint32_t rendered_texture_id) {
  if (!m_lightFieldShaderProgram) return false;

  // bind the display framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, m_screenResolutionX, m_screenResolutionY);  // reset viewport to window size

  // Clean the back buffer and depth buffer
  glClearColor(0.f, 0.f, 0.f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glDisable(GL_DEPTH_TEST);  // prevents screen-filling triangle from being discarded

  // Draw the framebuffer rectangle
  // activate lightfield interlace shader
  m_lightFieldShaderProgram->Activate();

  UpdateLightFieldShaderSettings();

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, rendered_texture_id);

  m_lightFieldShaderProgram->SetUniform("screenTex", 0);
  m_lightFieldShaderProgram->SetUniform("screenDepthTex", 1);
  m_lightFieldShaderProgram->SetUniform("rangeTexture", 2);

  glDrawArrays(GL_TRIANGLES, 0, 3);  // run interlacing by drawing a single triangle covering viewport

  m_lightFieldShaderProgram->Deactivate();

  glEnable(GL_DEPTH_TEST);  // re-enable depth test

  // deactivate texture units
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, 0);

  return true;
}

uint32_t LookingGlassDisplay::QuiltCols() const { return m_quiltCols; }

uint32_t LookingGlassDisplay::QuiltRows() const { return m_quiltRows; }

bool LookingGlassDisplay::GetDesiredWindowPositionAndSize(int* pos_x, int* pos_y, int* width, int* height) const {
  if (pos_x) *pos_x = m_screenPositionX;
  if (pos_y) *pos_y = m_screenPositionY;
  if (width) *width = m_screenResolutionX;
  if (height) *height = m_screenResolutionY;
  return pos_x && pos_y && width && height;
}

bool LookingGlassDisplay::GetStereoViewPoints(std::vector<glm::vec3>* view_points) {
  view_points->clear();
  view_points->resize(m_numViews);
  // This could be a property
  const float camera_distance_meters =
      0.9144f;  // LookingGlass recommends an average distance of 36 inches. 36'' = 0.9144 meters
  for (size_t i = 0; i < m_numViews; i++) {
    // start at -view_cone * 0.5 and go up to view_cone * 0.5
    float offset_angle = (i / (m_numViews - 1.0f) - 0.5f) * glm::radians(m_viewCone);
    float offset = camera_distance_meters * tan(offset_angle);  // calculate the offset that the camera should move
    glm::fvec3 eye_position = glm::fvec3(offset, 0.0f, camera_distance_meters);
    (*view_points)[i] = eye_position;
  }
  return true;
}

core::Error LookingGlassDisplay::GetLookingGlassInfo() {
  hpc_client_error errco = hpc_InitializeApp("Holoplay Core Example App", hpc_LICENSE_NONCOMMERCIAL);
  if (errco) {
    std::string errstr;
    switch (errco) {
      case hpc_CLIERR_NOSERVICE:
        errstr = "HoloPlay Service not running";
        break;
      case hpc_CLIERR_SERIALIZEERR:
        errstr = "Client message could not be serialized";
        break;
      case hpc_CLIERR_VERSIONERR:
        errstr = "Incompatible version of HoloPlay Service";
        break;
      case hpc_CLIERR_PIPEERROR:
        errstr = "Interprocess pipe broken";
        break;
      case hpc_CLIERR_SENDTIMEOUT:
        errstr = "Interprocess pipe send timeout";
        break;
      case hpc_CLIERR_RECVTIMEOUT:
        errstr = "Interprocess pipe receive timeout";
        break;
      default:
        errstr = "Unknown error";
        break;
    }
    LOG_ERROR("HoloPlay Service: (code: %u): %s", errco, errstr);
    LoadDummyDisplayConfiguration();
    return core::Error::ERR_DISPLAY;
  } else if (!LoadDisplayConfiguration()) {
    LoadDummyDisplayConfiguration();
    return core::Error::ERR_DISPLAY;
  } else {
    char buf[1000];
    hpc_GetHoloPlayCoreVersion(buf, 1000);
    LOG_INFO("HoloPlay Service: Core version %s", buf);
    hpc_GetHoloPlayServiceVersion(buf, 1000);
    LOG_INFO("HoloPlay Service: Service version %s", buf);
    int num_displays = hpc_GetNumDevices();
    if (num_displays < 1) {
      LOG_ERROR("HoloPlay Service: No devices connected");
      return core::Error::ERR_DISPLAY;
    }
    LOG_INFO("HoloPlay Service: %d devices connected", num_displays);
    for (int i = 0; i < num_displays; ++i) {
      LOG_INFO("Device information for display %d", i);
      hpc_GetDeviceHDMIName(i, buf, 1000);
      LOG_INFO("Device name: %s", buf);
      hpc_GetDeviceType(i, buf, 1000);
      LOG_INFO("Device type: %s", buf);
      // Window parameters and quilt texture settings
      LOG_INFO("Window parameters for display %d:", i);
      LOG_INFO("Position: (%d,%d)", hpc_GetDevicePropertyWinX(i), hpc_GetDevicePropertyWinY(i));
      LOG_INFO("Size: (%d,%d)", hpc_GetDevicePropertyScreenW(i), hpc_GetDevicePropertyScreenH(i));
      LOG_INFO("Display aspect ratio: %.12f", hpc_GetDevicePropertyDisplayAspect(i));
      LOG_INFO("Quilt aspect ratio: %.12f", hpc_GetDevicePropertyQuiltAspect(i));
      LOG_INFO("Hor quilt tex res: %d", hpc_GetDevicePropertyQuiltX(i));
      LOG_INFO("Vert quilt tex res: %d", hpc_GetDevicePropertyQuiltY(i));
      LOG_INFO("Hor quilt tile dim: %d", hpc_GetDevicePropertyTileX(i));
      LOG_INFO("Vert quilt tile dim: %d", hpc_GetDevicePropertyTileY(i));
      // Interlace shader parameters
      LOG_INFO("Shader uniforms for display %d:", i);
      LOG_INFO("Pitch: %.12f", hpc_GetDevicePropertyPitch(i));
      LOG_INFO("Tilt: %.12f", hpc_GetDevicePropertyTilt(i));
      LOG_INFO("Center: %.12f", hpc_GetDevicePropertyCenter(i));
      LOG_INFO("Subpixel width: %.12f", hpc_GetDevicePropertySubp(i));

      LOG_INFO("View cone: %.12f", hpc_GetDevicePropertyFloat(i, "/calibration/viewCone/value"));
      LOG_INFO("Fringe: %.12f", hpc_GetDevicePropertyFringe(i));
      LOG_INFO("RI: %d", hpc_GetDevicePropertyRi(i));
      LOG_INFO("BI: %d", hpc_GetDevicePropertyBi(i));
      LOG_INFO("invView: %d", hpc_GetDevicePropertyInvView(i));
    }
  }
  return core::Error::SUCCESS;
}

void LookingGlassDisplay::LoadDummyDisplayConfiguration() {
  // These parameters are taken from 8.9" gen1 display
  m_screenPositionX = 0;
  m_screenPositionY = 0;
  m_screenResolutionX = 2560;
  m_screenResolutionY = 1600;

  m_pitch = 370.869f;
  m_tilt = 0.117437f;
  m_center = 0.0586957f;
  m_subp = 0.000130208f;
  m_viewCone = 40.f;
  m_fringe = 0.f;
  m_ri = 0;
  m_bi = 2;
  m_invView = 1;

  m_displayAspectRatio = static_cast<float>(m_screenResolutionX) / static_cast<float>(m_screenResolutionY);
  m_quiltAspectRatio = m_displayAspectRatio;
}

bool LookingGlassDisplay::LoadDisplayConfiguration() {
  m_screenPositionX = hpc_GetDevicePropertyWinX(kDevIndex);
  m_screenPositionY = hpc_GetDevicePropertyWinY(kDevIndex);
  m_screenResolutionX = hpc_GetDevicePropertyScreenW(kDevIndex);
  m_screenResolutionY = hpc_GetDevicePropertyScreenH(kDevIndex);

  if (m_screenResolutionX == 0 || m_screenResolutionY == 0) {
    LOG_ERROR("Failed to read physical sceen resolution of LookingGlass display.");
    return false;
  }

  int monitor_idx = Display::FindMonitorBasedOnDesktopLocation(m_screenPositionX, m_screenPositionY);
  int screen_width_mm_i, screen_height_mm_i;
  core::Error err = Display::GetPhysicalSizeMm(monitor_idx, &screen_width_mm_i, &screen_height_mm_i);
  if (err != core::Error::SUCCESS) {
    LOG_ERROR("Failed to read physical screen size of LG resolution of LookingGlass display.");
    return false;
  }
  m_screenWidthMm = static_cast<float>(screen_width_mm_i);
  m_screenHeightMm = static_cast<float>(screen_height_mm_i);

  m_displayAspectRatio = hpc_GetDevicePropertyDisplayAspect(kDevIndex);
  if (m_displayAspectRatio == 0.f)
    m_displayAspectRatio = static_cast<float>(m_screenResolutionX) / static_cast<float>(m_screenResolutionY);

  // sdk reports 0, so this usually matches display aspect ratio
  // m_displayAspectRatio = hpc_GetDevicePropertyQuiltAspect(kDevIndex);
  m_quiltAspectRatio = m_displayAspectRatio;

  m_pitch = hpc_GetDevicePropertyPitch(kDevIndex);
  m_tilt = hpc_GetDevicePropertyTilt(kDevIndex);
  m_center = hpc_GetDevicePropertyCenter(kDevIndex);
  m_invView = hpc_GetDevicePropertyInvView(kDevIndex);
  m_subp = hpc_GetDevicePropertySubp(kDevIndex);
  m_fringe = hpc_GetDevicePropertyFringe(kDevIndex);
  m_ri = hpc_GetDevicePropertyRi(kDevIndex);
  m_bi = hpc_GetDevicePropertyBi(kDevIndex);
  m_viewCone = hpc_GetDevicePropertyFloat(kDevIndex, "/calibration/viewCone/value");

  return true;
}

core::Error LookingGlassDisplay::SetupQuiltLayoutFromPreset(uint32_t tile_width, uint32_t tile_height,
                                                            int landscape_preset) {
  uint32_t window_width = ScreenResolutionX();
  // presets for different displays:
  if (window_width == 1536) {
    // looking glass portrait
    // 1536x2048
    // 0.75, 1.333
    m_quiltCols = 8;
    m_quiltRows = 6;
  } else if (window_width == 1440) {
    // looking glass go
    // 1440 x 2560 -> aspect ratio:
    // 0.56, 1.7777
    m_quiltCols = 9;
    m_quiltRows = 5;
  } else {
    // landscape looking glass
    // 3840x2160 (16") -> aspect 1.777
    // 7680<D7>4320 (32", 65") -> aspect 1.777
    switch (landscape_preset) {
      case 0:  // standard, 2k x 2k, 32 views
        m_quiltCols = 4;
        m_quiltRows = 8;
        break;
      case 1:  // 4k x 4k, 45 views
        m_quiltCols = 5;
        m_quiltRows = 9;
        break;
      case 2:  // 8k x 8k, 45 views
        m_quiltCols = 5;
        m_quiltRows = 9;
        break;
      case 3:  // 16k by 8k, 90 views
        m_quiltCols = 10;
        m_quiltRows = 9;
        break;
      case 4:  // 16k by 16k, 180 views
        m_quiltCols = 10;
        m_quiltRows = 18;
        break;
      default:
        m_quiltCols = 4;
        m_quiltRows = 8;
        break;
    }
  }
  m_quiltWidth = m_quiltCols * tile_width;
  m_quiltHeight = m_quiltRows * tile_height;
  m_numViews = m_quiltCols * m_quiltRows;
  return core::Error::SUCCESS;
}

core::Error LookingGlassDisplay::LoadLightFieldShader() {
  core::Error err = core::Error::SUCCESS;
  LOG_INFO("Loading interlace shader");

  const std::string vert_source =
#include "Core/Rendering/Shaders/LookingglassInterlacer.vert"  // NOLINT(build/include) (No limit in how many times a shader can be included)
      ;  // NOLINT(whitespace/semicolon) (Needed when including string literal)
  const std::string frag_source =
#include "Core/Rendering/Shaders/LookingglassInterlacer.frag"  // NOLINT(build/include) (No limit in how many times a shader can be included)
      ;  // NOLINT(whitespace/semicolon) (Needed when including string literal)

  m_lightFieldShaderProgram = std::make_shared<Shader>();
  err = m_lightFieldShaderProgram->CreateShaderFromSource(vert_source, frag_source);
  CHECK_SUCCESS(err, "Failed to create light field shader");

  // set default shader settings
  m_lightFieldShaderProgram->Activate();
  UpdateLightFieldShaderSettings();
  m_lightFieldShaderProgram->Deactivate();
bail:
  return err;
}

// must have light field shader activated before calling !
void LookingGlassDisplay::UpdateLightFieldShaderSettings() {
  GLenum gl_err = 0;
  Shader& shader = *m_lightFieldShaderProgram.get();

  shader.SetUniform("pitch", m_pitch);
  shader.SetUniform("tilt", m_tilt);
  shader.SetUniform("center", m_center);
  shader.SetUniform("invView", m_invView);
  shader.SetUniform("quiltInvert", m_quiltInvert);
  shader.SetUniform("subp", m_subp);
  shader.SetUniform("ri", m_ri);
  shader.SetUniform("bi", m_bi);
  shader.SetUniform("displayAspect", m_displayAspectRatio);
  shader.SetUniform("quiltAspect", m_quiltAspectRatio);
  shader.SetUniform("overscan", m_overscan);

  glm::vec3 tile = glm::vec3(m_quiltCols, m_quiltRows, m_numViews);

  shader.SetUniform("tile", &tile);

  glm::vec2 viewPortion;
  {
    int qs_viewWidth = m_quiltWidth / m_quiltCols;
    int qs_viewHeight = m_quiltHeight / m_quiltRows;
    viewPortion = glm::vec2(static_cast<float>(qs_viewWidth * m_quiltCols) / static_cast<float>(m_quiltWidth),
                            static_cast<float>(qs_viewHeight * m_quiltRows) / static_cast<float>(m_quiltHeight));
  }
  shader.SetUniform("viewPortion", &viewPortion);
}

}  // namespace display
}  // namespace rendering
}  // namespace core
}  // namespace nv3dvc

#else  // NV3DVC_WITH_LOOKING_GLASS

#include <memory>
#include <string>
#include <vector>

#include "LookingGlassDisplay.h"

namespace nv3dvc {
namespace core {
namespace rendering {
namespace display {

LookingGlassDisplay::LookingGlassDisplay() { m_numViews = 2; }
LookingGlassDisplay::~LookingGlassDisplay() {}
core::Error LookingGlassDisplay::Initialize(uint32_t render_width, uint32_t render_height, bool enforce_render_size,
                                            uint32_t horizontal_resolution_divider,
                                            uint32_t vertical_resolution_divider) {
  return core::Error::ERR_UNIMPLEMENTED;
}
bool LookingGlassDisplay::BlitToDisplayBuffer(uint32_t fbo, uint32_t rendered_texture_id) { return false; }
uint32_t LookingGlassDisplay::QuiltCols() const { return 0; }
uint32_t LookingGlassDisplay::QuiltRows() const { return 0; }
bool LookingGlassDisplay::GetDesiredWindowPositionAndSize(int* pos_x, int* pos_y, int* width, int* height) const {
  return false;
}
bool LookingGlassDisplay::GetStereoViewPoints(std::vector<glm::vec3>* view_points) { return false; }
core::Error LookingGlassDisplay::GetLookingGlassInfo() { return core::Error::ERR_UNIMPLEMENTED; }
void LookingGlassDisplay::LoadDummyDisplayConfiguration() {}
bool LookingGlassDisplay::LoadDisplayConfiguration() { return false; }
core::Error LookingGlassDisplay::SetupQuiltLayoutFromPreset(uint32_t tile_width, uint32_t tile_height,
                                                            int landscape_preset) {
  return core::Error::ERR_UNIMPLEMENTED;
}
core::Error LookingGlassDisplay::LoadLightFieldShader() { return core::Error::ERR_UNIMPLEMENTED; }
void LookingGlassDisplay::UpdateLightFieldShaderSettings() {}

}  // namespace display
}  // namespace rendering
}  // namespace core
}  // namespace nv3dvc

#endif  // NV3DVC_WITH_LOOKING_GLASS
