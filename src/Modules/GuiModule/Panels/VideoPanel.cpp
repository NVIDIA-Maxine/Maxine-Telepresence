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

#include "VideoPanel.h"

#include <string>

#include "Core/EntityComponentSystem/Entity.h"
#include "Core/Util/Logger.h"
#include "Modules/CommonModule/Components/VideoFrameComponent.h"
#include "imgui.h"

namespace nv3dvc {
namespace modules {
namespace guimodule {
namespace panels {

core::Error VideoPanel::MappedImage::Initialize(const NvCVImage* img, cudaStream_t stream) {
  core::Error err = core::Error::SUCCESS;
  if (img->width <= 0 || img->height <= 0) {
    LOG_DEBUG("Image is not ready to be mapped yet. Try again after allocating.");
    return core::Error::ERR_INITIALIZATION;
  }
  img_src = img;
  glGenTextures(1, &tex_id);
  BAIL_IF_GLERR(glGetError(), err);
  glBindTexture(GL_TEXTURE_2D, tex_id);
  BAIL_IF_GLERR(glGetError(), err);

  // Setup filtering parameters for display
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  BAIL_IF_GLERR(glGetError(), err);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  BAIL_IF_GLERR(glGetError(), err);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img_src->width, img_src->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  BAIL_IF_GLERR(glGetError(), err);

  // register texture to CUDA resource
  // Image is only for display, i.e. transfer to the mapped image is sufficient
  CHECK_CUDA_SUCCESS(cudaGraphicsGLRegisterImage(&cuda_graphics_resource, tex_id, GL_TEXTURE_2D,
                                                 cudaGraphicsRegisterFlagsWriteDiscard));
  CHECK_CUDA_SUCCESS(cudaGraphicsMapResources(1, &cuda_graphics_resource, stream));
  CHECK_CUDA_SUCCESS(cudaGraphicsSubResourceGetMappedArray(&cuda_array, cuda_graphics_resource, 0, 0));
  initialized = true;
  LOG_DEBUG("Initialization of mapped image successful.");
bail:
  return err;
}

core::Error VideoPanel::MappedImage::UnInitialize(cudaStream_t stream) {
  core::Error err = core::Error::SUCCESS;
  if (cuda_graphics_resource) {
    CHECK_CUDA_SUCCESS(cudaGraphicsUnmapResources(1, &cuda_graphics_resource, stream));
    CHECK_CUDA_SUCCESS(cudaGraphicsUnregisterResource(cuda_graphics_resource));
  }
  if (tex_id) {
    glDeleteTextures(1, &tex_id);
    BAIL_IF_GLERR(glGetError(), err);
    LOG_DEBUG("UnInitialization of mapped image successful.");
  }
  cuda_graphics_resource = nullptr;
  cuda_array = nullptr;
  tex_id = 0;
  img_src = nullptr;
  initialized = false;
bail:
  return err;
}

core::Error VideoPanel::Initialize(core::ecs::registry::EntityRegistry* reg, cudaStream_t stream) {
  core::Error err = core::Error::SUCCESS;

  // First unitialize any old resources
  UnInitialize(reg, stream);
  // Try to initialize new images
  auto view = reg->view<commonmodule::components::VideoFrameComponent>();
  for (auto entity : view) {
    auto& video_frame = entity.GetComponent<commonmodule::components::VideoFrameComponent>();
    MappedImage mapped_image;
    m_mappedImages[entity] = mapped_image;
    if (mapped_image.Initialize(video_frame.GetImagePtr(), stream) != core::Error::SUCCESS) {
      LOG_DEBUG("Initialization of mapped image failed. Image has not been allocated.");
    }
  }

bail:
  return err;
}

core::Error VideoPanel::UnInitialize(core::ecs::registry::EntityRegistry* reg, cudaStream_t stream) {
  core::Error err = core::Error::SUCCESS;
  // Uninitialize resources if they exist
  for (auto& mapped_image : m_mappedImages) {
    core::Error err_tmp = m_mappedImages[mapped_image.first].UnInitialize(stream);
    if (err == core::Error::SUCCESS) {
      err = err_tmp;  // Save first error
    }
  }
  m_mappedImages.clear();
bail:
  return err;
}

core::Error VideoPanel::Render(core::ecs::registry::EntityRegistry* reg, cudaStream_t stream) {
  core::Error err = core::Error::SUCCESS;

  auto view = reg->view<commonmodule::components::VideoFrameComponent>();
  for (auto entity : view) {
    auto& video_frame = entity.GetComponent<commonmodule::components::VideoFrameComponent>();
    if (m_mappedImages.find(entity) != m_mappedImages.end()) {
      MappedImage& mapped_image = m_mappedImages[entity];
      NvCVImage* image_rgba = video_frame.GetRgbaImagePtr();
      if (!video_frame.ShouldShowVideo()) {
        continue;
      }
      if (!mapped_image.initialized &&
          mapped_image.Initialize(video_frame.GetImagePtr(), nullptr) != core::Error::SUCCESS) {
        LOG_DEBUG("Initialization of mapped image failed. Image has not been allocated.");
        continue;
      }

      if (ImGui::Begin((std::string("Video : ") + entity.GetComponent<std::string>()).c_str())) {
        video_frame.SetShouldUpdateRgbaImage(true);
        NvCVImage dst;
        NvCVImage_Init(&dst, mapped_image.img_src->width, mapped_image.img_src->height, mapped_image.img_src->pitch,
                       mapped_image.cuda_array, NVCV_RGBA, mapped_image.img_src->componentType,
                       mapped_image.img_src->planar, NVCV_CUDA_ARRAY);
        CHECK_NVCV_SUCCESS(NvCVImage_Transfer(image_rgba, &dst, 1.0f, stream, nullptr));

        ImVec2 size = ImGui::GetItemRectSize();
        size.x -= 20;  // To roughly center. Required due to left margin
        size.y = size.x * (static_cast<float>(mapped_image.img_src->height) / mapped_image.img_src->width);
        ImGui::Image(reinterpret_cast<void*>((intptr_t)mapped_image.tex_id), size);
        ImGui::End();
      } else {
        video_frame.SetShouldUpdateRgbaImage(false);
        ImGui::End();
      }
    }
  }
bail:
  return err;
}

void VideoPanel::ConfigDoNotUpdate(core::ecs::registry::EntityRegistry* reg) {
  auto view = reg->view<commonmodule::components::VideoFrameComponent>();
  for (auto entity : view) {
    auto& video_frame = entity.GetComponent<commonmodule::components::VideoFrameComponent>();
    if (m_mappedImages.find(entity) != m_mappedImages.end()) {
      video_frame.SetShouldUpdateRgbaImage(false);
    }
  }
}

}  // namespace panels
}  // namespace guimodule
}  // namespace modules
}  // namespace nv3dvc
