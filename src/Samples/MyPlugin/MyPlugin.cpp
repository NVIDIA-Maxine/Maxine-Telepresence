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

#include "Core/Engine/Engine.h"
#include "Modules/BehaviorModule/BehaviorModule.h"
#include "Modules/CommonModule/CommonModule.h"
#include "Modules/GuiModule/GuiModule.h"
#include "Modules/RenderModule/RenderModule.h"
#include "Modules/TriplaneModule/TriplaneModule.h"
#include "Modules/WindowModule/WindowModule.h"
#include "Samples/MyModule/MyModule.h"

char* g_nvARSDKPath = nullptr;
char* g_nvVFXSDKPath = nullptr;

//! [Simple scene building]
nv3dvc::core::Error BuildScene(nv3dvc::core::engine::Engine* engine) {
  try {
    auto sky_box_entity = engine->CreateEntity("SkyBox");
    sky_box_entity.AddComponent<nv3dvc::modules::rendermodule::components::CubeMapComponent>();
    auto camera_entity = engine->CreateEntity("Camera");
    camera_entity.AddComponent<nv3dvc::modules::commonmodule::components::TransformComponent>();
    camera_entity.AddComponent<nv3dvc::modules::commonmodule::components::CameraComponent>();  // Virtual camera
    auto display_entity = engine->CreateEntity("Display");
    display_entity.AddComponent<nv3dvc::modules::rendermodule::components::DisplayComponent>();
  } catch (std::exception& e) {
    return nv3dvc::core::Error::ERR_SCENE;
  }
  return nv3dvc::core::Error::SUCCESS;
}
//! [Simple scene building]

int main(int argc, char** argv) {
  //! [Using the engine directly]
  nv3dvc::core::Error err = nv3dvc::core::Error::SUCCESS;
  nv3dvc::core::engine::Engine engine;
  std::vector<std::shared_ptr<nv3dvc::core::ecs::System>> systems;
  try {
    // Register modules
    engine.RegisterModule<nv3dvc::modules::commonmodule::CommonModule>();
    engine.RegisterModule<nv3dvc::modules::rendermodule::RenderModule>(&engine);
    engine.RegisterModule<nv3dvc::modules::windowmodule::WindowModule>(&engine);
    auto& behavior_module = engine.RegisterModule<nv3dvc::modules::behaviormodule::BehaviorModule>(&engine.Input());
    engine.RegisterModule<samples::MyModule>(&behavior_module);

    // Create systems
    auto& behavior_system = engine.GetSystem<nv3dvc::modules::behaviormodule::systems::BehaviorSystem>();
    auto& render_system = engine.GetSystem<nv3dvc::modules::rendermodule::systems::RenderSystem>();
    systems = {behavior_system, render_system};
  } catch (const std::exception& e) {
    std::cerr << "Failed to register modules. " << e.what() << std::endl;
    return nv3dvc::core::Error::ERR_GENERAL;
  }

  err = engine.InitializeCudaDevice();
  if (err != nv3dvc::core::Error::SUCCESS) {
    std::cerr << "Failed to initialize CUDA device. " << err << std::endl;
    return err;
  }
  err = engine.InitializeModules();
  if (err != nv3dvc::core::Error::SUCCESS) {
    std::cerr << "Failed to initialize Modules. " << err << std::endl;
    return err;
  }
  err = engine.InitializeSystems();
  if (err != nv3dvc::core::Error::SUCCESS) {
    std::cerr << "Failed to initialize Systems. " << err << std::endl;
    return err;
  }

  // Create a scene
  err = BuildScene(&engine);
  if (err != nv3dvc::core::Error::SUCCESS) {
    std::cerr << "Failed to build scene. " << err << std::endl;
    return err;
  }

  // Main loop
  while (!engine.ShouldClose()) {
    float dt = 1.0f / 60.0f;  // For simplicity, should be the real delta time
    err = engine.BroadcastQueuedEvents();
    if (err != nv3dvc::core::Error::SUCCESS) {
      std::cerr << "Failed to broadcast events. " << err << std::endl;
      return err;
    }
    err = engine.RunSystems(systems, dt);
    if (err != nv3dvc::core::Error::SUCCESS) {
      std::cerr << "Failed to Run Systems. " << err << std::endl;
      return err;
    }
    err = engine.UpdateModules(dt);
    if (err != nv3dvc::core::Error::SUCCESS) {
      std::cerr << "Failed to Update Modules. " << err << std::endl;
      return err;
    }
  }
  //! [Using the engine directly]
  return err;
}
