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

#ifndef SRC_CORE_RENDERING_DISPLAY_DIMENCODISPLAY_H_
#define SRC_CORE_RENDERING_DISPLAY_DIMENCODISPLAY_H_

#include <memory>
#include <string>
#include <vector>

#include "SimpleDisplay.h"

namespace SR {
class SRContext;
class Screen;
class GLWeaver;
class EyeTracker;
class SystemSense;
}  // namespace SR

namespace nv3dvc {
namespace core {
namespace rendering {
namespace display {

class TrackedEyes {
 public:
  void Set(const glm::vec3 left, const glm::vec3 right) {
    std::lock_guard lock(m_trackedEyesMutex);
    m_eyeLeft = left;
    m_eyeRight = right;
    if (fix_focal_plane_distance) {
      m_eyeLeft.z = focal_plane_distance;
      m_eyeRight.z = focal_plane_distance;
    }
  }
  glm::vec3 eyeLeft() {
    std::lock_guard lock(m_trackedEyesMutex);
    return m_eyeLeft;
  }
  glm::vec3 eyeRight() {
    std::lock_guard lock(m_trackedEyesMutex);
    return m_eyeRight;
  }
  bool fix_focal_plane_distance = false;
  float focal_plane_distance = 600.0f;  // Millimeters

 private:
  std::mutex m_trackedEyesMutex;
  glm::vec3 m_eyeLeft = {-30.0f, 0.0f, 600.0f};  // Millimeters
  glm::vec3 m_eyeRight = {30.0f, 0.0f, 600.0f};  // Millimeters
};

// Forward declaration
class MyEyes;

class DimencoDisplay : public SimpleDisplay {
 public:
  constexpr static const char* NAME = "DimencoDisplay";
  std::string Name() const { return NAME; }

  DimencoDisplay();
  ~DimencoDisplay() override;

  /// @brief See Display::Initialize
  core::Error Initialize(uint32_t render_width, uint32_t render_height, bool enforce_render_size,
                         uint32_t horizontal_resolution_divider, uint32_t vertical_resolution_divider) override;

  bool IsActiveStereo() const override;
  bool BlitToDisplayBuffer(unsigned int fbo, unsigned int rendered_texture_id) override;
  bool GetStereoViewPoints(std::vector<glm::vec3>* view_points);
  float ScreenWidthMm() const override;
  float ScreenHeightMm() const override;
  unsigned int QuiltCols() const override { return 2; };
  unsigned int QuiltRows() const override { return 1; };

 private:
  TrackedEyes m_trackedEyes;
  bool m_contextValid = false;
  std::mutex m_constructNewContextMutex;

  std::shared_ptr<MyEyes> m_srEyes = nullptr;
  SR::SRContext* m_srContext = nullptr;
  SR::Screen* m_srScreen = nullptr;
  SR::GLWeaver* m_srWeaver = nullptr;
  SR::EyeTracker* m_eyeTracker = nullptr;
  SR::SystemSense* m_systemSense = nullptr;

  /// @brief Create the SimulatedReality context
  /// @return True If successful
  bool CreateSrContext();

  /// @brief Initialize the SimulatedReality objects required in the scene
  /// @return True If successful
  bool InitializeSrObjects();

  /// @brief Destroys the SimulatedReality context
  void DestroySrContext();

  /// @brief Stop all SimulatedReality objects
  void StopSrObjects();
};

}  // namespace display
}  // namespace rendering
}  // namespace core
}  // namespace nv3dvc

#endif  // SRC_CORE_RENDERING_DISPLAY_DIMENCODISPLAY_H_
