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

#ifndef SRC_API_REFRAMING_REFRAMING_H_
#define SRC_API_REFRAMING_REFRAMING_H_

#include "ReframingDefs.h"
#include "core/Error.h"

// Forward declaration for CUDA API
// CUstream and cudaStream_t are CUstream_st*
struct CUstream_st;
typedef struct CUstream_st* CUstream;

class NvCVImage;

namespace nv3dvc {
namespace api {
namespace reframing {

/// @brief Wrapper for cudaStreamCreate(), if it is desired to avoid linking with the cuda lib.
/// @param[out] stream  A place to store the newly allocated stream.
core::Error CudaStreamCreate(CUstream* stream);

/// @brief Wrapper for cudaStreamDestroy(), if it is desired to avoid linking with the cuda lib.
/// @param[in]  stream  The stream to destroy.
core::Error CudaStreamDestroy(CUstream stream);

/// @brief Wrapper for cudaStreamSynchronize(), if it is desired to avoid linking with the cuda lib.
/// @param[in]  stream  The stream to destroy.
core::Error CudaStreamSynchronize(CUstream stream);

/// @brief Blueprint API for reframing
class Reframing {
 public:
  Reframing();
  ~Reframing();

  static InitParams GetDefaultInitParams();

  /// @brief Get the version of the reframing API
  ///
  /// @return The version of the reframing API
  uint32_t GetVersion() const;

  /// @brief Initialize
  ///
  /// Loads required modules, and creates a scene configuration.
  /// @return core::Error::SUCCESS If successful
  core::Error Initialize(const InitParams* init_params = nullptr);

  /// @brief Unloads all modules and destroys the scene
  ///
  /// @return core::Error::SUCCESS If successful
  core::Error Uninitialize();

  /// Configure the logger
  ///
  /// There are several ways to use this API:
  /// (1) log to stderr:      (verbosity, "stderr",  NULL, NULL).
  /// (2) log to a new file:  (verbosity, file_name, NULL, NULL) or (verbosity, file_name, NULL, "w").
  /// (3) append to a file:   (verbosity, file_name, NULL, "a").
  /// (4) log to a  function: (verbosity, NULL,      func, func_data)
  /// (5) stop logging        (verbosity, NULL,      NULL, NULL)
  ///
  /// @param[in]  verbosity the highest level of verbosity desired in the log.
  /// @param[in]  file      the file name, for logging to a file; NULL otherwise.
  ///                       The special file "stderr" will cause logging to go to stderr.
  ///                       The special file "same" will allow you to change the verbosity level without changing the
  ///                       logger, in which case the current verbosity is returned to the int at cb_data, if not NULL.
  /// @param[in]  cb        the callback function, if file == NULL, of type
  ///                       typedef void (*NvCVCallbackProc)(void *user_data, const char *msg);
  /// @param[in]  cb_data   pointer to data used by the callback function. This can also used to alter the mode used
  ///                       when opening a file (e.g. "a"), which is "w" by default.
  core::Error ConfigureLogger(int verbosity, const char* file, void (*cb)(void*, const char*), void* cb_data);

  /// @brief Add a subject to the scene
  ///
  /// @param[in] subject_id   The subject identifier with which to associate the subject's component within the scene
  /// @param[in] scene_preset The type of scene which to create.
  ///                         If ScenePreset::STATICREFRAMING: Participants's head poses are not tracked
  ///                         but rather fixed relative to their display reference frame.
  ///                         If ScenePreset::VIDEOCONFERENCING: Participant's heads have their own
  ///                         transform component relative the the reference frame of their displays, enabling them to
  ///                         move freely.
  core::Error AddSubject(int32_t subject_id, ScenePreset scene_preset = ScenePreset::DEFAULT);

  /// @brief Remove a subject from the scene
  ///
  /// @param[in] subject_id The subject identifier with which to associate the subject to remove
  /// @return    core::Error::SUCCESS If successful
  core::Error RemoveSubject(int32_t subject_id);

