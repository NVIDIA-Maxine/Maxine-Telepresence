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

#ifndef SRC_CORE_RENDERING_RENDERER_H_
#define SRC_CORE_RENDERING_RENDERER_H_

#include <memory>
#include <string>
#include <vector>

#include "Core/Engine/Engine.h"
#include "Core/Error.h"
#include "Core/Properties/Property.h"
#include "CubeMapModel.h"
#include "MeshModel.h"
#include "glm/glm.hpp"
#include "nvAR.h"
#include "nvCVTriplaneVolume.h"

// Forward declaration
class cudaGraphicsResource;

namespace nv3dvc {
namespace core {
namespace rendering {

// Forward declaration
class Shader;
namespace display {
class Display;
}

/// @defgroup RendererProperties Renderer
/// @ingroup  ClassProperties
/// @brief    Renderer
///

/// @see RendererProperties
class Renderer : public properties::PropertyOwner {
 public:
  /// @brief Constructor
  /// @param[in] name The name of the property owner
  explicit Renderer(const std::string& name);
  std::string Name() const override { return m_name; }

  /// @brief Initialize the renderer
  ///
  /// Sets up the render buffers and loads shaders.
  /// The resulting render buffer will have the size (render_width * quilt_cols) x (render_height * quilt_rows)
  /// @param[in] render_width  The render width in pixels
  /// @param[in] render_height The render height in pixels
  /// @param[in] num_views     The number of views. Must be <= quilt_rows * quilt_cols
  /// @param[in] quilt_cols    The number of quilt tiles in one row of the render buffer
  /// @param[in] quilt_rows    The number of quilt tiles in one column of the render buffer
  /// @return    Error::SUCCESS If successful
  Error Initialize(unsigned render_width, unsigned render_height, unsigned num_views, unsigned quilt_cols,
                   unsigned quilt_rows);

  /// @brief Uninitizlize the renderer
  /// @return Error::SUCCESS If successful
  Error Uninitialize();

  /// @brief Get the render width
  /// @return The render width
  uint32_t GetRenderWidth() const;

  /// @brief Get the render height
  /// @return The render height
  uint32_t GetRenderHeight() const;

  /// @brief Get the number of views
  /// @return The number of views
  uint32_t NumViews() const;

  /// @brief Get the full frame buffer width
  /// @return The full frame buffer width
  uint32_t GetFramebufferWidth() const;

  /// @brief Get the full frame buffer height
  /// @return The full frame buffer height
  uint32_t GetFramebufferHeight() const;

  /// @brief Clear the frame buffer
  /// @return Error::SUCCESS If successful
  Error Clear();

  /// @brief Prepare a new frame to be rendered
  ///
  /// Should be called once per frame before any object have been rendered. After objects have been rendered, call
  /// EndFrame
  /// @param[in] camera_view_matrix The camera root's view matrix
  /// @param[in] view_matrices      The camera array's view matrices. The cameras in the camera array are children of
  /// the camera root. Size should be 1 for mono view rendering, and > 1 for stereo view rendering.
  /// @param[in] camera_intrinsics  The camera array's intrinsics parameters. Size should match the one for
  /// view_matrices
  /// @return    Error::SUCCESS     If successful
  Error BeginFrame(const glm::mat4& camera_view_matrix, const std::vector<glm::mat4>& view_matrices,
                   const std::vector<NvAR_RenderCameraIntrinsicParams>& camera_intrinsics);

  /// @brief End the rendering of the current frame
  ///
  /// Requires one preceding call to BeginFrame
  /// @return Error::SUCCESS If successful
  Error EndFrame();

  /// @brief Draw a single triplane model
  ///
  /// Requires one preceding call to BeginFrame
  /// @param triplane_volume Pointer to the triplane volume to render
  /// @param model_transform The model transform of the object
  /// @return Error::SUCCESS If successful
  Error DrawTriplaneModel(NvCVTriplaneVolume* triplane_volume, const glm::mat4& model_transform);

  /// @brief Draw a single cube map model
  ///
  /// Requires one preceding call to BeginFrame
  /// @param cube_map_model  Pointer to the cube map model to render
  /// @param model_transform The model transform of the object
  /// @param at_infinity     Whether to draw cube map at infinity. If true, only the transform rotation will be used
  /// @return Error::SUCCESS If successful
  Error DrawCubeMapModel(CubeMapModel* cube_map_model, const glm::mat4& model_transform, bool at_infinity = true);

  /// @brief Draw a mesh model without lighting
  ///
  /// @param mesh_model      Pointer to the mesh model to render
  /// @param model_transform The model transform of the object
  /// @return Error::SUCCESS If successful
  Error DrawUnlitMesh(MeshModel* mesh_model, const glm::mat4& model_transform);

  /// @brief Composite the foreground over the background
  ///
  /// May also composite cast shadow if enabled
  /// @return Error::SUCCESS If successful
  Error CompositeForegroundBackground();

