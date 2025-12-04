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

#include "Renderer.h"

#include <memory>
#include <string>

#include "Core/Engine/Engine.h"
#include "Core/Error.h"
#include "Core/Rendering/Display/Display.h"
#include "Core/Rendering/Shader.h"
#include "Core/Util/Logger.h"
#include "Modules/CommonModule/Components/CameraComponent.h"
#include "cuda.h"
#include "cudaGL.h"
#include "cudaGLTypedefs.h"
#include "cuda_gl_interop.h"
#include "cuda_runtime.h"
#include "cuda_runtime_api.h"
#include "driver_types.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "nvAR.h"
#include "nvARVolumetricRendering.h"
#include "nvCVStatus.h"
#include "nvCVTriplaneVolume.h"
#include "nvCVVolumeDefs.h"

namespace nv3dvc {
namespace core {
namespace rendering {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Macros                                                                                                           ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Static function declarations                                                                                     ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Convert cudaError_t to NvCV_Status
/// @param[in] cu_err CUDA error
/// @return           The corresponding NvCV_Status code
static NvCV_Status StatusFromCuda(cudaError_t cu_err);

/// @brief Converts a vector of OpenCV style camera intrinsics matrices to OpenGL style camera perspective matrices
///
/// See modules::commonmodule::components::CameraComponent::CameraIntrinsicsToOpenGlPerspectiveProjectionMatrix
///
/// @param[in] camera_intrinsics_vec vector of camera intrinsics objects
/// @param[in] width_pixels The horizontal display size in pixels
/// @param[in] height_pixels The vertical display size in pixels
/// @param[in] hither The distance to the resulting near plane
/// @param[in] yon The distance to the resulting far plane
/// @return The OpenGL projection matrices corresponding to the input
std::vector<glm::fmat4> CameraIntrinsicParamsToOpenGlProjectionMatrices(
    const std::vector<NvAR_RenderCameraIntrinsicParams>& camera_intrinsics_vec, unsigned int width_pixels,
    unsigned int height_pixels, float hither, float yon);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Static function definitions                                                                                      ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static NvCV_Status StatusFromCuda(const cudaError_t cu_err) {
  return (cudaSuccess == cu_err)
             ? NVCV_SUCCESS
             : static_cast<NvCV_Status>((static_cast<int>(NVCV_ERR_CUDA_BASE) - static_cast<int>(cu_err)));
}

std::vector<glm::fmat4> CameraIntrinsicParamsToOpenGlProjectionMatrices(
    const std::vector<NvAR_RenderCameraIntrinsicParams>& camera_intrinsics_vec, const unsigned int width_pixels,
    const unsigned int height_pixels, const float hither, const float yon) {
  std::vector<glm::fmat4> res;
  res.reserve(camera_intrinsics_vec.size());
  for (const auto& camera_intrinsic_params : camera_intrinsics_vec) {
    res.push_back(
        modules::commonmodule::components::CameraComponent::CameraIntrinsicsToOpenGlPerspectiveProjectionMatrix(
            camera_intrinsic_params.fx, camera_intrinsic_params.fy, camera_intrinsic_params.cx,
            camera_intrinsic_params.cy, width_pixels, height_pixels, hither, yon));
  }
  return res;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Member function definitions                                                                                      ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Renderer::Renderer(const std::string& name)
    : m_name(name),
      m_foregroundFbo(0),
      m_foregroundColorTextureId(0),
      m_foregroundDepthTextureId(0),
      m_numViews(1),
      m_framebufferWidth(0),
      m_framebufferHeight(0),
      m_renderWidth(0),
      m_renderHeight(0),
      m_quiltCols(0),
      m_quiltRows(0),
      m_cameraRoot(glm::mat4(1.0f)),
      m_renderHandle(nullptr),
      m_renderedImageTexResource(nullptr),
      m_downsampleFbo(0),
      m_compositeFbo(0),
      m_compositeColorTextureId(0),
      m_postProcFbo(0),
      m_postProcColorTextureId(0),
      m_cubeMapShadingProgram(nullptr),
      m_unlitMeshShadingProgram(nullptr),
      m_blitShadingProgram(nullptr),
      m_downsampleShadingProgram(nullptr),
      m_compositeShadingProgram(nullptr),
      m_vignetteShadingProgram(nullptr),
      m_emptyVao(0) {}

Error Renderer::Initialize(const unsigned render_width, const unsigned render_height, const unsigned num_views,
                           const unsigned quilt_cols, const unsigned quilt_rows) {
  Error err = Error::SUCCESS;
  m_renderWidth = render_width;
  m_renderHeight = render_height;
  m_quiltCols = quilt_cols;
  m_quiltRows = quilt_rows;
  m_framebufferWidth = render_width * quilt_cols;
  m_framebufferHeight = render_height * quilt_rows;

  // Create and load the render feature
  CHECK_NVCV_SUCCESS(NvAR_Create(NvAR_Feature_VolumetricRendering, &m_renderHandle));
  CHECK_NVCV_SUCCESS(NvAR_SetString(m_renderHandle, NvAR_Parameter_Config(ModelDir), ar_sdk_model_dir.get()->c_str()));
  CHECK_NVCV_SUCCESS(NvAR_SetU32(m_renderHandle, NvAR_Parameter_Config(RenderWidth), m_renderWidth));
  CHECK_NVCV_SUCCESS(NvAR_SetU32(m_renderHandle, NvAR_Parameter_Config(RenderHeight), m_renderHeight));
  // Using shared GL context. The feature should not create one
  CHECK_NVCV_SUCCESS(NvAR_SetU32(m_renderHandle, NvAR_Parameter_Config(CreateInternalGLContext), 0));

  m_numViews = num_views;

  m_cubeMapCameraProjectionMatrices.resize(m_numViews);
  m_viewports.resize(m_numViews);

  CHECK_NVCV_SUCCESS(NvAR_SetU32(m_renderHandle, NvAR_Parameter_Config(QuiltGridCols), quilt_cols));
  CHECK_NVCV_SUCCESS(NvAR_SetU32(m_renderHandle, NvAR_Parameter_Config(QuiltGridRows), quilt_rows));
  CHECK_NVCV_SUCCESS(NvAR_SetU32(m_renderHandle, NvAR_Parameter_Config(NumViews), m_numViews));
  // Configure not to clear frame buffer. Instead we clear the frame buffer ourselves so that we can render a cubemap
  // before rendering the triplane volume objects
  CHECK_NVCV_SUCCESS(NvAR_SetU32(m_renderHandle, NvAR_Parameter_Config(ClearBuffer), 0));
  CHECK_NVCV_SUCCESS(NvAR_Load(m_renderHandle));
  // Internal GL objects
  CHECK_NVCV_SUCCESS(
      NvAR_GetU32(m_renderHandle, NvAR_Parameter_Config(ColorTextureAttachment), &m_foregroundColorTextureId));
  CHECK_NVCV_SUCCESS(
      NvAR_GetU32(m_renderHandle, NvAR_Parameter_Config(DepthTextureAttachment), &m_foregroundDepthTextureId));
  CHECK_NVCV_SUCCESS(NvAR_GetU32(m_renderHandle, NvAR_Parameter_Config(FrameBufferObject), &m_foregroundFbo));
  CHECK_NVCV_SUCCESS(NvAR_GetU32(m_renderHandle, NvAR_Parameter_Output(FrameBufferWidth), &m_framebufferWidth));
  CHECK_NVCV_SUCCESS(NvAR_GetU32(m_renderHandle, NvAR_Parameter_Output(FrameBufferHeight), &m_framebufferHeight));

  CHECK_TRUE(m_foregroundFbo != 0, core::Error::ERR_GL_INVALID_FRAMEBUFFER_OPERATION);

  // Allocate mipmap for the foreground texture, for compositing and drop shadow.
  CHECK_GLGETERROR(glBindTexture(GL_TEXTURE_2D, m_foregroundColorTextureId));
  CHECK_GLGETERROR(glGenerateMipmap(GL_TEXTURE_2D));
  CHECK_GLGETERROR(glBindTexture(GL_TEXTURE_2D, 0));
  // Create FBO for generating the mipmap levels.
  CHECK_GLGETERROR(glGenFramebuffers(1, &m_downsampleFbo));

  CHECK_SUCCESS(CreateFbo(&m_compositeFbo, &m_compositeColorTextureId));
  CHECK_SUCCESS(CreateFbo(&m_postProcFbo, &m_postProcColorTextureId));
  CHECK_SUCCESS(CompileShaders());

  CHECK_GLGETERROR(glFrontFace(GL_CCW));
  CHECK_GLGETERROR(glEnable(GL_CULL_FACE));
  CHECK_GLGETERROR(glCullFace(GL_BACK));
  CHECK_GLGETERROR(glGenVertexArrays(1, &m_emptyVao));
  CHECK_GLGETERROR(glBindVertexArray(m_emptyVao));

  {
    auto update_tex_resource_fn = [this]() {
      cudaError_t cu_err = cudaSuccess;
      if (m_renderedImageTexResource) {
        cu_err = cudaGraphicsUnregisterResource(m_renderedImageTexResource);
        if (cu_err != CUDA_SUCCESS) LOG_ERROR("Failed to unregister CUDA graphics resource");
        m_renderedImageTexResource = nullptr;
      }
      // Register CUDA resource for output rendered image
      auto texture = apply_vignette ? m_postProcColorTextureId : m_compositeColorTextureId;
      cu_err = cudaGraphicsGLRegisterImage(&m_renderedImageTexResource, texture, GL_TEXTURE_2D,
                                           cudaGraphicsRegisterFlagsReadOnly);
      if (cu_err != CUDA_SUCCESS) LOG_ERROR("Failed to register CUDA graphics resource");
    };
    apply_vignette.SetOnChangeFunction(update_tex_resource_fn);
    // Register the resource
    update_tex_resource_fn();
  }

bail:
  if (core::Error::SUCCESS != err)
    LOG_DEBUG("Failed to initialize NvCVRenderVolume with model dir \"%s\"", ar_sdk_model_dir.get()->c_str());
  return err;
}

Error Renderer::ClearFbo(GLuint fbo, const glm::vec4& clear_color) {
  Error err = Error::SUCCESS;

  CHECK_GLGETERROR(glBindFramebuffer(GL_FRAMEBUFFER, fbo));
  CHECK_GLGETERROR(glViewport(0, 0, m_framebufferWidth, m_framebufferHeight));
  CHECK_GLGETERROR(glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a));
  CHECK_GLGETERROR(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
bail:
  return err;
}

Error Renderer::Clear() {
  Error err = Error::SUCCESS;

  if (m_foregroundFbo) CHECK_SUCCESS(ClearFbo(m_foregroundFbo, glm::vec4(0.0f)));
  if (m_compositeFbo) CHECK_SUCCESS(ClearFbo(m_compositeFbo, glm::vec4(0.2f, 0.2f, 0.2f, 1.0f)));
  if (m_postProcFbo) CHECK_SUCCESS(ClearFbo(m_postProcFbo, glm::vec4(0.2f, 0.2f, 0.2f, 1.0f)));

  // Clear default FBO
  CHECK_SUCCESS(ClearFbo(0, glm::vec4(0.2f, 0.2f, 0.2f, 1.0f)));

bail:
  return err;
}

Error Renderer::BeginFrame(const glm::mat4& camera_view_matrix, const std::vector<glm::mat4>& view_matrices,
                           const std::vector<NvAR_RenderCameraIntrinsicParams>& camera_intrinsics) {
  Error err = Error::SUCCESS;

  m_cameraRoot = camera_view_matrix;
  m_cameraRoot = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f) * *scale_factor.get()) * m_cameraRoot;
  m_cameraExtrinsicsMatrices = view_matrices;
  std::vector<NvAR_RenderCameraIntrinsicParams> cube_map_intrinsics_vec = camera_intrinsics;