  /// @brief Set the image that will be used as input for a subject of the given identifier
  ///
  /// @param[in] subject_id The subject identifier with which to associate the input image
  /// @param[in] image      The input image. Pre-allocated. Requirements:
  ///                       - pixelFormat: NvCVImage_PixelFormat::NVCV_BGR
  ///                       - componentType: NvCVImage_ComponentType::NVCV_U8
  ///                       - gpuMem: NVCV_CUDA
  /// @param[in] timestamp  The timestamp associated with the latest update of this subject
  /// @return    core::Error::SUCCESS If successful
  core::Error SetInputImage(int32_t subject_id, NvCVImage* image, int64_t timestamp);

  /// @brief Set the 3D viewpoints from which the subject is viewing the scene
  ///
  /// If the viewpoint should be inferred by the input image, this function does not have to be called. This function
  /// enables explicit setting of the viewpoint if either the input image is not set, or to override the viewpoint
  /// inferred from the input image. The 3D view point's x, y, z coordinates are defined in the reference frame of the
  /// subject's physical display. Unit is meters. With a viewpoint {x, y, z} = {0.0, 0.0, 0.6}, the view is centered,
  /// 0.6 meters from the physical display.
  /// @param[in] subject_id The subject identifier with which to associate the view point
  /// @param[in] x          The view point's x coordinate in the reference frame of the display [meters]
  /// @param[in] y          The view point's y coordinate in the reference frame of the display [meters]
  /// @param[in] z          The view point's z coordinate in the reference frame of the display [meters]
  /// @param[in] timestamp  The timestamp associated with the latest update of this subject
  /// @return    core::Error::SUCCESS If successful
  core::Error SetViewPoint(int32_t subject_id, float x, float y, float z, int64_t timestamp);

  /// @brief Set the web camera's translation transform for the provided subject
  ///
  /// The input should match the transform of the physical camera used for input images provided.
  /// By default the camera's reference frame aligns with that of the display. A typical physical display-camera setup
  /// has the camera positioned centered on top of the display, tilted down slightly. The camera defines the transform
  /// from the camera's reference frame to the display's reference frame.
  /// @param[in] subject_id The subject identifier with which to associate the camera transform
  /// @param[in] x          The camera translation's x component in the reference frame of the display [meters]
  /// @param[in] y          The camera translation's y component in the reference frame of the display [meters]
  /// @param[in] z          The camera translation's z component in the reference frame of the display [meters]
  /// @param[in] timestamp  The timestamp associated with the latest update of this subject
  /// @return    core::Error::SUCCESS If successful
  core::Error SetCameraTransformTranslation(int32_t subject_id, float x, float y, float z, int64_t timestamp);

  /// @brief Set the web camera's rotation transform quaternion for the provided subject
  ///
  /// The input should match the transform of the physical camera used for input images provided.
  /// By default the camera's reference frame aligns with that of the display. A typical physical display-camera setup
  /// has the camera positioned centered on top of the display, tilted down slightly. The camera defines the transform
  /// from the camera's reference frame to the display's reference frame.
  /// @param[in] subject_id The subject identifier with which to associate the camera transform
  /// @param[in] x          The camera rotation quaternion's (imaginary) x component in the display reference frame
  /// @param[in] y          The camera rotation quaternion's (imaginary) y component in the display reference frame
  /// @param[in] z          The camera rotation quaternion's (imaginary) z component in the display reference frame
  /// @param[in] w          The camera rotation quaternion's (real) w component in the display reference frame
  /// @param[in] timestamp  The timestamp associated with the latest update of this subject
  /// @return    core::Error::SUCCESS If successful
  core::Error SetCameraTransformRotationQuat(int32_t subject_id, float x, float y, float z, float w, int64_t timestamp);

  /// @brief Set the web camera's intrinsics parameters for the provided subject
  ///
  /// Intrinsics parameters for the camera are used to convert the 2D view coordinates of the tracked head into the
  /// reference frame of the camera.
  /// @param[in] subject_id The subject identifier with which to associate the camera intrinsics parameters
  /// @param[in] fx         Horizontal focal length. Should be same as fy if pixels are square [pixels]
  /// @param[in] fy         Vertical focal length [pixels]
  /// @param[in] cx         Horizontal coordinate of principal point32_t [pixels]. Use (image width - 1.0) * 0.5 to
  ///                       center the principal point
  /// @param[in] cy         Vertical coordinate of principal point32_t [pixels]. Use (image height - 1.0) * 0.5 to
  ///                       center the principal point
  /// @param[in] timestamp  The timestamp associated with the latest update of this subject
  /// @return    core::Error::SUCCESS If successful
  core::Error SetCameraIntrinsicParams(int32_t subject_id, float fx, float fy, float cx, float cy, int64_t timestamp);

