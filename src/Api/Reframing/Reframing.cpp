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

#include "Reframing.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Core/Engine/Engine.h"
#include "Core/Error.h"
#include "Core/Serialization/Serialization.h"
#include "Core/Util/Logger.h"
#include "Modules/CaptureModule/CaptureDevice/nvCVOpenCV.h"
#include "Modules/Commonmodule/Commonmodule.h"
#include "Modules/ControlModule/ControlModule.h"
#include "Modules/RenderModule/RenderModule.h"
#include "Modules/TrackingModule/TrackingModule.h"
#include "Modules/TriplaneModule/TriplaneModule.h"
#include "Modules/VideoEffectsModule/VideoEffectsModule.h"
#include "Modules/VolumetricEncodingModule/VolumetricEncodingModule.h"
#include "Modules/WindowModule/WindowModule.h"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "nvCVImage.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Macros                                                                                                           ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define MAX_NUM_SUPPORTED_SUBJECTS 2

#define GET_SUBJECT_OR_BAIL(impl, subject_struct, subject_id)                                             \
  do {                                                                                                    \
    std::lock_guard lock(impl->subjects_mutex);                                                           \
    CHECK_TRUE(impl->subjects.find(subject_id) != impl->subjects.end(), core::Error::ERR_ITEM_NOT_FOUND); \
    subject_struct = &impl->subjects[subject_id];                                                         \
  } while (0)

#define IMPLEMENT_SETPROPERTY(Type)                                                                            \
  template <>                                                                                                  \
  core::Error Reframing::SetProperty(const int32_t subject_id, const EntitySelector entity_selector,           \
                                     const char* component_name, const char* property_name, const Type& val) { \
    return m_impl->SetProperty(subject_id, entity_selector, component_name, property_name, val);               \
  }

#define IMPLEMENT_GETPROPERTY(Type)                                                                      \
  template <>                                                                                            \
  core::Error Reframing::GetProperty(const int32_t subject_id, const EntitySelector entity_selector,     \
                                     const char* component_name, const char* property_name, Type* val) { \
    return m_impl->GetProperty(subject_id, entity_selector, component_name, property_name, val);         \
  }

#define IMPLEMENT_SETGETPROPERTY(Type) \
  IMPLEMENT_SETPROPERTY(Type)          \
  IMPLEMENT_GETPROPERTY(Type)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Globals                                                                                                          ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

char* g_nvARSDKPath = NULL;
char* g_nvVFXSDKPath = NULL;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Types                                                                                                            ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct SubjectStruct {
  /// @brief User owned
  struct User {
    NvCVImage* input_image = nullptr;
    NvCVImage* output_image = nullptr;
  } user;
  /// @brief Internal components for this subject
  struct Internal {
    // Entities
    std::unordered_map<nv3dvc::api::reframing::EntitySelector, nv3dvc::core::ecs::Entity> entities;
    // Components
    nv3dvc::modules::commonmodule::components::VideoFrameComponent* video_frame_component = nullptr;
    nv3dvc::modules::commonmodule::components::VideoFrameComponent* cropped_video_frame_component = nullptr;
    nv3dvc::modules::commonmodule::components::CameraCalibrationComponent* camera_calibration_component = nullptr;
    nv3dvc::modules::commonmodule::components::CameraComponent* camera_component = nullptr;
    nv3dvc::modules::rendermodule::components::DisplayComponent* display_component = nullptr;
    nv3dvc::modules::rendermodule::components::StereoViewComponent* stereo_view_component = nullptr;
    nv3dvc::modules::commonmodule::components::TransformComponent* root_transform_component = nullptr;
    nv3dvc::modules::commonmodule::components::TransformComponent* camera_transform_component = nullptr;
    // Etc
    nv3dvc::modules::commonmodule::components::RecordingCallbackComponent::VideoFormat output_image_format;
    NvCVImage output_image_wrapper;  // Wrapper for mapped frame data
  } internal;
  nv3dvc::api::reframing::ScenePreset scene_preset = nv3dvc::api::reframing::ScenePreset::DEFAULT;
  int32_t id = -1;
  int64_t timestamp = std::numeric_limits<int64_t>::min();
};

/// @brief Pipeline for running systems
struct Pipeline {
  CUstream stream = nullptr;
  std::vector<std::shared_ptr<nv3dvc::core::ecs::System>> systems;
};

struct nv3dvc::api::reframing::Reframing::Impl {
  /// @brief Internal class for safe scene edit using scope-bound resource management
  class SafeSceneEdit {
   public:
    explicit SafeSceneEdit(nv3dvc::api::reframing::Reframing::Impl* reframing_impl, Pipeline* current_pipeline);
    ~SafeSceneEdit();

   private:
    nv3dvc::api::reframing::Reframing::Impl* m_impl;
    std::vector<bool> m_threadSystemPauseStates;
  };

  uint32_t version = 0;    /// < API implementation version
  InitParams init_params;  /// < Parameters with which the API was initialized
  nv3dvc::core::engine::Engine engine = {};
  std::unordered_map<int32_t, SubjectStruct> subjects;
  std::mutex subjects_mutex;                    /// < For accessing subjects
  std::unique_ptr<Pipeline> main_pipeline;      /// < All systems to run on main thread
  std::unique_ptr<Pipeline> tracking_pipeline;  /// < Empty if non-threaded
  std::vector<std::shared_ptr<nv3dvc::core::ecs::System>> all_systems;
  int32_t id_to_render = -1;  /// < Id to subject who's view gets rendered at Run

  // Modules
  nv3dvc::modules::windowmodule::WindowModule* window_module = nullptr;
  nv3dvc::modules::rendermodule::RenderModule* render_module = nullptr;
  nv3dvc::modules::commonmodule::CommonModule* common_module = nullptr;
  nv3dvc::modules::behaviormodule::BehaviorModule* behavior_module = nullptr;
  nv3dvc::modules::controlmodule::ControlModule* control_module = nullptr;
  nv3dvc::modules::trackingmodule::TrackingModule* tracking_module = nullptr;
  nv3dvc::modules::videoeffectsmodule::VideoEffectsModule* video_effects_module = nullptr;
  nv3dvc::modules::volumetricencodingmodule::VolumetricEncodingModule* volumetric_encoding_module = nullptr;
  nv3dvc::modules::triplanemodule::TriplaneModule* triplane_module = nullptr;

  // Systems
  std::shared_ptr<nv3dvc::modules::trackingmodule::systems::HeadTrackingSystem> head_tracking_system;
  std::shared_ptr<nv3dvc::modules::videoeffectsmodule::systems::AigsSystem> aigs_system;
  std::shared_ptr<nv3dvc::modules::volumetricencodingmodule::systems::TriplaneEncoderSystem> triplane_encoder_system;
  std::shared_ptr<nv3dvc::modules::behaviormodule::systems::BehaviorSystem> behavior_system;
  std::shared_ptr<nv3dvc::modules::rendermodule::systems::RenderSystem> render_system;