  CHECK_NVCV_SUCCESS(NvAR_SetF32Array(m_renderHandle, NvAR_Parameter_Input(CameraRoot), &m_cameraRoot[0][0], 16));

  CHECK_NVCV_SUCCESS(NvAR_SetObject(m_renderHandle, NvAR_Parameter_Input(CameraIntrinsics),
                                    const_cast<NvAR_RenderCameraIntrinsicParams*>(camera_intrinsics.data()),
                                    m_numViews * sizeof(NvAR_RenderCameraIntrinsicParams)));
  CHECK_NVCV_SUCCESS(NvAR_SetF32Array(m_renderHandle, NvAR_Parameter_Input(CameraExtrinsics),
                                      const_cast<float*>(&view_matrices[0][0][0]), view_matrices.size() * 16));

  m_cameraProjectionMatrices =
      CameraIntrinsicParamsToOpenGlProjectionMatrices(camera_intrinsics, m_renderWidth, m_renderHeight, 0.1f, 20.0f);

  // Reduce the amount of parallax for cube map rendering.
  // WARNING: This may lead to inwanted artifacts due to physically incorrect parallax on object rendered at infinity
  for (NvAR_RenderCameraIntrinsicParams& cube_map_intrinsics : cube_map_intrinsics_vec) {
    float cx_diff = cube_map_intrinsics.cx - static_cast<float>(m_renderWidth) * 0.5;
    float cy_diff = cube_map_intrinsics.cy - static_cast<float>(m_renderHeight) * 0.5;
    cx_diff *= cubemap_parallax_factor;
    cy_diff *= cubemap_parallax_factor;
    cube_map_intrinsics.cx = static_cast<float>(m_renderWidth) * 0.5 + cx_diff;
    cube_map_intrinsics.cy = static_cast<float>(m_renderHeight) * 0.5 + cy_diff;
    if (fix_cubemap_viewdistance) {
      float inv_aspect = cube_map_intrinsics.fy / cube_map_intrinsics.fx;
      cube_map_intrinsics.fx = *cubemap_viewdistance_pixels.get() * inv_aspect;
      cube_map_intrinsics.fy = cubemap_viewdistance_pixels;
    }
  }
  m_cubeMapCameraProjectionMatrices = CameraIntrinsicParamsToOpenGlProjectionMatrices(
      cube_map_intrinsics_vec, m_renderWidth, m_renderHeight, 0.1f, 20.0f);

bail:
  return err;
}

Error Renderer::EndFrame() {
  Error err = Error::SUCCESS;
  CHECK_GLGETERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));
bail:
  return err;
}