  /// @brief Set the web camera's vertical field of view for the provided subject
  ///
  /// Assumes a simple projection model for the web camera, where camera intrinsics params may not be known. The field
  /// of view will be used to infer intrinsics params used to convert the 2D view coordinates of the tracked head into
  /// the reference frame of the camera.
  /// @param[in] subject_id The subject identifier with which to associate the camera field of view
  /// @param[in] v_fov      The vertical field of view for the camera [degrees]
  /// @return    core::Error::SUCCESS If successful
  core::Error SetCameraVfov(int32_t subject_id, float v_fov, int64_t timestamp);

  /// @brief Set the translation model transform for the provided subject's root
  ///
  /// The root model transform represents the subjects transform in world space. If this is a static subject, the
  /// rendered object will be transformed according to the provided transform. If it is an animated object, e.g.
  /// specified with ScenePreset::VIDEOCONFERENCING, the root transform does not coincide with the rendered object. The
  /// rendered object's local transform is relative to the root transform in world space.
  /// @param[in] subject_id The subject identifier with which to associate the model transform
  /// @param[in] x          The translation's x component in the world reference frame [meters]
  /// @param[in] y          The translation's y component in the world reference frame [meters]
  /// @param[in] z          The translation's z component in the world reference frame [meters]
  /// @param[in] timestamp  The timestamp associated with the latest update of this subject
  /// @return    core::Error::SUCCESS If successful
  core::Error SetRootModelTransformTranslation(int32_t subject_id, float x, float y, float z, int64_t timestamp);

  /// @brief Set the quaternion rotation model transform for the provided subject's root
  ///
  /// The root model transform represents the subjects transform in world space. If this is a static subject, the
  /// rendered object will be transformed according to the provided transform. If it is an animated object, e.g.
  /// specified with ScenePreset::VIDEOCONFERENCING, the root transform does not coincide with the rendered object. The
  /// rendered object's local transform is relative to the root transform in world space.
  /// @param[in] subject_id The subject identifier with which to associate the model transform
  /// @param[in] x          The camera rotation quaternion's (imaginary) x component in the world reference frame
  /// @param[in] y          The camera rotation quaternion's (imaginary) y component in the world reference frame
  /// @param[in] z          The camera rotation quaternion's (imaginary) z component in the world reference frame
  /// @param[in] w          The camera rotation quaternion's (real) w component in the world reference frame
  /// @param[in] timestamp  The timestamp associated with the latest update of this subject
  /// @return    core::Error::SUCCESS If successful
  core::Error SetRootModelTransformRotationQuat(int32_t subject_id, float x, float y, float z, float w,
                                                int64_t timestamp);

  /// @brief Set the image for which the output view will be rendered for a subject of the given identifier
  ///
  /// The function prepares a rendering context for the output which includes a GL frame buffer object, a color texture
  /// attachment and a depth texture attachment. These can be accessed using GetFrameBufferObject,
  /// GetColorTextureAttachment, and GetDepthTextureAttachment
  /// @param[in] subject_id The subject identifier with which to associate the output image
  /// @param[in] image      The output image. Pre-allocated. Requirements:
  ///                       - pixelFormat: NvCVImage_PixelFormat::NVCV_RGBA
  ///                       - componentType: NvCVImage_ComponentType::NVCV_U8
  ///                       - gpuMem: NVCV_CUDA
  /// @return    core::Error::SUCCESS If successful
  core::Error SetOutputImage(int32_t subject_id, NvCVImage* image);

  /// @brief Set the cuda stream on which to run the main pipeline
  /// @param[in] cu_stream The CUDA stream
  /// @return    core::Error::SUCCESS If successful
  core::Error SetCudaStream(CUstream cu_stream);