  /// @brief Register all modules
  void RegisterModules();

  /// @brief Unregister all modules
  void UnregisterModules();

  /// @brief Populate system pointers. Call after modules are registered
  void PopulateSystemPointers();

  /// @brief Setup main pipeline. Populates main_systems
  void SetupMainPipeline();

  /// @brief Setup tracking pipeline. Populates tracking_systems
  /// @param threaded Whether to run tracking pipeline on a separate thread. If false populate main pipeline
  void SetupTrackingPipeline(bool threaded);

  /// @brief Build a subject in the scene. Adds all entities and components to the subject_struct
  /// @param[in,out] subject_struct The subject for which to add entities and components
  /// @param[in]     scene_preset   The scene preset to use for this subject
  void BuildSubjectInScene(SubjectStruct* subject_struct, ScenePreset scene_preset);

  /// @brief Add a root entity to the scene. This is the parent of all other entities
  /// @param[in] subject_struct The subject for which to add the root entity
  /// @return    The root entity
  core::ecs::Entity AddRoot(SubjectStruct* subject_struct);

  /// @brief Add a display entity to the scene
  ///
  /// Adds components:
  /// * TransformComponent
  /// * DisplayComponent
  /// * CameraComponent
  /// * StereoViewComponent
  /// * RecordingCallbackComponent
  /// @param[in,out] subject_struct The subject for which to add the new entity
  /// @param[in]     parent         The entity that will be set to parent for the new entity
  /// @return        The new entity
  core::ecs::Entity AddDisplay(SubjectStruct* subject_struct, const core::ecs::Entity& parent);

  /// @brief Add a webcam entity to the scene
  ///
  /// Adds components:
  /// * PoseCalibrationBehavior
  /// * TransformComponent
  /// * VideoFrameComponent
  /// * CameraCalibrationComponent
  /// * TrackedHeadComponent
  /// @param[in,out] subject_struct The subject for which to add the new entity
  /// @param[in]     parent         The entity that will be set to parent for the new entity
  /// @return        The new entity
  core::ecs::Entity AddWebcam(SubjectStruct* subject_struct, const core::ecs::Entity& parent);

  /// @brief Add a webcam entity to the scene
  ///
  /// Adds components:
  /// * VideoFrameComponent
  /// * VideoEffectsComponent
  /// * EncodedTriplaneComponent
  /// @param[in,out] subject_struct The subject for which to add the new entity
  /// @param[in]     parent         The entity that will be set to parent for the new entity
  /// @return        The new entity
  core::ecs::Entity AddVideoOut(SubjectStruct* subject_struct, const core::ecs::Entity& parent);

  /// @brief Add a webcam entity to the scene
  ///
  /// Adds components:
  /// * RenderableTriplaneComponent
  /// * TriplaneBufferComponent
  /// * TransformComponent
  /// @param[in,out] subject_struct The subject for which to add the new entity
  /// @param[in]     parent         The entity that will be set to parent for the new entity
  /// @return        The new entity
  core::ecs::Entity AddTriplane(SubjectStruct* subject_struct, const core::ecs::Entity& parent);

  /// @brief Internal implementation of SetProperty. See Reframing::SetProperty
  template <typename T>
  core::Error SetProperty(int32_t subject_id, EntitySelector entity_selector, const const char* component_name,
                          const const char* property_name, const T& val);