core::Error Renderer::DrawTriplaneModel(NvCVTriplaneVolume* triplane_volume, const glm::mat4& model_transform) {
  Error err = Error::SUCCESS;

  CHECK_NVCV_SUCCESS(NvAR_SetObject(m_renderHandle, NvAR_Parameter_Input(TriplaneVolume), triplane_volume,
                                    sizeof(NvCVTriplaneVolume)));
  CHECK_NVCV_SUCCESS(NvAR_SetF32Array(m_renderHandle, NvAR_Parameter_Input(ModelTransform),
                                      const_cast<float*>(&model_transform[0][0]), 16));
  CHECK_NVCV_SUCCESS(NvAR_SetF32(m_renderHandle, NvAR_Parameter_Config(QualityFactor), quality_factor));
  CHECK_NVCV_SUCCESS(NvAR_Run(m_renderHandle));

  if (apply_shadow || sharpen_foreground != 0.0f) {
    CHECK_SUCCESS(GenerateForegroundMipMap());
  }

bail:
  return err;
}

core::Error Renderer::GenerateForegroundMipMap() {
  Error err = Error::SUCCESS;

  int prev_level = 0, prev_width = m_framebufferWidth, prev_height = m_framebufferHeight;

  // Create frame buffer object.
  CHECK_GLGETERROR(glBindFramebuffer(GL_FRAMEBUFFER, m_downsampleFbo));
  CHECK_GLGETERROR(glDisable(GL_DEPTH_TEST));
  CHECK_GLGETERROR(glDisable(GL_BLEND));
  CHECK_GLGETERROR(m_downsampleShadingProgram->Activate());
  // Bind foreground texture to sample from.
  CHECK_GLGETERROR(glActiveTexture(GL_TEXTURE0));
  CHECK_GLGETERROR(glBindTexture(GL_TEXTURE_2D, m_foregroundColorTextureId));
  CHECK_GLGETERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
  CHECK_GLGETERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
  // Bind empty VAO for implicit full-screen triangle.
  CHECK_GLGETERROR(glBindVertexArray(m_emptyVao));
  // Generate each level by sampling the previous level.
  while (prev_width > 1 && prev_height > 1) {
    // Get current level size.
    const int level = prev_level + 1, width = prev_width / 2, height = prev_height / 2;
    // Attach current texture level to FBO as first color attachment.
    CHECK_GLGETERROR(
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_foregroundColorTextureId, level));
    // Draw, sampling prev_level to generate level.
    CHECK_GLGETERROR(m_downsampleShadingProgram->SetUniform("prevLevel", static_cast<float>(prev_level)));
    CHECK_GLGETERROR(m_downsampleShadingProgram->SetUniform("invPrevSize", 1.0f / prev_width, 1.0f / prev_height));
    CHECK_GLGETERROR(glViewport(0, 0, width, height));
    CHECK_GLGETERROR(glDrawArrays(GL_TRIANGLES, 0, 3));  // draw a single triangle
    // Move curr to prev.
    prev_level = level, prev_width = width, prev_height = height;
  }
  CHECK_GLGETERROR(m_downsampleShadingProgram->Deactivate());

  CHECK_GLGETERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));
  CHECK_GLGETERROR(glBindTexture(GL_TEXTURE_2D, 0));

