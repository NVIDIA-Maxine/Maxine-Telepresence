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

#ifdef NV3DVC_WITH_DIMENCO

#include "DimencoDisplay.h"

#include <glad/glad.h>
#include <sr/sense/core/inputstream.h>
#include <sr/sense/eyetracker/eyetracker.h>
#include <sr/sense/handtracker/handtracker.h>
#include <sr/sense/system/systemevent.h>
#include <sr/sense/system/systemsense.h>
#include <sr/types.h>
#include <sr/utility/exception.h>
#include <sr/weaver/glweaver.h>
#include <sr/world/display/screen.h>

#include <chrono>  // NOLINT(build/c++11) (cpplint outside Chromium: https://github.com/google/styleguide/issues/483)
#include <thread>  // NOLINT(build/c++11) (cpplint outside Chromium: https://github.com/google/styleguide/issues/483)

namespace nv3dvc {
namespace core {
namespace rendering {
namespace display {

// My SR::EyePairListener that stores the last tracked eye positions
class MyEyes : public SR::EyePairListener {
 private:
  SR::InputStream<SR::EyePairStream> stream;
  TrackedEyes* m_eyes;

 public:
  MyEyes(SR::EyeTracker* tracker, TrackedEyes* eyes) : m_eyes(eyes) {
    // Open a stream between tracker and this class
    stream.set(tracker->openEyePairStream(this));
  }
  // Called by the tracker for each tracked eye pair
  void accept(const SR_eyePair& eyePair) override {
    // Remember the eye positions
    m_eyes->Set(glm::vec3(eyePair.left.x, eyePair.left.y, eyePair.left.z),
                glm::vec3(eyePair.right.x, eyePair.right.y, eyePair.right.z));
  }
};

DimencoDisplay::DimencoDisplay() { m_numViews = 2; }

DimencoDisplay::~DimencoDisplay() {
  StopSrObjects();
  DestroySrContext();
}

bool DimencoDisplay::IsActiveStereo() const { return true; }

core::Error DimencoDisplay::Initialize(uint32_t render_width, uint32_t render_height, bool enforce_render_size,
                                       uint32_t horizontal_resolution_divider, uint32_t vertical_resolution_divider) {
  core::Error err = core::Error::SUCCESS;
  bool res = true;
  err = Display::Initialize(render_width, render_height, enforce_render_size, horizontal_resolution_divider,
                            vertical_resolution_divider);
  BAIL_IF_ERR(err);
  res = InitializeSrObjects();
  BAIL_IF_FALSE(res, err, core::Error::ERR_DISPLAY);
bail:
  return err;
}

bool DimencoDisplay::CreateSrContext() {
  if (m_srContext != nullptr) {
    delete m_srContext;
    m_srContext = nullptr;
  }

  while (m_srContext == nullptr && m_contextValid == false) {
    try {
      m_srContext = SR::SRContext::create(true, SR::SRContext::NetworkMode::ClientMode);
      return true;
    } catch (SR::ServerNotAvailableException e) {
      std::cout << "SR server not available, trying again in 0.5 second" << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
  }
  return false;
}

bool DimencoDisplay::InitializeSrObjects() {
  std::lock_guard<std::mutex> lock(m_constructNewContextMutex);
  StopSrObjects();
  DestroySrContext();

  // constructing context
  if (CreateSrContext() == false) {
    return false;
  }
  // constructing EyePairListener
  m_eyeTracker = SR::EyeTracker::create(*m_srContext);
  m_srEyes = std::make_shared<MyEyes>(m_eyeTracker, &m_trackedEyes);
  m_systemSense = SR::SystemSense::create(*m_srContext);

  m_srContext->initialize();

  m_srScreen = SR::Screen::create(*m_srContext);

  // weaver must hold both views which is the same size as rendered width of both views combined
  m_srWeaver = SR::GLWeaver::create(*m_srContext, ScreenResolutionX(), ScreenResolutionY());
  GLenum glerr = glGetError();  // Get the last error as weaver may yield invalid enum without failure
  m_contextValid = true;
  return m_contextValid;
}

void DimencoDisplay::DestroySrContext() {
  if (m_srContext) {
    delete m_srContext;
    m_srContext = nullptr;
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(2000));  // Sleep to release camera
}

void DimencoDisplay::StopSrObjects() {
  if (m_eyeTracker) {
    m_eyeTracker->stop();
  }
  if (m_systemSense) {
    m_systemSense->stop();
  }
}

bool DimencoDisplay::BlitToDisplayBuffer(unsigned int fbo, unsigned int rendered_texture_id) {
  m_srWeaver->setInputFrameBuffer(fbo, rendered_texture_id);

  bool can_weave = m_srWeaver->canWeave(ScreenResolutionX(), ScreenResolutionY(), ScreenPositionX(), ScreenPositionY());
  if (!can_weave) {
    printf("WARNING: weaver reports configuration is incorrect !\n");
    return false;
  }
  m_srWeaver->weave(ScreenResolutionX(), ScreenResolutionY(), ScreenPositionX(), ScreenPositionY());
  return true;
}

bool DimencoDisplay::GetStereoViewPoints(std::vector<glm::vec3>* view_points) {
  // Millimeter to meter conversion
  *view_points = {m_trackedEyes.eyeLeft() * 0.001f, m_trackedEyes.eyeRight() * 0.001f};
  return true;
}

float DimencoDisplay::ScreenWidthMm() const {
  // Centimeter to millimeter conversion
  return 10.0f * m_srScreen->getPhysicalSizeWidth();
}

float DimencoDisplay::ScreenHeightMm() const {
  // Centimeter to millimeter conversion
  return 10.0f * m_srScreen->getPhysicalSizeHeight();
}

}  // namespace display
}  // namespace rendering
}  // namespace core
}  // namespace nv3dvc

#else  // NV3DVC_WITH_DIMENCO

#include "DimencoDisplay.h"

namespace nv3dvc {
namespace core {
namespace rendering {
namespace display {

class MyEyes {};

DimencoDisplay::DimencoDisplay() { m_numViews = 2; }

DimencoDisplay::~DimencoDisplay() {}

bool DimencoDisplay::IsActiveStereo() const { return true; }

core::Error DimencoDisplay::Initialize(uint32_t render_width, uint32_t render_height, bool enforce_render_size,
                                       uint32_t horizontal_resolution_divider, uint32_t vertical_resolution_divider) {
  return core::Error::ERR_UNIMPLEMENTED;
}

bool DimencoDisplay::CreateSrContext() { return false; }

bool DimencoDisplay::InitializeSrObjects() { return false; }

void DimencoDisplay::DestroySrContext() {}
void DimencoDisplay::StopSrObjects() {}
bool DimencoDisplay::BlitToDisplayBuffer(unsigned int fbo, unsigned int rendered_texture_id) { return false; }
bool DimencoDisplay::GetStereoViewPoints(std::vector<glm::vec3>* view_points) { return false; }
float DimencoDisplay::ScreenWidthMm() const { return 0.0f; }
float DimencoDisplay::ScreenHeightMm() const { return 0.0f; }

}  // namespace display
}  // namespace rendering
}  // namespace core
}  // namespace nv3dvc

#endif  // NV3DVC_WITH_DIMENCO