  /// @brief Set a property for the given subject
  ///
  /// Note: Atomic properties are not supported.
  /// @tparam    T               The property type. Supported types are:
  ///                            bool, int16_t, uint16_t, int32_t, int64_t, uint32_t, uint64_t, signed char, unsigned
  ///                            char, float, double, wchar_t, std::string, glm::u8vec1, glm::u8vec2, glm::u8vec3,
  ///                            glm::u8vec4, glm::bvec1, glm::bvec2, glm::bvec3, glm::bvec4, glm::ivec1, glm::ivec2,
  ///                            glm::ivec3, glm::ivec4, glm::uvec1, glm::uvec2, glm::uvec3, glm::uvec4, glm::vec1,
  ///                            glm::vec2, glm::vec3, glm::vec4, glm::dvec1, glm::dvec2, glm::dvec3, glm::dvec4,
  ///                            glm::mat2x2, glm::mat2x3, glm::mat2x4, glm::mat3x2, glm::mat3x3, glm::mat3x4,
  ///                            glm::mat4x2, glm::mat4x3, glm::mat4x4, glm::dmat2x2, glm::dmat2x3, glm::dmat2x4,
  ///                            glm::dmat3x2, glm::dmat3x3, glm::dmat3x4, glm::dmat4x2, glm::dmat4x3, glm::dmat4x4,
  ///                            glm::fquat, glm::dquat
  /// @param[in] subject_id      The subject identifier with which to associate the property
  /// @param[in] entity_selector The entity with which to associate the property
  /// @param[in] component_name  The name of the component with which to associate the property
  /// @param[in] property_name   The name of the property
  /// @param[in] val             The value of the property to set
  /// @return    core::Error::ERR_ITEM_NOT_FOUND If the subject does not exist, or
  ///                                            If the entity does not exist, or
  ///                                            If the component does not exist, or
  ///                                            If the property does not exist
  ///            core::Error::ERR_SCENE          If the component is not a property owner
  /// @return    core::Error::SUCCESS            If successful
  template <typename T>
  core::Error SetProperty(int32_t subject_id, EntitySelector entity_selector, const char* component_name,
                          const char* property_name, const T& val);

  /// @brief Get a property for the given subject
  ///
  /// Note: Atomic properties are not supported.
  /// @tparam    T                The property type. Supported types are:
  ///                             bool, int16_t, uint16_t, int32_t, int64_t, uint32_t, uint64_t, signed char, unsigned
  ///                             char, float, double, wchar_t, std::string, glm::u8vec1, glm::u8vec2, glm::u8vec3,
  ///                             glm::u8vec4, glm::bvec1, glm::bvec2, glm::bvec3, glm::bvec4, glm::ivec1, glm::ivec2,
  ///                             glm::ivec3, glm::ivec4, glm::uvec1, glm::uvec2, glm::uvec3, glm::uvec4, glm::vec1,
  ///                             glm::vec2, glm::vec3, glm::vec4, glm::dvec1, glm::dvec2, glm::dvec3, glm::dvec4,
  ///                             glm::mat2x2, glm::mat2x3, glm::mat2x4, glm::mat3x2, glm::mat3x3, glm::mat3x4,
  ///                             glm::mat4x2, glm::mat4x3, glm::mat4x4, glm::dmat2x2, glm::dmat2x3, glm::dmat2x4,
  ///                             glm::dmat3x2, glm::dmat3x3, glm::dmat3x4, glm::dmat4x2, glm::dmat4x3, glm::dmat4x4,
  ///                             glm::fquat, glm::dquat
  /// @param[in]  subject_id      The subject identifier with which to associate the property
  /// @param[in]  entity_selector The entity with which to associate the property
  /// @param[in]  component_name  The name of the component with which to associate the property
  /// @param[in]  property_name   The name of the property
  /// @param[out] val             A location where the output can be written
  /// @return     core::Error::ERR_NULL_POINTER   If val is nullptr
  ///             core::Error::ERR_ITEM_NOT_FOUND If the subject does not exist, or
  ///                                             If the entity does not exist, or
  ///                                             If the component does not exist, or
  ///                                             If the property does not exist
  ///             core::Error::ERR_SCENE          If the component is not a property owner
  /// @return     core::Error::SUCCESS            If successful
  template <typename T>
  core::Error GetProperty(int32_t subject_id, EntitySelector entity_selector, const char* component_name,
                          const char* property_name, T* val);