bail:
  return err;
}

core::Error Renderer::DrawCubeMapModel(CubeMapModel* cube_map_model, const glm::mat4& model_transform,
                                       const bool at_infinity) {
  Error err = Error::SUCCESS;
  CHECK_GLGETERROR(glBindFramebuffer(GL_FRAMEBUFFER, m_compositeFbo));
  CHECK_GLGETERROR(glEnable(GL_SCISSOR_TEST));
  for (int view_idx = 0; view_idx < m_numViews; view_idx++) {
    int x = (view_idx % m_quiltCols) * m_renderWidth;
    int y = (view_idx / m_quiltCols) * m_renderHeight;
    CHECK_GLGETERROR(glViewport(x, y, m_renderWidth, m_renderHeight));
    CHECK_GLGETERROR(glScissor(x, y, m_renderWidth, m_renderHeight));

    Shader* shader = m_cubeMapShadingProgram.get();
    // render triangle-based models
    shader->Activate();
    glm::mat4 camera_view_matrix = m_cameraExtrinsicsMatrices[view_idx] * m_cameraRoot;
    if (at_infinity) camera_view_matrix = glm::mat4(glm::mat3(camera_view_matrix));
    CHECK_GLGETERROR(shader->SetUniform("viewMatrix", &camera_view_matrix));
    CHECK_GLGETERROR(shader->SetUniform("projectionMatrix", &m_cubeMapCameraProjectionMatrices[view_idx]));

    cube_map_model->Draw(shader, model_transform);

    shader->Deactivate();
  }
  CHECK_GLGETERROR(glDisable(GL_SCISSOR_TEST));
bail:
  return err;
}