  /// @brief Render vignette
  ///
  /// Requires one preceding call to BeginFrame
  /// @return Error::SUCCESS If successful
  Error RenderVignette();

  /// @brief Blit to display buffer
  ///
  /// Should be called after EndFrame
  /// @param display The display, which may be able to perform a blit operation
  /// @param x       Viewport x start position, pixels
  /// @param y       Viewport y start position, pixels
  /// @param width   Viewport width, pixels
  /// @param height  Viewport height, pixels
  /// @return Error::SUCCESS If successful
  Error BlitToDisplayBuffer(display::Display* display, uint32_t x, uint32_t y, uint32_t width, uint32_t height);

  /// @brief Read pixels from buffer to CPU memory
  ///
  /// Should be called after EndFrame
  /// @param pixels Pointer to the buffer in memory, which must be large enough to hold width * height * 4 values.
  /// @param x      Viewport x start position, pixels
  /// @param y      Viewport y start position, pixels
  /// @param width  Viewport width, pixels
  /// @param height Viewport height, pixels
  /// @return Error::SUCCESS If successful
  Error ReadPixelsRgbaU8(uint8_t* pixels, uint32_t x, uint32_t y, uint32_t width, uint32_t height) const;

  /// @brief Map a CUDA texture resource used for reading pixels from the main GL render buffer
  ///
  /// @param[in] texture_array An address to a CUDA texture array which is linked to the frame buffer object to read
  /// @param[in] cu_stream     The CUDA stream on which to process the nmapping of the resource
  /// @return    Error::SUCCESS If successful
  Error MapTextureResource(cudaArray** texture_array, cudaStream_t cu_stream);

  /// @brief Unmap a CUDA texture resource used for reading pixels from the main GL render buffer
  ///
  /// @param[in] cu_stream The CUDA stream on which to process the unmapping of the resource
  /// @return    Error::SUCCESS If successful
  Error UnMapTextureResource(cudaStream_t cu_stream);

  /// @brief Get the internal volume renderer's frame buffer object
  /// @param[out] fbo A location where the FBO handle can be written
  /// @return     Error::SUCCESS If successful
  Error GetRenderVolumeFrameBufferObject(uint32_t* fbo);

  /// @brief Get the internal volume renderer's color texture render target
  /// @param[out] color_texture A location where the color texture handle can be written
  /// @return     Error::SUCCESS If successful
  Error GetRenderVolumeColorTextureAttachment(uint32_t* color_texture);

  /// @brief Get the internal volume renderer's depth texture render target
  /// @param[out] depth_texture A location where the depth texture handle can be written
  /// @return     Error::SUCCESS If successful
  Error GetRenderVolumeDepthTextureAttachment(uint32_t* depth_texture);

 public:
  /// @ingroup RendererProperties
  /// @{
  properties::Property<std::string> ar_sdk_model_dir = {
      this,
      "ar_sdk_model_dir",
      "Path to AR SDK model folder. The default value will be determined based on the environment variable ARSDK "
      "which should be set before running the engine. See README.md for details on setting up environment variables.",
      engine::Engine::GetArSdkDir() + "bin/models/",
  };
  properties::Property<float> scale_factor = {
      this,
      "scale_factor",
      "Scale factor to apply to rendered triplanes, in view space",
      1.0f,
  };
  properties::Property<float> cubemap_viewdistance_pixels = {
      this,
      "cubemap_viewdistance_pixels",
      "If fix_cubemap_viewdistance is enabled, set the cubemap distance from the camera, measured in screen pixels",
      1920.0f,
  };
  properties::Property<bool> fix_cubemap_viewdistance = {
      this,
      "fix_cubemap_viewdistance",
      "Whether to render cube maps with a different, fixed focal length, than that of the camera",
      false,
  };
  properties::Property<float> cubemap_parallax_factor = {
      this,
      "cubemap_parallax_factor",
      "Enables less parallax effect on the cube map if < 1.0. This has the effect of bringing the background closer.",
      1.0f,
  };
  properties::Property<float> quality_factor = {
      this,
      "quality_factor",
      "Overall quality factor for performance / quality tradeoff.",
      0.8f,
  };
  properties::Property<bool> apply_vignette = {
      this,
      "apply_vignette",
      "Apply vignette effect to the rendered image",
      true,
  };
  properties::Property<float> vignette_intensity = {
      this,
      "vignette_intensity",
      "Strength of vignette effect, if applied - higher values make the image darker",
      0.5f,
  };
  properties::Property<bool> apply_shadow = {
      this,
      "apply_shadow",
      "Enable drop shadow behind foreground",
      true,
  };
  properties::Property<float> shadow_blur = {
      this,
      "shadow_blur",
      "Level of detail to use for shadow, higher levels add more blur to the shadow",
      5.0f,
  };
  properties::Property<float> shadow_opacity = {
      this,
      "shadow_opacity",
      "Shadow opacity, between 0.0 (transparent) and 1.0 (opaque)",
      0.5f,
  };
  properties::Property<float> shadow_scale = {
      this,
      "shadow_scale",
      "Shadow scale, relative to the size of the foreground",
      1.1f,
  };
  properties::Property<glm::vec2> shadow_offset = {
      this,
      "shadow_offset",
      "Shadow offset, as a fraction of the image width",
      {0.05f, 0.0f},
  };
  properties::Property<float> sharpen_foreground = {
      this,
      "sharpen_foreground",
      "Sharpen foreground RGB during compositing",
      0.0f,
  };
  /// @}

