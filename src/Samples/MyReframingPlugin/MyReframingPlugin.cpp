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

#include "Api/Reframing/Reframing.h"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "nvCVImage.h"

int main(int argc, char** argv) {
  //! [Reframing API sample]
  nv3dvc::api::reframing::Reframing reframer;

  reframer.Initialize();

  NvCVImage img_output;                      // Rendered image from new viewpoint
  NvCVImage img_viewer;                      // Webcam image
  NvCVImage img_subject;                     // Webcam image
  float v_fov_viewer = 40.0f;                // May or may not be known / set
  float v_fov_subject = 40.0f;               // May or may not be known / set
  glm::vec3 viewpoint = {0.0f, 0.0f, 0.6f};  // Meters. May be used if no spectator image exists

  NvCVImage_Realloc(&img_output, 1920, 1080, NvCVImage_PixelFormat::NVCV_RGBA, NvCVImage_ComponentType::NVCV_U8,
                    NVCV_CHUNKY, NVCV_GPU, 0);
  NvCVImage_Realloc(&img_viewer, 1920, 1080, NvCVImage_PixelFormat::NVCV_BGR, NvCVImage_ComponentType::NVCV_U8,
                    NVCV_CHUNKY, NVCV_GPU, 0);
  NvCVImage_Realloc(&img_subject, 1920, 1080, NvCVImage_PixelFormat::NVCV_BGR, NvCVImage_ComponentType::NVCV_U8,
                    NVCV_CHUNKY, NVCV_GPU, 0);
  reframer.AddSubject(NV3DVC_REFRAMING_SUBJECT_SPECTATOR, nv3dvc::api::reframing::ScenePreset::VIDEOCONFERENCING);
  reframer.AddSubject(NV3DVC_REFRAMING_SUBJECT_PARTICIPANT_1, nv3dvc::api::reframing::ScenePreset::VIDEOCONFERENCING);

  const auto start_time = std::chrono::steady_clock::now();
  auto now = std::chrono::steady_clock::now();
  auto timestamp_in = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
  reframer.SetCameraVfov(NV3DVC_REFRAMING_SUBJECT_SPECTATOR, v_fov_viewer, timestamp_in);
  reframer.SetCameraVfov(NV3DVC_REFRAMING_SUBJECT_PARTICIPANT_1, v_fov_subject, timestamp_in);
  reframer.SetOutputImage(NV3DVC_REFRAMING_SUBJECT_SPECTATOR, &img_output);

  {  // In processing loop
    int64_t timestamp_out = 0;
    const float dt = 1.0f / 60.0f;
    now = std::chrono::steady_clock::now();
    timestamp_in = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();

    reframer.SetViewPoint(NV3DVC_REFRAMING_SUBJECT_SPECTATOR, viewpoint.x, viewpoint.y, viewpoint.z, timestamp_in);
    reframer.SetInputImage(NV3DVC_REFRAMING_SUBJECT_SPECTATOR, &img_viewer, timestamp_in);
    reframer.SetInputImage(NV3DVC_REFRAMING_SUBJECT_PARTICIPANT_1, &img_subject, timestamp_in);
    reframer.Run(NV3DVC_REFRAMING_SUBJECT_SPECTATOR, dt, &timestamp_out);
  }
  reframer.Uninitialize();
  //! [Reframing API sample]
}