core::Error Renderer::DrawUnlitMesh(MeshModel* mesh_model, const glm::mat4& model_transform) {
  Error err = Error::SUCCESS;
  CHECK_GLGETERROR(glBindFramebuffer(GL_FRAMEBUFFER, m_compositeFbo));
  CHECK_GLGETERROR(glEnable(GL_SCISSOR_TEST));
  for (int view_idx = 0; view_idx < m_numViews; view_idx++) {
    int x = (view_idx % m_quiltCols) * m_renderWidth;
    int y = (view_idx / m_quiltCols) * m_renderHeight;
    CHECK_GLGETERROR(glViewport(x, y, m_renderWidth, m_renderHeight));
    CHECK_GLGETERROR(glScissor(x, y, m_renderWidth, m_renderHeight));

    // render triangle-based models
    m_unlitMeshShadingProgram->Activate();
    glm::mat4 camera_view_matrix = m_cameraExtrinsicsMatrices[view_idx] * m_cameraRoot;
    CHECK_GLGETERROR(m_unlitMeshShadingProgram->SetUniform("viewMatrix", &camera_view_matrix));
    CHECK_GLGETERROR(m_unlitMeshShadingProgram->SetUniform("projectionMatrix", &m_cameraProjectionMatrices[view_idx]));

    mesh_model->Draw(m_unlitMeshShadingProgram.get(), model_transform);
  }
  m_unlitMeshShadingProgram->Deactivate();

  CHECK_GLGETERROR(glDisable(GL_SCISSOR_TEST));
bail:
  return err;
}

Error Renderer::CompositeForegroundBackground() {
  Error err = Error::SUCCESS;

  // Remember whether GL_BLEND is enabled before we modify it.
  const bool blend_enabled = glIsEnabled(GL_BLEND);

  // Bind display frame buffer
  CHECK_GLGETERROR(glBindFramebuffer(GL_FRAMEBUFFER, m_compositeFbo));

  CHECK_GLGETERROR(glViewport(0, 0, m_framebufferWidth, m_framebufferHeight));
  CHECK_GLGETERROR(glDisable(GL_DEPTH_TEST));
  // Set blend func for Composite.frag, which outputs RGBA with pre-multiplied alpha.
  CHECK_GLGETERROR(glEnable(GL_BLEND));
  CHECK_GLGETERROR(glBlendEquation(GL_FUNC_ADD));
  CHECK_GLGETERROR(glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA));

  // activate blit shader
  CHECK_GLGETERROR(m_compositeShadingProgram->Activate());
  CHECK_GLGETERROR(glActiveTexture(GL_TEXTURE0));
  CHECK_GLGETERROR(glBindTexture(GL_TEXTURE_2D, m_foregroundColorTextureId));
  // Set the foreground texture parameters for compositing.
  CHECK_GLGETERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
  CHECK_GLGETERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
  CHECK_GLGETERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
  CHECK_GLGETERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

  CHECK_GLGETERROR(m_compositeShadingProgram->SetUniform("invShadowScale", 1.0f / shadow_scale));
  CHECK_GLGETERROR(m_compositeShadingProgram->SetUniform("shadowOpacity", apply_shadow ? shadow_opacity : 0.0f));
  CHECK_GLGETERROR(m_compositeShadingProgram->SetUniform("shadowLod", shadow_blur));
  CHECK_GLGETERROR(
      m_compositeShadingProgram->SetUniform("shadowOffset", shadow_offset.get()->x, shadow_offset.get()->y));
  CHECK_GLGETERROR(m_compositeShadingProgram->SetUniform("sharpenForeground", sharpen_foreground));

  // run blit shader -> does not need any VAO since vertex positions are computed directly from vertex ids from a
  // single triangle
  CHECK_GLGETERROR(glBindVertexArray(m_emptyVao));
  CHECK_GLGETERROR(glDrawArrays(GL_TRIANGLES, 0, 3));  // draw a single triangle

  CHECK_GLGETERROR(m_compositeShadingProgram->Deactivate());
  CHECK_GLGETERROR(glEnable(GL_DEPTH_TEST));
  if (!blend_enabled) {
    CHECK_GLGETERROR(glDisable(GL_BLEND));
  }

  // deactivate texture units
  CHECK_GLGETERROR(glActiveTexture(GL_TEXTURE0));
  CHECK_GLGETERROR(glBindTexture(GL_TEXTURE_2D, 0));

bail:
  return err;
}

