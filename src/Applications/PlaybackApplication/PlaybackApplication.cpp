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

#include "PlaybackApplication.h"

#include "Modules/AudioModule/AudioModule.h"
#include "Modules/CaptureModule/CaptureModule.h"
#include "Modules/CommonModule/CommonModule.h"
#include "Modules/ControlModule/ControlModule.h"
#include "Modules/GuiModule/GuiModule.h"
#include "Modules/RenderModule/RenderModule.h"
#include "Modules/TrackingModule/TrackingModule.h"
#include "Modules/TriplaneModule/TriplaneModule.h"
#include "Modules/VideoEffectsModule/VideoEffectsModule.h"
#include "Modules/VolumetricEncodingModule/VolumetricEncodingModule.h"
#include "Modules/WindowModule/WindowModule.h"

namespace nv3dvc {
namespace applications {

PlaybackApplication::PlaybackApplication() {
  // Register modules to enable all internally registered components to be used in the scene. Components that are
  // registered within a given module can be added to entities, and will get initialized during scene deserialization.
  // Components that have not been registered can not be deserialized.
  // The engine will also create the modules' systems to be registered by the application.
  RegisterModule<modules::rendermodule::RenderModule>(&GetEngine());
  RegisterModule<modules::windowmodule::WindowModule>(&GetEngine());
  RegisterModule<modules::audiomodule::AudioModule>();
  RegisterModule<modules::guimodule::GuiModule>(&GetEngine());
  RegisterModule<modules::trackingmodule::TrackingModule>(&GetEngine());
  RegisterModule<modules::commonmodule::CommonModule>();
  RegisterModule<modules::volumetricencodingmodule::VolumetricEncodingModule>();
  auto& behavior_module = RegisterModule<modules::behaviormodule::BehaviorModule>(&GetEngine().Input());
  RegisterModule<modules::capturemodule::CaptureModule>(&GetEngine(), &behavior_module);
  RegisterModule<modules::triplanemodule::TriplaneModule>(&behavior_module);
  RegisterModule<modules::controlmodule::ControlModule>(&GetEngine().Control(), &behavior_module);
  RegisterModule<modules::videoeffectsmodule::VideoEffectsModule>(&GetEngine());

  // This defines the pipeline of systems to execute on the main thread. The order of the template arguments is
  // important as the systems will be executed one by one. Before the main update loop, each system is initialized in
  // the same order.
  RegisterMainThreadSystems<modules::behaviormodule::systems::BehaviorSystem,  //
                            modules::rendermodule::systems::RenderSystem,      //
                            modules::guimodule::systems::GuiSystem>();

  // Order GUI system to front, followed by the behavior system. This enables GUI events to be consumed
  OrderToFront<modules::behaviormodule::systems::BehaviorSystem>();
  OrderToFront<modules::guimodule::systems::GuiSystem>();

  // Configure capture and head tracking to run on the same thread
  RegisterThreadSystems<modules::capturemodule::systems::CameraCaptureSystem,
                        modules::trackingmodule::systems::HeadTrackingSystem,
                        modules::videoeffectsmodule::systems::AigsSystem,
                        modules::volumetricencodingmodule::systems::TriplaneEncoderSystem>();

  // Configure audio input and output to run on a thread.
  RegisterThreadSystems<modules::audiomodule::systems::AudioInputSystem,     //
                        modules::audiomodule::systems::AudioFeedbackSystem,  //
                        modules::audiomodule::systems::AudioOutputSystem>();

  // Configure capture and head tracking to run on the same CUDA stream
  ConfigureStream<modules::capturemodule::systems::CameraCaptureSystem,  //
                  modules::trackingmodule::systems::HeadTrackingSystem,  //
                  modules::videoeffectsmodule::systems::AigsSystem,      //
                  modules::volumetricencodingmodule::systems::TriplaneEncoderSystem>();
}

core::Error PlaybackApplication::BuildDefaultScene() {
  try {
    auto sky_box_entity = GetEngine().CreateEntity("SkyBox");
    sky_box_entity.AddComponent<modules::rendermodule::components::CubeMapComponent>();
    sky_box_entity.AddComponent<modules::commonmodule::components::TransformComponent>();

    auto display_entity = GetEngine().CreateEntity("Display");
    display_entity.AddComponent<modules::commonmodule::components::TransformComponent>();
    display_entity.AddComponent<modules::commonmodule::components::CameraComponent>();  // Virtual camera
    display_entity.AddComponent<modules::rendermodule::components::DisplayComponent>();
    display_entity.AddComponent<modules::rendermodule::components::StereoViewComponent>();
    display_entity.AddComponent<modules::controlmodule::components::ViewExtensionBehavior>();
    display_entity.AddComponent<modules::controlmodule::components::CameraControlBehavior>();
    display_entity.AddComponent<modules::audiomodule::components::AudioOutputComponent>();
    display_entity.AddComponent<modules::capturemodule::components::RecordingBehavior>();
    display_entity.AddComponent<modules::commonmodule::components::RecordingCallbackComponent>();

    auto webcam_entity = GetEngine().CreateEntity("Webcam");
    auto& webcam_transform = webcam_entity.AddComponent<modules::commonmodule::components::TransformComponent>();
    // Assume webcam is 20cm above the center of the display, tilted down (around X axis) by 10 degrees.
    webcam_transform.translation = {0.0f, 0.2f, 0.0f};
    // Turn camera 180 degrees to point along the display normal
    webcam_transform.rotation = glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    // Tilt camera down
    webcam_transform.rotation =
        glm::angleAxis(glm::radians(10.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * *webcam_transform.rotation.get();
    webcam_entity.AddComponent<modules::capturemodule::components::WebCameraComponent>();  // Physical camera
    webcam_entity.AddComponent<nv3dvc::modules::commonmodule::components::CameraCalibrationComponent>();
    // Full view webcam video frame
    webcam_entity.AddComponent<modules::commonmodule::components::VideoFrameComponent>();
    webcam_entity.AddComponent<modules::trackingmodule::components::TrackedHeadComponent>();
    webcam_entity.SetParent(display_entity);

    // Recorded participant's display.
    auto remote_display_entity = GetEngine().CreateEntity("RemoteDisplay");
    auto& remote_display_transform =
        remote_display_entity.AddComponent<modules::commonmodule::components::TransformComponent>();
    // Turn remote display 180 degrees to view the object front facing
    remote_display_transform.rotation = glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    auto remote_webcam_entity = GetEngine().CreateEntity("RemoteWebcam");
    remote_webcam_entity.AddComponent<modules::controlmodule::components::PoseCalibrationBehavior>()
        .calibrate_on_startup = true;
    auto& remote_webcam_transform =
        remote_webcam_entity.AddComponent<modules::commonmodule::components::TransformComponent>();
    // Rotate remote webcam transform so that it faces the remote participant
    remote_webcam_transform.rotation = glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    auto& webcam_file = remote_webcam_entity.AddComponent<modules::capturemodule::components::WebCameraComponent>();
    webcam_file.capture_api = modules::capturemodule::capturedevice::CaptureApi::MULTIMEDIA_FILE;
    webcam_file.camera_descriptor.camera_device_path = core::engine::Engine::GetResourcesDir() + "webcam/FaceTrack.mp4";
    remote_webcam_entity.AddComponent<nv3dvc::modules::commonmodule::components::CameraCalibrationComponent>();
    // Full view webcam video frame
    remote_webcam_entity.AddComponent<modules::commonmodule::components::VideoFrameComponent>();
    remote_webcam_entity.AddComponent<modules::trackingmodule::components::TrackedHeadComponent>();
    auto& audio_input = remote_webcam_entity.AddComponent<modules::audiomodule::components::AudioInputComponent>();
    audio_input.capture_api = modules::audiomodule::AudioInputApi::WEB_CAMERA;
    remote_webcam_entity.AddComponent<modules::audiomodule::components::AudioSinkComponent>();
    remote_webcam_entity.AddComponent<modules::audiomodule::components::AudioSourceComponent>();
    remote_webcam_entity.SetParent(remote_display_entity);

    auto video_out_entity = GetEngine().CreateEntity("RemoteCropAndTriplane");
    // Cropped head video frame
    video_out_entity.AddComponent<modules::commonmodule::components::VideoFrameComponent>();
    video_out_entity.AddComponent<modules::videoeffectsmodule::components::VideoEffectsComponent>();  // AIGS
    video_out_entity.AddComponent<modules::volumetricencodingmodule::components::EncodedTriplaneComponent>();
    video_out_entity.SetParent(remote_webcam_entity);

    auto triplane_entity = GetEngine().CreateEntity("TriplaneVolume");
    auto& transform = triplane_entity.AddComponent<modules::commonmodule::components::TransformComponent>();
    triplane_entity.AddComponent<modules::rendermodule::components::RenderableTriplaneComponent>();
    triplane_entity.AddComponent<modules::triplanemodule::components::TriplaneBufferComponent>();
    triplane_entity.SetParent(remote_display_entity);
  } catch (const std::exception& e) {
    return core::Error::ERR_SCENE;
  }
  return core::Error::SUCCESS;
}

}  // namespace applications
}  // namespace nv3dvc