 private:
  /// @brief Compile all shader programs used by the renderer
  /// @return Error::SUCCESS If successful
  Error CompileShaders();

  /// @brief Create a frame buffer object with render texture attachment
  /// @param[out] fbo        The ID of the generated frame buffer object
  /// @param[out] texture_id The ID of the generated render texture object attached to the frame buffer
  /// @return     Error::SUCCESS If successful
  Error CreateFbo(GLuint* fbo, GLuint* texture_id);

  /// @brief Destory a frame buffer object with render texture attachment
  /// @param[out] fbo        The ID of the frame buffer object to destory
  /// @param[out] texture_id The ID of the render texture object attached to the frame buffer to destroy
  /// @return     Error::SUCCESS If successful
  Error DestroyFbo(GLuint* fbo, GLuint* texture_id);

  /// @brief Clear a frame buffer
  /// @param[in,out] fbo         The frame buffer to clear
  /// @param[in]     clear_color The color used in clearing the frame buffer
  /// @return        Error::SUCCESS If successful
  Error ClearFbo(GLuint fbo, const glm::vec4& clear_color);

  /// @brief Generate custom mipmap for the FBO texture, for compositing and drop shadow
  ///
  /// The mipmap generation uses a custom 4x4 down sampling kernel [1/8, 3/8, 3/8, 1/8] which reduced blocky artifacts
  /// seen in default mimpamps
  /// @return Error::SUCCESS If successful
  Error GenerateForegroundMipMap();

  const std::string m_name;
  unsigned int m_foregroundFbo;                       ///< Owned by the render feature
  unsigned int m_foregroundColorTextureId;            ///< Owned by the render feature
  unsigned int m_foregroundDepthTextureId;            ///< Owned by the render feature
  unsigned int m_numViews;                            ///< 1 for mono, larger for stereo rendering
  unsigned int m_framebufferWidth;                    ///< Full size of frame buffer, including all views if stereo
  unsigned int m_framebufferHeight;                   ///< Full size of frame buffer, including all views if stereo
  unsigned int m_renderWidth;                         ///< Set during initialization
  unsigned int m_renderHeight;                        ///< Set during initialization
  unsigned int m_quiltCols;                           ///< 1 for mono, (possibly) larger for stereo rendering
  unsigned int m_quiltRows;                           ///< 1 for mono, (possibly) larger for stereo rendering
  NvAR_FeatureHandle m_renderHandle;                  ///< Render feature handle
  cudaGraphicsResource* m_renderedImageTexResource;   ///< Texture resource enabling writing output to GL texture
  std::vector<glm::mat4> m_cameraProjectionMatrices;  ///< One for each view
  std::vector<glm::mat4> m_cameraExtrinsicsMatrices;  ///< One for each view. Used for render structures
  std::vector<glm::mat4> m_cubeMapCameraProjectionMatrices;  ///< One for each view. Used for render structures
  glm::mat4 m_cameraRoot;                                    ///< One camera root shared between views
  std::vector<glm::vec4> m_viewports;                        ///< One for each view. Used for render structures

  // GL FBOs and textures for compositing
  unsigned int m_downsampleFbo;
  unsigned int m_compositeFbo;
  unsigned int m_compositeColorTextureId;
  unsigned int m_postProcFbo;
  unsigned int m_postProcColorTextureId;

  // shader list
  std::vector<std::shared_ptr<Shader>> m_shaders;

  // render programs
  std::shared_ptr<Shader> m_cubeMapShadingProgram;
  std::shared_ptr<Shader> m_unlitMeshShadingProgram;
  std::shared_ptr<Shader> m_blitShadingProgram;
  std::shared_ptr<Shader> m_compositeShadingProgram;
  std::shared_ptr<Shader> m_vignetteShadingProgram;
  std::shared_ptr<Shader> m_downsampleShadingProgram;

  unsigned int m_emptyVao;  ///< Enables rendering of simple fixed objects without a VBO
};

}  // namespace rendering
}  // namespace core
}  // namespace nv3dvc

#endif  // SRC_CORE_RENDERING_RENDERER_H_