Error Renderer::RenderVignette() {
  Error err = Error::SUCCESS;
  if (!apply_vignette) {
    return Error::SUCCESS;
  }

  Shader* vignette_shader = m_vignetteShadingProgram.get();

  CHECK_GLGETERROR(glBindFramebuffer(GL_FRAMEBUFFER, m_postProcFbo));
  CHECK_GLGETERROR(glViewport(0, 0, m_framebufferWidth, m_framebufferHeight));
  CHECK_GLGETERROR(glDisable(GL_DEPTH_TEST));

  vignette_shader->Activate();
  CHECK_GLGETERROR(vignette_shader->SetUniform("vignetteIntensity", vignette_intensity));

  CHECK_GLGETERROR(glActiveTexture(GL_TEXTURE0));
  CHECK_GLGETERROR(glBindTexture(GL_TEXTURE_2D, m_compositeColorTextureId));

  // run blit shader -> does not need any VAO since vertex positions are computed directly from vertex ids from a
  // single triangle
  CHECK_GLGETERROR(glBindVertexArray(m_emptyVao));
  CHECK_GLGETERROR(glDrawArrays(GL_TRIANGLES, 0, 3));  // draw a single triangle
  vignette_shader->Deactivate();

  CHECK_GLGETERROR(glEnable(GL_DEPTH_TEST));  // re-enable depth test
  // deactivate texture units
  CHECK_GLGETERROR(glActiveTexture(GL_TEXTURE0));
  CHECK_GLGETERROR(glBindTexture(GL_TEXTURE_2D, 0));
bail:
  return err;
}

Error Renderer::Uninitialize() {
  Error err = Error::SUCCESS;
  CHECK_SUCCESS(DestroyFbo(&m_downsampleFbo, nullptr));
  CHECK_SUCCESS(DestroyFbo(&m_compositeFbo, &m_compositeColorTextureId));
  CHECK_SUCCESS(DestroyFbo(&m_postProcFbo, &m_postProcColorTextureId));
  if (m_renderedImageTexResource) {
    CHECK_CUDA_SUCCESS(cudaGraphicsUnregisterResource(m_renderedImageTexResource));
    m_renderedImageTexResource = nullptr;
  }
  if (m_renderHandle) {
    CHECK_NVCV_SUCCESS(NvAR_Destroy(m_renderHandle));
    m_renderHandle = nullptr;
  }
  if (m_emptyVao) {
    CHECK_GLGETERROR(glDeleteVertexArrays(1, &m_emptyVao));
    m_emptyVao = 0;
  }
  m_shaders.clear();
  m_cubeMapShadingProgram = nullptr;
  m_unlitMeshShadingProgram = nullptr;
  m_blitShadingProgram = nullptr;
  m_downsampleShadingProgram = nullptr;
  m_compositeShadingProgram = nullptr;
  m_vignetteShadingProgram = nullptr;

bail:
  return err;
}

uint32_t Renderer::GetRenderWidth() const { return m_renderWidth; }

uint32_t Renderer::GetRenderHeight() const { return m_renderHeight; }

uint32_t Renderer::NumViews() const { return m_numViews; }

uint32_t Renderer::GetFramebufferWidth() const { return m_framebufferWidth; }

uint32_t Renderer::GetFramebufferHeight() const { return m_framebufferHeight; }

Error Renderer::CreateFbo(GLuint* fbo, GLuint* texture_id) {
  Error err = Error::SUCCESS;

  // Create frame buffer object.
  CHECK_GLGETERROR(glGenFramebuffers(1, fbo));
  CHECK_GLGETERROR(glBindFramebuffer(GL_FRAMEBUFFER, *fbo));

  // Create texture.
  CHECK_GLGETERROR(glGenTextures(1, texture_id));
  CHECK_GLGETERROR(glBindTexture(GL_TEXTURE_2D, *texture_id));
  CHECK_GLGETERROR(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, m_framebufferWidth, m_framebufferHeight, 0, GL_RGB,
                                GL_UNSIGNED_BYTE, NULL));

  // Set texture parameters.
  CHECK_GLGETERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
  CHECK_GLGETERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
  CHECK_GLGETERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
  CHECK_GLGETERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));

  CHECK_GLGETERROR(glBindTexture(GL_TEXTURE_2D, 0));  // unbind

  // Attach texture to FBO as first color attachment.
  CHECK_GLGETERROR(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *texture_id, 0));

  // Check FBO status.
  CHECK_TRUE(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
             core::ERR_GL_INVALID_FRAMEBUFFER_OPERATION);

  // Unbind FBO.
  CHECK_GLGETERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));

bail:
  return err;
}

Error Renderer::DestroyFbo(GLuint* fbo, GLuint* texture_id) {
  Error err = Error::SUCCESS;
  GLenum glerr = GL_NO_ERROR;

  if (fbo && *fbo != 0) {
    glDeleteFramebuffers(1, fbo);
    if (glerr == GL_NO_ERROR) glerr = glGetError();  // Preserve first error.
    *fbo = 0;
  }

  if (texture_id && *texture_id != 0) {
    glDeleteTextures(1, texture_id);
    if (glerr == GL_NO_ERROR) glerr = glGetError();  // Preserve first error.
    *texture_id = 0;
  }

bail:
  return err;
}

