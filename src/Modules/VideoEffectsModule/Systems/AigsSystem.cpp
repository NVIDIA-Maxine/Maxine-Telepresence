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

#include "AigsSystem.h"

#include <algorithm>

#include "Core/Engine/Engine.h"
#include "Core/EntityComponentSystem/Entity.h"
#include "Core/Util/Logger.h"
#include "Modules/CommonModule/Components/VideoFrameComponent.h"
#include "Modules/VideoEffectsModule/Components/VideoEffectsComponent.h"
#include "glm/glm.hpp"
#include "nvVFXGreenScreen.h"
#include "nvVideoEffects.h"

namespace nv3dvc {
namespace modules {
namespace videoeffectsmodule {
namespace systems {

AigsSystem::AigsSystem() : m_aigsEffect(nullptr), m_maxInputWidth(512), m_maxInputHeight(512) {}

core::Error AigsSystem::Initialize() {
  core::Error err = core::Error::SUCCESS;

  CHECK_NVCV_SUCCESS(NvVFX_CreateEffect(NVVFX_FX_GREEN_SCREEN, &m_aigsEffect));
  CHECK_NVCV_SUCCESS(NvVFX_SetString(m_aigsEffect, NVVFX_MODEL_DIRECTORY, vfx_sdk_model_dir.get()->c_str()));
  CHECK_NVCV_SUCCESS(NvVFX_SetU32(m_aigsEffect, NVVFX_MODE, effects_mode));
  CHECK_NVCV_SUCCESS(NvVFX_SetU32(m_aigsEffect, NVVFX_CUDA_GRAPH, cuda_graph ? 1u : 0u));
  CHECK_NVCV_SUCCESS(NvVFX_SetCudaStream(m_aigsEffect, NVVFX_CUDA_STREAM, GetStream()));
  // Call Load again if these change
  CHECK_NVCV_SUCCESS(NvVFX_SetU32(m_aigsEffect, NVVFX_MAX_INPUT_WIDTH, m_maxInputWidth));
  CHECK_NVCV_SUCCESS(NvVFX_SetU32(m_aigsEffect, NVVFX_MAX_INPUT_HEIGHT, m_maxInputHeight));
  CHECK_NVCV_SUCCESS(NvVFX_SetU32(m_aigsEffect, NVVFX_MAX_NUMBER_STREAMS, 1));
  CHECK_NVCV_SUCCESS(NvVFX_SetCudaStream(m_aigsEffect, NVVFX_CUDA_STREAM, GetStream()));
  CHECK_NVCV_SUCCESS(NvVFX_Load(m_aigsEffect));
bail:
  if (core::Error::SUCCESS != err)
    LOG_DEBUG("Failure to initialize AigsSystem, from model path \"%s\"", vfx_sdk_model_dir.get()->c_str());
  return err;
}

core::Error AigsSystem::Uninitialize() {
  if (m_aigsEffect) {
    NvVFX_DestroyEffect(m_aigsEffect);
    m_aigsEffect = nullptr;
  }
  return core::Error::SUCCESS;
}

core::Error AigsSystem::Run(core::ecs::registry::EntityRegistry* reg, float dt) {
  core::Error err = core::Error::SUCCESS;
  for (auto& entity : reg->view<components::VideoEffectsComponent, commonmodule::components::VideoFrameComponent>()) {
    auto& vfx = entity.GetComponent<components::VideoEffectsComponent>();
    auto& video_frame = entity.GetComponent<commonmodule::components::VideoFrameComponent>();
    if (!vfx.ShouldPerformAigs()) continue;
    NvCVImage* image = video_frame.GetImagePtr();
    NvCVImage* matte_image = vfx.GetMatteImagePtr();
    if (!image->pixels) {
      LOG_DEBUG("Skipping empty image in entity \"%s\"", entity.GetComponent<std::string>().c_str());
      continue;
    }
    if (!(image->width <= m_maxInputWidth && image->height <= m_maxInputHeight)) {
      m_maxInputWidth = std::max(image->width, m_maxInputWidth);
      m_maxInputHeight = std::max(image->height, m_maxInputHeight);

      CHECK_NVCV_SUCCESS(NvVFX_SetU32(m_aigsEffect, NVVFX_MAX_INPUT_WIDTH, m_maxInputWidth));
      CHECK_NVCV_SUCCESS(NvVFX_SetU32(m_aigsEffect, NVVFX_MAX_INPUT_HEIGHT, m_maxInputHeight));
      CHECK_NVCV_SUCCESS(NvVFX_Load(m_aigsEffect));
    }
    if (!(matte_image->width == image->width && matte_image->height == image->height)) {
      CHECK_NVCV_SUCCESS(NvCVImage_Realloc(matte_image, image->width, image->height, NVCV_A, image->componentType,
                                           NVCV_CHUNKY, NVCV_GPU, 0));
    }

    CHECK_NVCV_SUCCESS(NvVFX_SetImage(m_aigsEffect, NVVFX_INPUT_IMAGE, image));
    CHECK_NVCV_SUCCESS(NvVFX_SetImage(m_aigsEffect, NVVFX_OUTPUT_IMAGE, matte_image));
    CHECK_NVCV_SUCCESS(NvVFX_Run(m_aigsEffect, 1));

    void* bg_color_device_ptr = vfx.GetBackgroundColorDevicePtr();
    cudaStream_t cu_stream = GetStream();
    CHECK_NVCV_SUCCESS(NvCVImage_CompositeOverConstant(image, matte_image, bg_color_device_ptr, image, cu_stream));
    CHECK_SUCCESS(video_frame.UpdateRgbaImage(cu_stream, &m_tmpImg));
  }
bail:
  return err;
}

}  // namespace systems
}  // namespace videoeffectsmodule
}  // namespace modules
}  // namespace nv3dvc