  /// @brief Internal implementation of GetProperty. See Reframing::GetProperty
  template <typename T>
  core::Error GetProperty(int32_t subject_id, EntitySelector entity_selector, const const char* component_name,
                          const const char* property_name, T* val);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Static function declarations                                                                                     ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static nv3dvc::core::Error UpdateCamera(SubjectStruct* subject_struct);

/// @brief Stop systems that may be running on a different thread
/// @param[in,out] all_systems            A vector of all systems run by the engine
/// @param[in,out] current_thread_systems The systems that are configured to run on the current thread, which we know
///                                       are not currently running
/// @param[out]    pause_states           The output pause states of all the systems, which can be used to reset states
///                                       when systems are resumed
/// @param[in]     max_try_time_ms        The maximum time to wait for systems to stop before timing out
/// @return        nv3dvc::core::Error::SUCCESS     If systems were stopped successfully
///                nv3dvc::core::Error::ERR_TIMEOUT If systems did not stop within max_try_time_ms
static nv3dvc::core::Error StopSystemsAndRecordPauseState(
    std::vector<std::shared_ptr<nv3dvc::core::ecs::System>>* all_systems,
    std::vector<std::shared_ptr<nv3dvc::core::ecs::System>>* current_thread_systems, std::vector<bool>* pause_states,
    int max_try_time_ms);

/// @brief Resume systems that were stopped by StopSystemsAndRecordPauseState
/// @param[in,out] all_systems            A vector of all systems run by the engine
/// @param[in]     pause_states           The pause states of all the systems, used to reset states
static void ResumePauseStates(std::vector<std::shared_ptr<nv3dvc::core::ecs::System>>* all_systems,
                              const std::vector<bool>& pause_states);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Static function definitions                                                                                      ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static nv3dvc::core::Error UpdateCamera(SubjectStruct* subject_struct) {
  nv3dvc::core::Error err = nv3dvc::core::Error::SUCCESS;
  subject_struct->internal.camera_component->SetViewMatrix(
      glm::inverse(nv3dvc::modules::commonmodule::GetGlobalTransform(
          subject_struct->internal.entities[nv3dvc::api::reframing::EntitySelector::DISPLAY])));
bail:
  return err;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Class member functions definitions                                                                               ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static nv3dvc::core::Error StopSystemsAndRecordPauseState(
    std::vector<std::shared_ptr<nv3dvc::core::ecs::System>>* all_systems,
    std::vector<std::shared_ptr<nv3dvc::core::ecs::System>>* current_thread_systems, std::vector<bool>* pause_states,
    const int max_try_time_ms = 1000) {
  nv3dvc::core::Error err = nv3dvc::core::Error::SUCCESS;
  static constexpr int kSleepTimeMs = 50;

  int try_time_ms = 0;
  // Record all pause states and pause all systems
  pause_states->reserve(all_systems->size());
  for (auto& system : *all_systems) {
    pause_states->push_back(system->IsPaused());
    system->SetPaused(true);
  }
  for (auto& system : *all_systems) {
    bool is_current_thread_system = std::find(current_thread_systems->begin(), current_thread_systems->end(), system) !=
                                    current_thread_systems->end();
    while (!is_current_thread_system && system->IsRunning()) {
      try_time_ms += kSleepTimeMs;
      if (try_time_ms >= max_try_time_ms) {
        BAIL(err, nv3dvc::core::Error::ERR_TIMEOUT);
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(kSleepTimeMs));
    }
    CHECK_CU_SUCCESS(cuStreamSynchronize(system->GetStream()));
  }
bail:
  return err;
}

static void ResumePauseStates(std::vector<std::shared_ptr<nv3dvc::core::ecs::System>>* all_systems,
                              const std::vector<bool>& pause_states) {
  // Reset pause states
  for (size_t i = 0; i < all_systems->size(); i++) {
    (*all_systems)[i]->SetPaused(pause_states[i]);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Member function definitions                                                                                      ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

nv3dvc::api::reframing::Reframing::Impl::SafeSceneEdit::SafeSceneEdit(
    nv3dvc::api::reframing::Reframing::Impl* reframing_impl, Pipeline* current_pipeline)
    : m_impl(reframing_impl) {
  if (current_pipeline) {
    StopSystemsAndRecordPauseState(&m_impl->all_systems, &current_pipeline->systems, &m_threadSystemPauseStates);
  }
  m_impl->engine.OnUnloadScene();
}

nv3dvc::api::reframing::Reframing::Impl::SafeSceneEdit::~SafeSceneEdit() {
  m_impl->engine.OnLoadScene();
  ResumePauseStates(&m_impl->all_systems, m_threadSystemPauseStates);
}

namespace nv3dvc {

void api::reframing::Reframing::Impl::RegisterModules() {
  window_module = &engine.RegisterModule<modules::windowmodule::WindowModule>(&engine);
  render_module = &engine.RegisterModule<modules::rendermodule::RenderModule>(&engine);
  common_module = &engine.RegisterModule<modules::commonmodule::CommonModule>();
  behavior_module = &engine.RegisterModule<modules::behaviormodule::BehaviorModule>(nullptr);
  control_module = &engine.RegisterModule<modules::controlmodule::ControlModule>(&engine.Control(), behavior_module);
  tracking_module = &engine.RegisterModule<modules::trackingmodule::TrackingModule>(&engine);
  video_effects_module = &engine.RegisterModule<modules::videoeffectsmodule::VideoEffectsModule>(&engine);
  volumetric_encoding_module = &engine.RegisterModule<modules::volumetricencodingmodule::VolumetricEncodingModule>();
  triplane_module = &engine.RegisterModule<modules::triplanemodule::TriplaneModule>(behavior_module);
}

void api::reframing::Reframing::Impl::UnregisterModules() {
  engine.UnregisterModule<modules::windowmodule::WindowModule>();
  engine.UnregisterModule<modules::rendermodule::RenderModule>();
  engine.UnregisterModule<modules::commonmodule::CommonModule>();
  engine.UnregisterModule<modules::behaviormodule::BehaviorModule>();
  engine.UnregisterModule<modules::controlmodule::ControlModule>();
  engine.UnregisterModule<modules::trackingmodule::TrackingModule>();
  engine.UnregisterModule<modules::videoeffectsmodule::VideoEffectsModule>();
  engine.UnregisterModule<modules::volumetricencodingmodule::VolumetricEncodingModule>();
  engine.UnregisterModule<modules::triplanemodule::TriplaneModule>();
}

void api::reframing::Reframing::Impl::PopulateSystemPointers() {
  head_tracking_system = engine.GetSystem<modules::trackingmodule::systems::HeadTrackingSystem>();
  aigs_system = engine.GetSystem<modules::videoeffectsmodule::systems::AigsSystem>();
  triplane_encoder_system = engine.GetSystem<modules::volumetricencodingmodule::systems::TriplaneEncoderSystem>();
  behavior_system = engine.GetSystem<modules::behaviormodule::systems::BehaviorSystem>();
  render_system = engine.GetSystem<modules::rendermodule::systems::RenderSystem>();
}

void api::reframing::Reframing::Impl::SetupMainPipeline() {
  main_pipeline = std::make_unique<Pipeline>();
  main_pipeline->systems.push_back(behavior_system);
  main_pipeline->systems.push_back(render_system);
}

void api::reframing::Reframing::Impl::SetupTrackingPipeline(const bool threaded) {
  std::vector<std::shared_ptr<nv3dvc::core::ecs::System>> tracking_systems;
  tracking_systems.push_back(head_tracking_system);
  tracking_systems.push_back(aigs_system);
  tracking_systems.push_back(triplane_encoder_system);

  if (threaded) {
    tracking_pipeline = std::make_unique<Pipeline>();
    tracking_pipeline->systems = tracking_systems;
  } else {
    main_pipeline->systems.insert(main_pipeline->systems.begin(), tracking_systems.begin(), tracking_systems.end());
  }
}

void api::reframing::Reframing::Impl::BuildSubjectInScene(SubjectStruct* subject_struct, ScenePreset scene_preset) {
  auto subject_root_entity = AddRoot(subject_struct);
  auto display_entity = AddDisplay(subject_struct, subject_root_entity);
  auto webcam_entity = AddWebcam(subject_struct, display_entity);

  if (subject_struct->id == NV3DVC_REFRAMING_SUBJECT_SPECTATOR) {
    subject_struct->internal.root_transform_component->rotation = glm::quat(1, 0, 0, 0);
    subject_struct->internal.root_transform_component->translation = glm::vec3(0, 0, 0);
  } else {
    subject_struct->internal.root_transform_component->rotation =
        glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    subject_struct->internal.root_transform_component->translation = glm::vec3(0, 0, 0);

    auto video_out_entity = AddVideoOut(subject_struct, webcam_entity);
    auto triplane_entity = AddTriplane(subject_struct, display_entity);
    if (scene_preset == ScenePreset::STATICREFRAMING) {
      auto& renderable_triplane_component =
          triplane_entity.GetComponent<modules::rendermodule::components::RenderableTriplaneComponent>();
      auto& encoded_triplane =
          video_out_entity.GetComponent<modules::volumetricencodingmodule::components::EncodedTriplaneComponent>();
      encoded_triplane.fix_rotation = true;
      encoded_triplane.fix_translation = true;
    }
  }
}

core::ecs::Entity api::reframing::Reframing::Impl::AddRoot(SubjectStruct* subject_struct) {
  auto subject_root_entity = engine.CreateEntity("Subject " + std::to_string(subject_struct->id));
  subject_struct->internal.root_transform_component =
      &subject_root_entity.AddComponent<modules::commonmodule::components::TransformComponent>();
  subject_struct->internal.entities[EntitySelector::ROOT] = subject_root_entity;
  return subject_root_entity;
}

core::ecs::Entity api::reframing::Reframing::Impl::AddDisplay(SubjectStruct* subject_struct,
                                                              const core::ecs::Entity& parent) {
  auto display_entity = engine.CreateEntity("Display " + std::to_string(subject_struct->id));
  display_entity.AddComponent<modules::commonmodule::components::TransformComponent>();
  auto& display_component = display_entity.AddComponent<modules::rendermodule::components::DisplayComponent>();
  display_component.display_type = core::rendering::display::DisplayType::VIRTUAL_DISPLAY;
  display_component.enforce_render_size = true;
  display_component.disable_rendering = true;
  subject_struct->internal.display_component = &display_component;
  subject_struct->internal.camera_component =
      &display_entity.AddComponent<modules::commonmodule::components::CameraComponent>();
  subject_struct->internal.stereo_view_component =
      &display_entity.AddComponent<modules::rendermodule::components::StereoViewComponent>();
  subject_struct->internal.stereo_view_component->flip_y = true;  // Because we read from GL to CV image coords
  display_entity.SetParent(parent);

  auto& recording_callback_component =
      display_entity.AddComponent<modules::commonmodule::components::RecordingCallbackComponent>();

  modules::commonmodule::components::RecordingCallbackComponent::OnFiredCallback render_callback =
      [&, subject_id = subject_struct->id](void* data_ptr, size_t data_size, size_t* processed_size, int data_type) {
        core::Error err = core::Error::SUCCESS;
        NvCVImage img_wrapper;
        SubjectStruct* subject_struct = nullptr;
        CHECK_TRUE(subjects.find(subject_id) != subjects.end(), core::Error::ERR_ITEM_NOT_FOUND);
        subject_struct = &subjects[subject_id];

        // Ok. We won't force setting of output image
        CHECK_NONNULL(subject_struct->user.output_image, core::Error::SUCCESS);
        switch (data_type) {
          case modules::commonmodule::components::RecordingCallbackComponent::VIDEO_FORMAT:
            subject_struct->internal.output_image_format =
                *reinterpret_cast<modules::commonmodule::components::RecordingCallbackComponent::VideoFormat*>(
                    data_ptr);
            break;
          case modules::commonmodule::components::RecordingCallbackComponent::VIDEO_DATA:
            CHECK_NVCV_SUCCESS(NvCVImage_Init(&img_wrapper, subject_struct->internal.output_image_format.width,
                                              subject_struct->internal.output_image_format.height,
                                              subject_struct->internal.output_image_format.pitch, data_ptr,
                                              subject_struct->internal.output_image_format.pixel_format,
                                              subject_struct->internal.output_image_format.component_type,
                                              subject_struct->internal.output_image_format.planar,
                                              subject_struct->internal.output_image_format.gpu_mem));
            CHECK_NVCV_SUCCESS(
                NvCVImage_Transfer(&img_wrapper, subject_struct->user.output_image, 1.0f, nullptr, nullptr));
            *processed_size = data_size;
            break;
          default:
            return core::Error::ERR_UNIMPLEMENTED;
            break;
        }
      bail:
        return err;
      };
  recording_callback_component.SetOnFiredCallback(render_callback);

  subject_struct->internal.entities[EntitySelector::DISPLAY] = display_entity;
  return display_entity;
}

core::ecs::Entity api::reframing::Reframing::Impl::AddWebcam(SubjectStruct* subject_struct,
                                                             const core::ecs::Entity& parent) {
  auto webcam_entity = engine.CreateEntity("Webcam " + std::to_string(subject_struct->id));
  webcam_entity.AddComponent<nv3dvc::modules::controlmodule::components::PoseCalibrationBehavior>()
      .calibrate_on_startup = false;
  subject_struct->internal.camera_transform_component =
      &webcam_entity.AddComponent<nv3dvc::modules::commonmodule::components::TransformComponent>();
  // Turn camera 180 degrees to point along the display normal
  subject_struct->internal.camera_transform_component->rotation =
      glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
  // Full view webcam video frame
  subject_struct->internal.video_frame_component =
      &webcam_entity.AddComponent<nv3dvc::modules::commonmodule::components::VideoFrameComponent>();
  // Padding is not allowed when the input image is allocated by the user
  subject_struct->internal.video_frame_component->pad_square = false;
  subject_struct->internal.video_frame_component->padding = glm::uvec2(0, 0);
  subject_struct->internal.camera_calibration_component =
      &webcam_entity.AddComponent<nv3dvc::modules::commonmodule::components::CameraCalibrationComponent>();
  subject_struct->internal.camera_calibration_component->use_vfov = true;
  subject_struct->internal.camera_calibration_component->vfov = 30.0f;

  webcam_entity.AddComponent<nv3dvc::modules::trackingmodule::components::TrackedHeadComponent>();
  webcam_entity.SetParent(parent);

  subject_struct->internal.entities[EntitySelector::WEBCAM] = webcam_entity;
  return webcam_entity;
}

core::ecs::Entity api::reframing::Reframing::Impl::AddVideoOut(SubjectStruct* subject_struct,
                                                               const core::ecs::Entity& parent) {
  auto video_out_entity = engine.CreateEntity("CropAndTriplane " + std::to_string(subject_struct->id));
  // Cropped head video frame
  subject_struct->internal.cropped_video_frame_component =
      &video_out_entity.AddComponent<nv3dvc::modules::commonmodule::components::VideoFrameComponent>();
  video_out_entity.AddComponent<nv3dvc::modules::videoeffectsmodule::components::VideoEffectsComponent>();  // AIGS
  video_out_entity.AddComponent<nv3dvc::modules::volumetricencodingmodule::components::EncodedTriplaneComponent>();
  video_out_entity.SetParent(parent);

  subject_struct->internal.entities[EntitySelector::VIDEO_OUT] = video_out_entity;
  return video_out_entity;
}

core::ecs::Entity api::reframing::Reframing::Impl::AddTriplane(SubjectStruct* subject_struct,
                                                               const core::ecs::Entity& parent) {
  auto triplane_entity = engine.CreateEntity("TriplaneVolume " + std::to_string(subject_struct->id));
  triplane_entity.AddComponent<nv3dvc::modules::rendermodule::components::RenderableTriplaneComponent>();
  triplane_entity.AddComponent<nv3dvc::modules::triplanemodule::components::TriplaneBufferComponent>();
  triplane_entity.AddComponent<nv3dvc::modules::commonmodule::components::TransformComponent>();
  triplane_entity.SetParent(parent);
  subject_struct->internal.entities[EntitySelector::TRIPLANE] = triplane_entity;
  return triplane_entity;
}

template <typename T>
core::Error api::reframing::Reframing::Impl::SetProperty(const int32_t subject_id, const EntitySelector entity_selector,
                                                         const const char* component_name,
                                                         const const char* property_name, const T& val) {
  core::Error err = core::Error::SUCCESS;
  core::Error err_tmp = core::Error::SUCCESS;
  SubjectStruct* subject_struct = nullptr;
  nlohmann::json json_description;
  std::unordered_map<std::string, core::properties::PropertyOwner*> sub_owners;
  core::properties::PropertyOwner* prop_owner;
  core::ecs::Entity entity;
  std::decay<decltype(subject_struct->internal.entities.begin())>::type found_entity;  // Iterator
  std::decay<decltype(sub_owners.begin())>::type found_component;                      // Iterator
  std::decay<decltype(json_description.begin())>::type found_prop;                     // Iterator

  GET_SUBJECT_OR_BAIL(this, subject_struct, subject_id);

  found_entity = subject_struct->internal.entities.find(entity_selector);
  CHECK_FALSE(found_entity == subject_struct->internal.entities.end(), core::Error::ERR_ITEM_NOT_FOUND,
              "The subject does not have this entity");
  entity = found_entity->second;
  CHECK_TRUE(entity.IsValid(), core::Error::ERR_SCENE);
  CHECK_TRUE(entity.HasComponent<core::properties::PropertyOwner>(), core::Error::ERR_SCENE);

  core::properties::PropertyOwner& super_owner = entity.GetComponent<core::properties::PropertyOwner>();
  sub_owners = super_owner.SubOwners();
  found_component = sub_owners.find(std::string(component_name));
  CHECK_FALSE(found_component == sub_owners.end(), core::Error::ERR_ITEM_NOT_FOUND,
              "A component of the provided name %s does not exist for this entity", component_name);

  prop_owner = found_component->second;
  json_description[std::string(property_name)] = val;
  CHECK_SUCCESS(core::serialization::DecodeProperties<T>(json_description, prop_owner),
                "A property of the provided name and type (%s) does not exist for the component %s", property_name,
                component_name);
bail:
  return err;
}

template <typename T>
core::Error api::reframing::Reframing::Impl::GetProperty(const int32_t subject_id, const EntitySelector entity_selector,
                                                         const const char* component_name,
                                                         const const char* property_name, T* val) {
  core::Error err = core::Error::SUCCESS;
  core::Error err_tmp = core::Error::SUCCESS;
  SubjectStruct* subject_struct = nullptr;
  nlohmann::json json_description;
  std::unordered_map<std::string, core::properties::PropertyOwner*> sub_owners;
  core::properties::PropertyOwner* prop_owner;
  core::ecs::Entity entity;
  std::decay<decltype(subject_struct->internal.entities.begin())>::type found_entity;  // Iterator
  std::decay<decltype(sub_owners.begin())>::type found_component;                      // Iterator
  std::decay<decltype(json_description.begin())>::type found_prop;                     // Iterator

  CHECK_NONNULL(val, core::Error::ERR_NULL_POINTER);
  GET_SUBJECT_OR_BAIL(this, subject_struct, subject_id);
  found_entity = subject_struct->internal.entities.find(entity_selector);
  CHECK_FALSE(found_entity == subject_struct->internal.entities.end(), core::Error::ERR_ITEM_NOT_FOUND,
              "The subject does not have this entity");
  entity = found_entity->second;
  CHECK_TRUE(entity.IsValid(), core::Error::ERR_SCENE);
  CHECK_TRUE(entity.HasComponent<core::properties::PropertyOwner>(), core::Error::ERR_SCENE);

  core::properties::PropertyOwner& super_owner = entity.GetComponent<core::properties::PropertyOwner>();
  sub_owners = super_owner.SubOwners();
  found_component = sub_owners.find(std::string(component_name));
  CHECK_FALSE(found_component == sub_owners.end(), core::Error::ERR_ITEM_NOT_FOUND,
              "A component of the provided name %s does not exist for this entity", component_name);

  prop_owner = found_component->second;
  CHECK_SUCCESS(core::serialization::EncodeProperties<T>(&json_description, prop_owner));
  found_prop = json_description.find(std::string(property_name));
  CHECK_FALSE(found_prop == json_description.end(), core::Error::ERR_ITEM_NOT_FOUND,
              "A property of the provided name and type (%s) does not exist for the component %s", property_name,
              component_name);
  *val = *found_prop;
bail:
  return err;
}

namespace api {
namespace reframing {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Public functions definitions                                                                                     ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

core::Error CudaStreamCreate(CUstream* stream) {
  core::Error err = core::Error::SUCCESS;
  CHECK_CU_SUCCESS(cuStreamCreate(stream, CU_STREAM_DEFAULT), "Failed to create CUDA stream");
bail:
  return err;
}

core::Error CudaStreamDestroy(CUstream stream) {
  core::Error err = core::Error::SUCCESS;
  CHECK_CU_SUCCESS(cuStreamDestroy(stream), "Failed to destroy CUDA stream");
bail:
  return err;
}

core::Error CudaStreamSynchronize(CUstream stream) {
  core::Error err = core::Error::SUCCESS;
  CHECK_CU_SUCCESS(cuStreamSynchronize(stream), "Failed to synchronize CUDA stream");
bail:
  return err;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Public class member functions definitions                                                                        ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Reframing::Reframing() : m_impl(new Impl()) {}

Reframing::~Reframing() { delete m_impl; }

InitParams Reframing::GetDefaultInitParams() {
  // These are the defaults
  InitParams init_params = {
      1,  // Version
      0,  // multithreading
  };
  return init_params;
}

uint32_t Reframing::GetVersion() const { return m_impl->version; }

core::Error Reframing::Initialize(const InitParams* init_params) {
  core::Error err = core::Error::SUCCESS;

  m_impl->init_params = init_params ? *init_params : GetDefaultInitParams();
  m_impl->RegisterModules();
  m_impl->PopulateSystemPointers();
  m_impl->SetupMainPipeline();
  m_impl->SetupTrackingPipeline(m_impl->init_params.multithreading == 1);

  if (m_impl->main_pipeline) {
    m_impl->all_systems.insert(m_impl->all_systems.begin(), m_impl->main_pipeline->systems.begin(),
                               m_impl->main_pipeline->systems.end());
  }
  if (m_impl->tracking_pipeline) {
    m_impl->all_systems.insert(m_impl->all_systems.begin(), m_impl->tracking_pipeline->systems.begin(),
                               m_impl->tracking_pipeline->systems.end());
  }
  // Configure modules
  m_impl->window_module->window_mode = core::application::Window::WindowMode::HIDDEN;
  m_impl->window_module->enable_vsync = false;
  // Configure systems
  m_impl->head_tracking_system->limit_head_confidence.get()->store(false);
  m_impl->render_system->record_frames_gpu = true;
  // CHECK_SUCCESS(m_impl->engine.InitializeCudaDevice());
  CHECK_SUCCESS(m_impl->engine.InitializeModules());
  CHECK_SUCCESS(m_impl->engine.InitializeSystems());

  // Play
  if (m_impl->main_pipeline) {
    for (auto& system : m_impl->main_pipeline->systems) {
      system->SetPaused(false);
    }
  }
  if (m_impl->tracking_pipeline) {
    // Main CUDA stream is set by user; tracking pipeline CUDA stream is set by us.
    CudaStreamCreate(&m_impl->tracking_pipeline->stream);
    for (auto& system : m_impl->tracking_pipeline->systems) {
      system->SetPaused(false);
      system->SetStream(m_impl->tracking_pipeline->stream);
    }
  }

bail:
  return err;
}

core::Error Reframing::Uninitialize() {
  core::Error err = core::Error::SUCCESS;
  m_impl->engine.Control().Queue<core::engine::command::Close>();
  // Executes the last command
  core::Error err_tmp = m_impl->engine.UpdateModules(1.0f / 60.0f);
  if (err == core::Error::SUCCESS) err = err_tmp;  // Continue, but save first error

  if (m_impl->tracking_pipeline) {
    if (m_impl->tracking_pipeline->stream != nullptr) {
      CudaStreamDestroy(m_impl->tracking_pipeline->stream);
      m_impl->tracking_pipeline->stream = nullptr;
    }
  }

  CHECK_SUCCESS(m_impl->engine.OnUnloadScene());
  CHECK_SUCCESS(m_impl->engine.Uninitialize());
  m_impl->UnregisterModules();
bail:
  return err;
}

core::Error Reframing::ConfigureLogger(int verbosity, const char* file, void (*cb)(void*, const char*), void* cb_data) {
  core::Error err = core::Error::SUCCESS;

  m_impl->engine.log_level = verbosity;
  err = static_cast<core::Error>(core::util::gLogger.init(m_impl->engine.log_level, file, cb, cb_data));
  return err;
}

core::Error Reframing::AddSubject(const int32_t subject_id, const ScenePreset scene_preset) {
  core::Error err = core::Error::SUCCESS;
  SubjectStruct subject_struct;
  CHECK_TRUE(m_impl->subjects.find(subject_id) == m_impl->subjects.end(), core::Error::ERR_SCENE,
             "Subject already in scene");
  CHECK_TRUE(m_impl->subjects.size() < MAX_NUM_SUPPORTED_SUBJECTS, core::Error::ERR_SCENE,
             "The maximum number of supported subjects in scene is %d", MAX_NUM_SUPPORTED_SUBJECTS);

  if (scene_preset == ScenePreset::NONE) return core::Error::ERR_SCENE;

  subject_struct.id = subject_id;
  try {
    nv3dvc::api::reframing::Reframing::Impl::SafeSceneEdit safe_scene_edit(m_impl, m_impl->main_pipeline.get());
    m_impl->BuildSubjectInScene(&subject_struct, scene_preset);
  } catch (const std::exception& e) {
    BAIL(err, core::Error::ERR_SCENE);
  }

  m_impl->subjects.emplace(subject_id, subject_struct);

bail:
  return err;
}

core::Error Reframing::RemoveSubject(const int32_t subject_id) {
  core::Error err = core::Error::SUCCESS;
  SubjectStruct* subject_struct = nullptr;
  GET_SUBJECT_OR_BAIL(m_impl, subject_struct, subject_id);

  if (subject_struct->internal.entities[EntitySelector::ROOT].IsValid()) {
    CHECK_SUCCESS(m_impl->engine.DestroyEntity(subject_struct->internal.entities[EntitySelector::ROOT]),
                  "Failed to destroy root_entity");
  }
  if (subject_struct->internal.entities[EntitySelector::DISPLAY].IsValid()) {
    CHECK_SUCCESS(m_impl->engine.DestroyEntity(subject_struct->internal.entities[EntitySelector::DISPLAY]),
                  "Failed to destroy display_entity");
  }
  if (subject_struct->internal.entities[EntitySelector::WEBCAM].IsValid()) {
    CHECK_SUCCESS(m_impl->engine.DestroyEntity(subject_struct->internal.entities[EntitySelector::WEBCAM]),
                  "Failed to destroy webcam_entity");
  }
  if (subject_struct->internal.entities[EntitySelector::VIDEO_OUT].IsValid()) {
    CHECK_SUCCESS(m_impl->engine.DestroyEntity(subject_struct->internal.entities[EntitySelector::VIDEO_OUT]),
                  "Failed to destroy video_out_entity");
  }
  if (subject_struct->internal.entities[EntitySelector::TRIPLANE].IsValid()) {
    CHECK_SUCCESS(m_impl->engine.DestroyEntity(subject_struct->internal.entities[EntitySelector::TRIPLANE]),
                  "Failed to destroy triplane_entity");
  }
  m_impl->subjects.erase(subject_id);
bail:
  return err;
}

core::Error Reframing::SetInputImage(const int32_t subject_id, NvCVImage* image, const int64_t timestamp) {
  core::Error err = core::Error::SUCCESS;
  SubjectStruct* subject_struct = nullptr;
  GET_SUBJECT_OR_BAIL(m_impl, subject_struct, subject_id);
  CHECK_NONNULL(image, core::Error::ERR_NULL_POINTER);
  CHECK_SUCCESS(subject_struct->internal.video_frame_component->Init(image, 0, 0, image->width, image->height));

  subject_struct->internal.video_frame_component->SetTimeStamp(timestamp);
  subject_struct->internal.stereo_view_component->SetTimeStamp(timestamp);

  subject_struct->internal.stereo_view_component->filter_view = true;
  subject_struct->internal.stereo_view_component->detect_static_eyes = true;
  subject_struct->internal.stereo_view_component->fix_focal_length = true;
  subject_struct->internal.camera_calibration_component->image_width = image->width;
  subject_struct->internal.camera_calibration_component->image_height = image->height;
  subject_struct->internal.camera_calibration_component->Update();
  subject_struct->user.input_image = image;
bail:
  return err;
}

core::Error Reframing::SetViewPoint(const int32_t subject_id, float x, float y, float z, const int64_t timestamp) {
  core::Error err = core::Error::SUCCESS;
  SubjectStruct* subject_struct = nullptr;
  GET_SUBJECT_OR_BAIL(m_impl, subject_struct, subject_id);
  CHECK_SUCCESS(subject_struct->internal.video_frame_component->UnInit());
  subject_struct->internal.stereo_view_component->detect_static_eyes = false;
  subject_struct->internal.stereo_view_component->fix_focal_length = false;
  subject_struct->internal.stereo_view_component->parallax_interpolation_factor = 1.0f;
  subject_struct->internal.stereo_view_component->SetIsConfident(true);
  subject_struct->internal.stereo_view_component->filter_view = false;

  const glm::vec3 eye = {x, y, z};  // Same viewpoint for both eyes
  const float dt = 0.0f;            // Filtering is disabled, dt can be 0.0
  subject_struct->internal.stereo_view_component->SetEyes(eye, eye, dt);
  subject_struct->internal.stereo_view_component->SetTimeStamp(timestamp);
bail:
  return err;
}

core::Error Reframing::SetCameraTransformTranslation(const int32_t subject_id, float x, float y, float z,
                                                     const int64_t timestamp) {
  core::Error err = core::Error::SUCCESS;
  SubjectStruct* subject_struct = nullptr;
  GET_SUBJECT_OR_BAIL(m_impl, subject_struct, subject_id);
  subject_struct->internal.camera_transform_component->SetTranslation({x, y, z});
  subject_struct->timestamp = timestamp;
bail:
  return err;
}
core::Error Reframing::SetCameraTransformRotationQuat(const int32_t subject_id, float x, float y, float z, float w,
                                                      const int64_t timestamp) {
  core::Error err = core::Error::SUCCESS;
  SubjectStruct* subject_struct = nullptr;
  GET_SUBJECT_OR_BAIL(m_impl, subject_struct, subject_id);
  subject_struct->internal.camera_transform_component->SetRotation({w, x, y, z});
  subject_struct->timestamp = timestamp;
bail:
  return err;
}

core::Error Reframing::SetCameraIntrinsicParams(const int32_t subject_id, float fx, float fy, float cx, float cy,
                                                const int64_t timestamp) {
  core::Error err = core::Error::SUCCESS;
  SubjectStruct* subject_struct = nullptr;
  GET_SUBJECT_OR_BAIL(m_impl, subject_struct, subject_id);
  subject_struct->internal.camera_calibration_component->use_vfov = false;
  subject_struct->internal.camera_calibration_component->fx = fx;
  subject_struct->internal.camera_calibration_component->fy = fy;
  subject_struct->internal.camera_calibration_component->cx = cx;
  subject_struct->internal.camera_calibration_component->cy = cy;
  subject_struct->timestamp = timestamp;
bail:
  return err;
}

core::Error Reframing::SetCameraVfov(const int32_t subject_id, const float v_fov, const int64_t timestamp) {
  core::Error err = core::Error::SUCCESS;
  SubjectStruct* subject_struct = nullptr;
  GET_SUBJECT_OR_BAIL(m_impl, subject_struct, subject_id);
  subject_struct->internal.camera_calibration_component->use_vfov = true;
  subject_struct->internal.camera_calibration_component->vfov = v_fov;
  subject_struct->timestamp = timestamp;
bail:
  return err;
}

core::Error Reframing::SetRootModelTransformTranslation(const int32_t subject_id, float x, float y, float z,
                                                        const int64_t timestamp) {
  core::Error err = core::Error::SUCCESS;
  SubjectStruct* subject_struct = nullptr;
  GET_SUBJECT_OR_BAIL(m_impl, subject_struct, subject_id);

  subject_struct->internal.root_transform_component->SetTranslation({x, y, z});
  CHECK_SUCCESS(UpdateCamera(subject_struct));
  subject_struct->timestamp = timestamp;
bail:
  return err;
}

core::Error Reframing::SetRootModelTransformRotationQuat(const int32_t subject_id, float x, float y, float z, float w,
                                                         const int64_t timestamp) {
  core::Error err = core::Error::SUCCESS;
  SubjectStruct* subject_struct = nullptr;
  GET_SUBJECT_OR_BAIL(m_impl, subject_struct, subject_id);

  subject_struct->internal.root_transform_component->SetRotation({w, x, y, z});
  CHECK_SUCCESS(UpdateCamera(subject_struct));
  subject_struct->timestamp = timestamp;
bail:
  return err;
}

core::Error Reframing::SetOutputImage(const int32_t subject_id, NvCVImage* image) {
  core::Error err = core::Error::SUCCESS;
  SubjectStruct* subject_struct = nullptr;
  GET_SUBJECT_OR_BAIL(m_impl, subject_struct, subject_id);
  CHECK_NONNULL(image, core::Error::ERR_NULL_POINTER);

  {
    nv3dvc::api::reframing::Reframing::Impl::SafeSceneEdit safe_scene_edit(m_impl, m_impl->main_pipeline.get());
    // Configure display
    subject_struct->internal.display_component->render_width = image->width;
    subject_struct->internal.display_component->render_height = image->height;
    subject_struct->internal.display_component->disable_rendering = true;

    if (subject_id == NV3DVC_REFRAMING_SUBJECT_SPECTATOR) {
      subject_struct->internal.display_component->disable_rendering = false;
      subject_struct->internal.camera_component->SetMainCamera();
    }
  }
  subject_struct->user.output_image = image;
  m_impl->id_to_render = subject_id;
bail:
  return err;
}

core::Error Reframing::SetCudaStream(CUstream cu_stream) {
  core::Error err = core::Error::SUCCESS;
  for (auto& system : m_impl->main_pipeline->systems) {
    system->SetStream(cu_stream);
  }
  return err;
}

// Basic types
IMPLEMENT_SETGETPROPERTY(bool)
IMPLEMENT_SETGETPROPERTY(int16_t)
IMPLEMENT_SETGETPROPERTY(uint16_t)
IMPLEMENT_SETGETPROPERTY(int32_t)
IMPLEMENT_SETGETPROPERTY(int64_t)
IMPLEMENT_SETGETPROPERTY(uint32_t)
IMPLEMENT_SETGETPROPERTY(uint64_t)
IMPLEMENT_SETGETPROPERTY(signed char)
IMPLEMENT_SETGETPROPERTY(unsigned char)
IMPLEMENT_SETGETPROPERTY(float)
IMPLEMENT_SETGETPROPERTY(double)
IMPLEMENT_SETGETPROPERTY(wchar_t)
IMPLEMENT_SETGETPROPERTY(std::string)
// Glm types
IMPLEMENT_SETGETPROPERTY(glm::u8vec1)  // Color R
IMPLEMENT_SETGETPROPERTY(glm::u8vec2)  // Color RG
IMPLEMENT_SETGETPROPERTY(glm::u8vec3)  // Color RGB
IMPLEMENT_SETGETPROPERTY(glm::u8vec4)  // Color RGBA
IMPLEMENT_SETGETPROPERTY(glm::bvec1)
IMPLEMENT_SETGETPROPERTY(glm::bvec2)
IMPLEMENT_SETGETPROPERTY(glm::bvec3)
IMPLEMENT_SETGETPROPERTY(glm::bvec4)
IMPLEMENT_SETGETPROPERTY(glm::ivec1)
IMPLEMENT_SETGETPROPERTY(glm::ivec2)
IMPLEMENT_SETGETPROPERTY(glm::ivec3)
IMPLEMENT_SETGETPROPERTY(glm::ivec4)
IMPLEMENT_SETGETPROPERTY(glm::uvec1)
IMPLEMENT_SETGETPROPERTY(glm::uvec2)
IMPLEMENT_SETGETPROPERTY(glm::uvec3)
IMPLEMENT_SETGETPROPERTY(glm::uvec4)
IMPLEMENT_SETGETPROPERTY(glm::vec1)
IMPLEMENT_SETGETPROPERTY(glm::vec2)
IMPLEMENT_SETGETPROPERTY(glm::vec3)
IMPLEMENT_SETGETPROPERTY(glm::vec4)
IMPLEMENT_SETGETPROPERTY(glm::dvec1)
IMPLEMENT_SETGETPROPERTY(glm::dvec2)
IMPLEMENT_SETGETPROPERTY(glm::dvec3)
IMPLEMENT_SETGETPROPERTY(glm::dvec4)
IMPLEMENT_SETGETPROPERTY(glm::mat2x2)
IMPLEMENT_SETGETPROPERTY(glm::mat2x3)
IMPLEMENT_SETGETPROPERTY(glm::mat2x4)
IMPLEMENT_SETGETPROPERTY(glm::mat3x2)
IMPLEMENT_SETGETPROPERTY(glm::mat3x3)
IMPLEMENT_SETGETPROPERTY(glm::mat3x4)
IMPLEMENT_SETGETPROPERTY(glm::mat4x2)
IMPLEMENT_SETGETPROPERTY(glm::mat4x3)
IMPLEMENT_SETGETPROPERTY(glm::mat4x4)
IMPLEMENT_SETGETPROPERTY(glm::dmat2x2)
IMPLEMENT_SETGETPROPERTY(glm::dmat2x3)
IMPLEMENT_SETGETPROPERTY(glm::dmat2x4)
IMPLEMENT_SETGETPROPERTY(glm::dmat3x2)
IMPLEMENT_SETGETPROPERTY(glm::dmat3x3)
IMPLEMENT_SETGETPROPERTY(glm::dmat3x4)
IMPLEMENT_SETGETPROPERTY(glm::dmat4x2)
IMPLEMENT_SETGETPROPERTY(glm::dmat4x3)
IMPLEMENT_SETGETPROPERTY(glm::dmat4x4)
IMPLEMENT_SETGETPROPERTY(glm::fquat)
IMPLEMENT_SETGETPROPERTY(glm::dquat)

core::Error Reframing::GetViewPoint(int32_t subject_id, float* x, float* y, float* z) {
  core::Error err = core::Error::SUCCESS;
  SubjectStruct* subject_struct = nullptr;
  GET_SUBJECT_OR_BAIL(m_impl, subject_struct, subject_id);
  const glm::vec3 viewpoint = subject_struct->internal.stereo_view_component->MeanViewPoint();
  if (x) *x = viewpoint.x;
  if (y) *y = viewpoint.y;
  if (z) *z = viewpoint.z;
bail:
  return err;
}

core::Error Reframing::GetTriplaneTransformTranslation(int32_t subject_id, float* x, float* y, float* z) {
  core::Error err = core::Error::SUCCESS;
  core::Error err_tmp = core::Error::SUCCESS;
  SubjectStruct* subject_struct = nullptr;
  GET_SUBJECT_OR_BAIL(m_impl, subject_struct, subject_id);

  CHECK_TRUE(subject_struct->internal.entities[EntitySelector::TRIPLANE].IsValid(), core::Error::ERR_ITEM_NOT_FOUND,
             "There is no triplane entity for this subject");
  CHECK_TRUE(subject_struct->internal.entities[EntitySelector::TRIPLANE]
                 .HasComponent<nv3dvc::modules::commonmodule::components::TransformComponent>(),
             core::Error::ERR_ITEM_NOT_FOUND, "The entity does not have a transform component");
  const auto& transform_component = subject_struct->internal.entities[EntitySelector::TRIPLANE]
                                        .GetComponent<nv3dvc::modules::commonmodule::components::TransformComponent>();
  const glm::vec3 transform = transform_component.GetTranslationFiltered();
  if (x) *x = transform.x;
  if (y) *y = transform.y;
  if (z) *z = transform.z;
bail:
  return err;
}

core::Error Reframing::GetFrameBufferObject(const int32_t subject_id, uint32_t* val) const {
  core::Error err = core::Error::SUCCESS;
  CHECK_NONNULL(m_impl->render_system.get(), core::Error::ERR_INITIALIZATION);
  CHECK_TRUE(subject_id == m_impl->id_to_render, core::Error::ERR_PARAMETER_MISMATCH,
             "Output image does not exist for this subject: %d", subject_id);
  CHECK_SUCCESS(m_impl->render_system->renderer->GetRenderVolumeFrameBufferObject(val));
bail:
  return err;
}

core::Error Reframing::GetColorTextureAttachment(const int32_t subject_id, uint32_t* val) const {
  core::Error err = core::Error::SUCCESS;
  CHECK_NONNULL(m_impl->render_system.get(), core::Error::ERR_INITIALIZATION);
  CHECK_TRUE(subject_id == m_impl->id_to_render, core::Error::ERR_PARAMETER_MISMATCH,
             "Output image does not exist for this subject: %d", subject_id);
  CHECK_SUCCESS(m_impl->render_system->renderer->GetRenderVolumeColorTextureAttachment(val));
bail:
  return err;
}

core::Error Reframing::GetDepthTextureAttachment(const int32_t subject_id, uint32_t* val) const {
  core::Error err = core::Error::SUCCESS;
  CHECK_NONNULL(m_impl->render_system.get(), core::Error::ERR_INITIALIZATION);
  CHECK_TRUE(subject_id == m_impl->id_to_render, core::Error::ERR_PARAMETER_MISMATCH,
             "Output image does not exist for this subject: %d", subject_id);
  CHECK_SUCCESS(m_impl->render_system->renderer->GetRenderVolumeDepthTextureAttachment(val));
bail:
  return err;
}

core::Error Reframing::Run(const int32_t subject_id, const float dt, int64_t* timestamp_out) {
  core::Error err = core::Error::SUCCESS;
  core::Error err_tmp = core::Error::SUCCESS;
  SubjectStruct* subject_struct = nullptr;
  GET_SUBJECT_OR_BAIL(m_impl, subject_struct, subject_id);

  uint32_t timed_entity_id = 0;  // The entity we care about for propagated time stamp
  if (subject_struct->user.output_image != nullptr) {
    err_tmp = m_impl->engine.RunSystems(m_impl->main_pipeline->systems, dt);
    if (err == core::Error::SUCCESS) err = err_tmp;  // Continue, but save first error
    err_tmp = m_impl->engine.UpdateModules(dt);
    if (err == core::Error::SUCCESS) err = err_tmp;  // Continue, but save first error
    timed_entity_id = static_cast<uint32_t>(subject_struct->internal.entities[EntitySelector::DISPLAY]);
  } else {
    err_tmp = m_impl->engine.RunSystems(m_impl->tracking_pipeline->systems, dt);
    if (err == core::Error::SUCCESS) err = err_tmp;  // Continue, but save first error
    timed_entity_id = static_cast<uint32_t>(subject_struct->internal.entities[EntitySelector::TRIPLANE]);
  }
  subject_struct->timestamp =
      std::max(m_impl->engine.GetTimeStampForEntity(timed_entity_id), subject_struct->timestamp);
  if (timestamp_out && subject_struct->timestamp != std::numeric_limits<int64_t>::min()) {
    *timestamp_out = subject_struct->timestamp;
  }
bail:
  return err;
}

}  // namespace reframing
}  // namespace api
}  // namespace nv3dvc
