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

#include "VideoEffectsModule.h"

#include "Core/Util/Logger.h"
#include "nvVideoEffects.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Static function declarations                                                                                     ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Log callback function for NvVFX
/// @param[in] user_data Pointer to core::engine::Engine
/// @param[in] msg       The log message to print. nullptr at deinitialization.
static void VideoEffectsLogCallback(void* user_data, const char* msg);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Static function definitions                                                                                      ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void VideoEffectsLogCallback(void* user_data, const char* msg) {
  auto* engine = static_cast<nv3dvc::core::engine::Engine*>(user_data);
  if (msg) {
    if (engine->log_level == NVCV_LOG_FATAL) {
      LOG_FATAL("%s", msg);
    }
    if (engine->log_level == NVCV_LOG_ERROR) {
      LOG_ERROR("%s", msg);
    }
    if (engine->log_level == NVCV_LOG_WARNING) {
      LOG_WARNING("%s", msg);
    }
    if (engine->log_level == NVCV_LOG_INFO) {
      LOG_INFO("%s", msg);
    }
    if (engine->log_level == NVCV_LOG_DEBUG) {
      LOG_DEBUG("%s", msg);
    }
    if (engine->log_level == NVCV_LOG_VERBOSE) {
      LOG_VERBOSE("%s", msg);
    }
  }
}

namespace nv3dvc {
namespace modules {
namespace videoeffectsmodule {

VideoEffectsModule::VideoEffectsModule(core::engine::Engine* engine) : m_engine(engine) {
  RegisterSystem<systems::AigsSystem>();
  RegisterComponent<components::VideoEffectsComponent>();
}

core::Error VideoEffectsModule::Initialize() {
  core::Error err = core::Error::SUCCESS;
  if (m_engine) {
    CHECK_NVCV_SUCCESS(
        NvVFX_ConfigureLogger(m_engine->log_level, nullptr, VideoEffectsLogCallback, static_cast<void*>(m_engine)));
  } else {
    LOG_WARNING("Engine not provided to VideoEffectsModule. No logging configured for NvVFX.");
  }
bail:
  return err;
}

core::Error VideoEffectsModule::Uninitialize() { return core::Error::SUCCESS; }

core::Error VideoEffectsModule::Update(float dt) { return core::Error::SUCCESS; }

}  // namespace videoeffectsmodule
}  // namespace modules
}  // namespace nv3dvc