  /// @brief Get the 3D viewpoints from which the subject is viewing the scene
  ///
  /// The 3D view point's x, y, z coordinates are defined in the reference frame of the subject's physical display. Unit
  /// is meters. With a viewpoint {x, y, z} = {0.0, 0.0, 0.6}, the view is centered, 0.6 meters from the physical
  /// display.
  /// @param[in] subject_id The subject identifier with which to associate the view point
  /// @param[out] x          A location of where to write the point's x coordinate in the reference frame of the display
  ///                       [meters]. Writing is ignored if nullptr is passed
  /// @param[out] y          A location of where to write the point's y coordinate in the reference frame of the display
  ///                       [meters]. Writing is ignored if nullptr is passed
  /// @param[out] z          A location of where to write the point's z coordinate in the reference frame of the display
  ///                       [meters]. Writing is ignored if nullptr is passed
  /// @return     core::Error::ERR_ITEM_NOT_FOUND If the subject doesn'e exist
  ///             core::Error::SUCCESS If successful
  core::Error GetViewPoint(int32_t subject_id, float* x, float* y, float* z);

  /// @brief Get the filtered 3D transforms's translation of the subject's triplane entity
  /// @param subject_id The subject identifier with which to associate the triplane transform
  /// @param[out] x     A location of where to write the point's x coordinate in the reference frame of the physical
  ///                   camera [meters]. Writing is ignored if nullptr is passed
  /// @param[out] y     A location of where to write the point's y coordinate in the reference frame of the physical
  ///                   camera [meters]. Writing is ignored if nullptr is passed
  /// @param[out] z     A location of where to write the point's z coordinate in the reference frame of the physical
  ///                   camera [meters]. Writing is ignored if nullptr is passed
  /// @return     core::Error::ERR_ITEM_NOT_FOUND If the subject doesn'e exist or if it does not have the transform
  ///             component
  ///             core::Error::SUCCESS If successful
  core::Error GetTriplaneTransformTranslation(int32_t subject_id, float* x, float* y, float* z);

  /// @brief Get the OpenGL frame buffer object ID
  ///
  /// SetOutputImage is expected to have been called for this subject before this function is called.
  /// @param[in]  subject_id The subject identifier with which to associate the frame buffer object
  /// @param[out] val        A location where the output can be written
  /// @return     core::Error::SUCCESS If successful
  core::Error GetFrameBufferObject(int32_t subject_id, uint32_t* val) const;

  /// @brief Get the OpenGL color texture attachment ID
  ///
  /// SetOutputImage is expected to have been called for this subject before this function is called.
  /// @param[in]  subject_id The subject identifier with which to associate the color texture attachment
  /// @param[out] val        A location where the output can be written
  /// @return     core::Error::SUCCESS If successful
  core::Error GetColorTextureAttachment(int32_t subject_id, uint32_t* val) const;

  /// @brief Get the OpenGL depth texture attachment ID
  ///
  /// SetOutputImage is expected to have been called for this subject before this function is called.
  /// @param[in]  subject_id The subject identifier with which to associate the depth texture attachment
  /// @param[out] val        A location where the output can be written
  /// @return     core::Error::SUCCESS If successful
  core::Error GetDepthTextureAttachment(int32_t subject_id, uint32_t* val) const;

  /// @brief Run the reframing blueprint pipeline one iteration
  ///
  /// @param[in]  subject_id    The subject identifier for which to render the output
  /// @param[in]  dt            Simulation step [seconds] (used for temporal components such as filters)
  /// @param[out] timestamp_out The output timestamp with which the rendered output image will be associated
  /// @return     core::Error::SUCCESS If successful
  core::Error Run(int32_t subject_id, float dt, int64_t* timestamp_out = nullptr);

 private:
  struct Impl;
  Impl* m_impl;
};

}  // namespace reframing
}  // namespace api
}  // namespace nv3dvc

#endif  // SRC_API_REFRAMING_REFRAMING_H_