Error Renderer::CompileShaders() {
  Error err = Error::SUCCESS;
  m_shaders.clear();
  // Blit shader
  std::string vert_source =
#include "Core/Rendering/Shaders/Blit.vert"  // NOLINT(build/include) (No limit in shader inclusions)
      ;                                      // NOLINT(whitespace/semicolon) (Needed for string literals)
  std::string frag_source =
#include "Core/Rendering/Shaders/Blit.frag"  // NOLINT(build/include) (No limit in shader inclusions)
      ;                                      // NOLINT(whitespace/semicolon) (Needed for string literals)
  m_blitShadingProgram = std::make_shared<Shader>();
  err = m_blitShadingProgram->CreateShaderFromSource(vert_source, frag_source);
  CHECK_SUCCESS(err, "Failed to create Blit shader");
  m_shaders.push_back(m_blitShadingProgram);

  // Vignette texture shader
  vert_source =
#include "Core/Rendering/Shaders/Blit.vert"  // NOLINT(build/include) (No limit in shader inclusions)
      ;                                      // NOLINT(whitespace/semicolon) (Needed for string literals)
  frag_source =
#include "Core/Rendering/Shaders/Vignette.frag"  // NOLINT(build/include) (No limit in shader inclusions)
      ;                                          // NOLINT(whitespace/semicolon) (Needed for string literals)
  m_vignetteShadingProgram = std::make_shared<Shader>();
  err = m_vignetteShadingProgram->CreateShaderFromSource(vert_source, frag_source);
  CHECK_SUCCESS(err, "Failed to create Vignette shader");
  m_shaders.push_back(m_vignetteShadingProgram);
  m_vignetteShadingProgram->Activate();
  m_vignetteShadingProgram->SetUniform("quiltTiling", static_cast<float>(m_quiltCols), static_cast<float>(m_quiltRows));
  m_vignetteShadingProgram->Deactivate();

  // Cubemap shader
  vert_source =
#include "Core/Rendering/Shaders/CubeMap.vert"  // NOLINT(build/include) (No limit in shader inclusions)
      ;                                         // NOLINT(whitespace/semicolon) (Needed for string literals)
  frag_source =
#include "Core/Rendering/Shaders/CubeMap.frag"  // NOLINT(build/include) (No limit in shader inclusions)
      ;                                         // NOLINT(whitespace/semicolon) (Needed for string literals)
  m_cubeMapShadingProgram = std::make_shared<Shader>();
  err = m_cubeMapShadingProgram->CreateShaderFromSource(vert_source, frag_source);
  CHECK_SUCCESS(err, "Failed to create CubeMap shader");
  m_shaders.push_back(m_cubeMapShadingProgram);

  // UnlitMesh shader
  vert_source =
#include "Core/Rendering/Shaders/UnlitMesh.vert"  // NOLINT(build/include) (No limit in shader inclusions)
      ;                                           // NOLINT(whitespace/semicolon) (Needed for string literals)
  frag_source =
#include "Core/Rendering/Shaders/UnlitMesh.frag"  // NOLINT(build/include) (No limit in shader inclusions)
      ;                                           // NOLINT(whitespace/semicolon) (Needed for string literals)
  m_unlitMeshShadingProgram = std::make_shared<Shader>();
  err = m_unlitMeshShadingProgram->CreateShaderFromSource(vert_source, frag_source);
  CHECK_SUCCESS(err, "Failed to create UnlitMesh shader");
  m_shaders.push_back(m_unlitMeshShadingProgram);

  // Downsample shader
  vert_source =
#include "Core/Rendering/Shaders/Blit.vert"  // NOLINT(build/include) (No limit in shader inclusions)
      ;                                      // NOLINT(whitespace/semicolon) (Needed for string literals)
  frag_source =
#include "Core/Rendering/Shaders/Downsample.frag"  // NOLINT(build/include) (No limit in shader inclusions)
      ;                                            // NOLINT(whitespace/semicolon) (Needed for string literals)
  m_downsampleShadingProgram = std::make_shared<Shader>();
  err = m_downsampleShadingProgram->CreateShaderFromSource(vert_source, frag_source);
  CHECK_SUCCESS(err, "Failed to create Downsample shader");
  m_shaders.push_back(m_downsampleShadingProgram);

  // Composite shader
  vert_source =
#include "Core/Rendering/Shaders/Blit.vert"  // NOLINT(build/include) (No limit in shader inclusions)
      ;                                      // NOLINT(whitespace/semicolon) (Needed for string literals)
  frag_source =
#include "Core/Rendering/Shaders/Composite.frag"  // NOLINT(build/include) (No limit in shader inclusions)
      ;                                           // NOLINT(whitespace/semicolon) (Needed for string literals)
  m_compositeShadingProgram = std::make_shared<Shader>();
  err = m_compositeShadingProgram->CreateShaderFromSource(vert_source, frag_source);
  CHECK_SUCCESS(err, "Failed to create Composite shader");
  m_shaders.push_back(m_compositeShadingProgram);

bail:
  return err;
}

