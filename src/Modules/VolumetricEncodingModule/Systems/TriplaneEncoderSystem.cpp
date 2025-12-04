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

#include "TriplaneEncoderSystem.h"

#include <string>

#include "Core/Engine/Engine.h"
#include "Core/EntityComponentSystem/Entity.h"
#include "Core/Error.h"
#include "Core/Util/Logger.h"
#include "Modules/CommonModule/Components/TransformComponent.h"
#include "Modules/CommonModule/Components/VideoFrameComponent.h"
#include "Modules/TrackingModule/Components/TrackedHeadComponent.h"
#include "Modules/TriplaneModule/Components/TriplaneBufferComponent.h"
#include "Modules/TriplaneModule/Components/TriplaneVolumeComponent.h"
#include "Modules/VolumetricEncodingModule/Components/EncodedTriplaneComponent.h"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/transform.hpp"
#include "nvARVolumetricEncoding.h"
#include "nvCVVolumeDefs.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Macros                                                                                                           ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace nv3dvc {
namespace modules {
namespace volumetricencodingmodule {
namespace systems {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Static function declarations                                                                                     ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Transform from raw pose matrix as output from the triplane encoder feature to local GL transform matrix
/// @param[in]  raw_pose_matrix The raw pose matrix
/// @return     A transform matrix corresponding to local pose in GL model space
static glm::mat4 ComputeLocalPoseFromPoseModel(const glm::mat4& raw_pose_matrix);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Static function definitions                                                                                      ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static glm::mat4 ComputeLocalPoseFromPoseModel(const glm::mat4& raw_pose_matrix) {
  glm::mat4 local_pose = glm::inverse(glm::transpose(glm::mat4(raw_pose_matrix)));
  glm::mat4 rot_y = glm::rotate(glm::pi<float>(), glm::vec3(0.0f, 1.0f, 0.0f));
  glm::mat4 rot_z = glm::rotate(glm::pi<float>(), glm::vec3(0.0f, 0.0f, 1.0f));
  // Take only rotational part
  glm::mat4 gl_rot_matrix = rot_z * rot_y * glm::mat4(glm::mat3(local_pose));
  // Translation of rotation center is to match local head space with tracked head
  glm::vec3 center_of_rotation = {0.0f, 0.02f, -0.02f};
  return gl_rot_matrix * glm::translate(glm::mat4(1.0f), center_of_rotation);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Member function definitions                                                                                      ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TriplaneEncoderSystem::TriplaneEncoderSystem()
    : m_encoderHandle(nullptr), m_outputFocalScale(0.0f), m_modelOutputPose({1.0f}) {}

core::Error TriplaneEncoderSystem::Initialize() {
  core::Error err = core::Error::SUCCESS;

  // For debugging
  CHECK_NVCV_SUCCESS(NvAR_ConfigureLogger(1, "stderr", nullptr, nullptr));

  // Configure
  CHECK_NVCV_SUCCESS(NvAR_Create(NvAR_Feature_VolumetricEncoding, &m_encoderHandle));
  CHECK_NVCV_SUCCESS(NvAR_SetString(m_encoderHandle, NvAR_Parameter_Config(ModelDir), ar_sdk_model_dir.get()->c_str()));
  CHECK_NVCV_SUCCESS(NvAR_SetU32(m_encoderHandle, NvAR_Parameter_Config(OutputType), NVCV_VOLUME_TYPE_TRIPLANE));

  // Load
  CHECK_NVCV_SUCCESS(NvAR_Load(m_encoderHandle));

bail:
  if (static_cast<NvCV_Status>(err) == NVCV_ERR_FILE) {
    LOG_ERROR("Model file read failed using model directory \"%s\"", ar_sdk_model_dir.get()->c_str());
    LOG_ERROR("Make sure to set model directory according to description \"%s\"", ar_sdk_model_dir.description());
  }
  return err;
}

core::Error TriplaneEncoderSystem::Uninitialize() {
  core::Error err = core::Error::SUCCESS;
  CHECK_NVCV_SUCCESS(NvAR_Destroy(m_encoderHandle));
bail:
  return err;
}

core::Error TriplaneEncoderSystem::OnLoadScene(core::ecs::registry::EntityRegistry* reg) {
  core::Error err = core::Error::SUCCESS;
  auto triplane_buffer_entities = reg->view<triplanemodule::components::TriplaneBufferComponent>();

  for (auto& entity : triplane_buffer_entities) {
    entity.GetComponent<triplanemodule::components::TriplaneBufferComponent>().Initialize();
  }

  CHECK_NVCV_SUCCESS(NvAR_SetCudaStream(m_encoderHandle, NvAR_Parameter_Config(CUDAStream), GetStream()));
bail:
  return err;
}

core::Error TriplaneEncoderSystem::Run(core::ecs::registry::EntityRegistry* reg, float dt) {
  core::Error err = core::Error::SUCCESS;
  auto triplane_buffer_entities = reg->view<triplanemodule::components::TriplaneBufferComponent>();

  if (triplane_buffer_entities.size() > 1) {
    printf("Need no more than one triplane buffer component in scene\n");
    return nv3dvc::core::ERR_NOTHINGRENDERED;
  }
  if (triplane_buffer_entities.size() == 0) {
    return core::Error::SUCCESS;
  }
  auto& triplane_buffer =
      triplane_buffer_entities[0].GetComponent<triplanemodule::components::TriplaneBufferComponent>();

  for (auto& entity :
       reg->view<modules::commonmodule::components::VideoFrameComponent, components::EncodedTriplaneComponent>()) {
    auto& source_frame = entity.GetComponent<modules::commonmodule::components::VideoFrameComponent>();
    // Try to get a tracked head component if we have one in the parent
    trackingmodule::components::TrackedHeadComponent* tracked_head = nullptr;
    if (entity.HasParent()) {
      auto parent = entity.GetParent();
      if (parent.HasComponent<trackingmodule::components::TrackedHeadComponent>()) {
        tracked_head = &parent.GetComponent<trackingmodule::components::TrackedHeadComponent>();
      }
    }
    auto& encoded_triplane = entity.GetComponent<components::EncodedTriplaneComponent>();

    triplanemodule::TriplaneFrame* triplane_frame = triplane_buffer.GetWriteableTriplaneFrame();
    if (!triplane_frame) continue;

    NvCVImage* cropped_head_image_bgr = source_frame.GetImagePtr();
    NvCVTriplaneVolume* triplane_volume = &triplane_frame->triplane_volume;

    if (!cropped_head_image_bgr->pixels) {
      LOG_DEBUG("Skipping empty image in entity \"%s\"", entity.GetComponent<std::string>().c_str());
      continue;
    }

    if (m_croppedHeadImageRgbPlanar.pixels == nullptr) {
      CHECK_NVCV_SUCCESS(NvCVImage_Realloc(&m_croppedHeadImageRgbPlanar, cropped_head_image_bgr->width,
                                           cropped_head_image_bgr->height, NvCVImage_PixelFormat::NVCV_RGB,
                                           NvCVImage_ComponentType::NVCV_F32, NVCV_PLANAR, NVCV_CUDA, 1));
    }
    CHECK_NVCV_SUCCESS(NvCVImage_Transfer(cropped_head_image_bgr, &m_croppedHeadImageRgbPlanar, 1.0f / 255.0f,
                                          GetStream(), &m_tmpImg));
    CHECK_NVCV_SUCCESS(
        NvAR_SetObject(m_encoderHandle, NvAR_Parameter_Input(Image), &m_croppedHeadImageRgbPlanar, sizeof(NvCVImage)));
    CHECK_NVCV_SUCCESS(
        NvAR_SetObject(m_encoderHandle, NvAR_Parameter_Output(Volume), triplane_volume, sizeof(NvCVTriplaneVolume)));
    CHECK_NVCV_SUCCESS(
        NvAR_SetF32Array(m_encoderHandle, NvAR_Parameter_Output(Pose), glm::value_ptr(m_modelOutputPose), 16));
    CHECK_NVCV_SUCCESS(NvAR_SetCudaStream(m_encoderHandle, NvAR_Parameter_Config(CUDAStream), GetStream()));
    CHECK_NVCV_SUCCESS(NvAR_Run(m_encoderHandle));
    CHECK_NVCV_SUCCESS(NvAR_GetF32(m_encoderHandle, NvAR_Parameter_Output(FocalScale), &m_outputFocalScale));

    if (tracked_head) {
      // Write the metadata
      triplanemodule::TriplanePackage& triplane_package = triplane_frame->triplane_package;

      glm::vec3 tracked_translation_m = tracked_head->TrackedTranslationWithOffset() * 0.01f;  // cm to m
      glm::mat4 gl_pose_matrix = ComputeLocalPoseFromPoseModel(m_modelOutputPose);

      glm::vec3 roll_correction_euler = glm::vec3(0.0f, 0.0f, tracked_head->raw_pose_euler.z);
      glm::quat roll_correction = tracked_head->apply_roll_correction.get()->load() ? glm::quat(roll_correction_euler)
                                                                                    : glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
      glm::mat4 triplane_matrix = glm::mat4_cast(roll_correction) * gl_pose_matrix;
      glm::quat default_rot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
      glm::vec3 default_pos = glm::vec3(0.0f, 0.0f, 0.0f);
      triplane_package.client_id = 0;
      triplane_package.focal_scale = m_outputFocalScale;
      triplane_package.gaze_angle = {0.0f, 0.0f};
      triplane_package.head_pose_quaternion = tracked_head->face_box_to_display_rotation;
      triplane_package.head_pose_translation = encoded_triplane.fix_translation ? default_pos : tracked_translation_m;
      triplane_package.head_scale = encoded_triplane.GetHeadScale();
      triplane_package.mins = {};  // Mins and maxs are implicitly written to triplane encoder's output object
      triplane_package.maxs = {};  // Mins and maxs are implicitly written to triplane encoder's output object
      triplane_package.raw_tracking_pose_euler = tracked_head->raw_pose_euler;
      triplane_package.timestamp = 0;
      triplane_package.triplane_quaternion = encoded_triplane.fix_rotation ? default_rot : glm::quat(triplane_matrix);
      triplane_package.triplane_translation =
          encoded_triplane.fix_translation
              ? default_pos
              : glm::vec3(triplane_matrix[3][0], triplane_matrix[3][1], triplane_matrix[3][2]);
      triplane_package.confidence = tracked_head->tracking_confidence;
    }

    triplane_frame->triplane_package.timestamp = source_frame.GetTimeStamp();
    triplane_buffer.SignalProvide();
  }
bail:
  return err;
}

}  // namespace systems
}  // namespace volumetricencodingmodule
}  // namespace modules
}  // namespace nv3dvc