Error Renderer::ReadPixelsRgbaU8(uint8_t* pixels, const uint32_t x, const uint32_t y, const uint32_t width,
                                 const uint32_t height) const {
  Error err = Error::SUCCESS;
  auto fbo = apply_vignette ? m_postProcFbo : m_compositeFbo;

  CHECK_GLGETERROR(glBindFramebuffer(GL_FRAMEBUFFER, fbo));
  CHECK_GLGETERROR(glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels));
  CHECK_GLGETERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));
bail:
  return err;
}

Error Renderer::MapTextureResource(cudaArray** texture_array, cudaStream_t cu_stream) {
  Error err = Error::SUCCESS;
  CHECK_NONNULL(m_renderedImageTexResource, Error::ERR_INITIALIZATION);
  CHECK_CUDA_SUCCESS(cudaGraphicsMapResources(1, &m_renderedImageTexResource, cu_stream),
                     "Failed to map CUDA resource");
  // Copy from FBO texture to output image buffer
  CHECK_CUDA_SUCCESS(cudaGraphicsSubResourceGetMappedArray(texture_array, m_renderedImageTexResource, 0, 0),
                     "Failed to get mapped CUDA array");
bail:
  return err;
}

Error Renderer::UnMapTextureResource(cudaStream_t cu_stream) {
  Error err = Error::SUCCESS;
  CHECK_CUDA_SUCCESS(cudaGraphicsUnmapResources(1, &m_renderedImageTexResource, cu_stream),
                     "Failed to unmap CUDA resource");
bail:
  return err;
}

Error Renderer::GetRenderVolumeFrameBufferObject(uint32_t* fbo) {
  Error err = Error::SUCCESS;
  CHECK_NVCV_SUCCESS(NvAR_GetU32(m_renderHandle, NvAR_Parameter_Config(FrameBufferObject), fbo));
bail:
  return err;
}

Error Renderer::GetRenderVolumeColorTextureAttachment(uint32_t* color_texture) {
  Error err = Error::SUCCESS;
  CHECK_NVCV_SUCCESS(NvAR_GetU32(m_renderHandle, NvAR_Parameter_Config(ColorTextureAttachment), color_texture));
bail:
  return err;
}

Error Renderer::GetRenderVolumeDepthTextureAttachment(uint32_t* depth_texture) {
  Error err = Error::SUCCESS;
  CHECK_NVCV_SUCCESS(NvAR_GetU32(m_renderHandle, NvAR_Parameter_Config(DepthTextureAttachment), depth_texture));
bail:
  return err;
}

Error Renderer::BlitToDisplayBuffer(display::Display* display, const uint32_t x, const uint32_t y, const uint32_t width,
                                    const uint32_t height) {
  Error err = Error::SUCCESS;

  auto fbo = apply_vignette ? m_postProcFbo : m_compositeFbo;
  auto color_texture_id = apply_vignette ? m_postProcColorTextureId : m_compositeColorTextureId;

  // Bind display frame buffer
  CHECK_GLGETERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));
  CHECK_GLGETERROR(glViewport(x, y, width, height));

  // Try blit with display implementation first. If not successful, use default blit
  if (!display || !display->BlitToDisplayBuffer(fbo, color_texture_id)) {
    // Clear the color and depth buffer
    CHECK_GLGETERROR(glClearColor(0.0, 0.0, 0.0, 1.0f));
    CHECK_GLGETERROR(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    CHECK_GLGETERROR(glDisable(GL_DEPTH_TEST));

    // activate blit shader
    CHECK_GLGETERROR(m_blitShadingProgram->Activate());
    CHECK_GLGETERROR(glActiveTexture(GL_TEXTURE0));
    CHECK_GLGETERROR(glBindTexture(GL_TEXTURE_2D, color_texture_id));

    // run blit shader -> does not need any VAO since vertex positions are computed directly from vertex ids from a
    // single triangle
    CHECK_GLGETERROR(glBindVertexArray(m_emptyVao));
    CHECK_GLGETERROR(glDrawArrays(GL_TRIANGLES, 0, 3));  // draw a single triangle
    CHECK_GLGETERROR(m_blitShadingProgram->Deactivate());
    CHECK_GLGETERROR(glEnable(GL_DEPTH_TEST));

    // deactivate texture units
    CHECK_GLGETERROR(glActiveTexture(GL_TEXTURE0));
    CHECK_GLGETERROR(glBindTexture(GL_TEXTURE_2D, 0));
  }
bail:
  return err;
}

}  // namespace rendering
}  // namespace core
}  // namespace nv3dvc
