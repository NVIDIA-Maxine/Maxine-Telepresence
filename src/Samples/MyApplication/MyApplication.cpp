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

#include "Core/Application/Application.h"
#include "Modules/BehaviorModule/BehaviorModule.h"
#include "Modules/CommonModule/CommonModule.h"
#include "Modules/GuiModule/GuiModule.h"
#include "Modules/RenderModule/RenderModule.h"
#include "Modules/WindowModule/WindowModule.h"
#include "Samples/MyModule/MyModule.h"

char* g_nvARSDKPath = nullptr;
char* g_nvVFXSDKPath = nullptr;

namespace samples {

//! [Simple application sample]
class MyApplication : public nv3dvc::core::application::Application {
 public:
  MyApplication() {
    // Register modules to enable all internally registered components to be used in the scene. Components that are
    // registered within a given module can be added to entities, and will get initialized during scene deserialization.
    // Components that have not been registered can not be deserialized.
    // The engine will also create the modules' systems to be registered by the application.
    RegisterModule<nv3dvc::modules::rendermodule::RenderModule>(&GetEngine());
    RegisterModule<nv3dvc::modules::windowmodule::WindowModule>(&GetEngine());
    RegisterModule<nv3dvc::modules::guimodule::GuiModule>(&GetEngine());
    auto& behavior_module = RegisterModule<nv3dvc::modules::behaviormodule::BehaviorModule>(&GetEngine().Input());
    RegisterModule<samples::MyModule>(&behavior_module);

    // This defines the pipeline of systems to execute on the main thread. The order of the template arguments is
    // important as the systems will be executed one by one. Before the main update loop, each system is initialized in
    // the same order.
    RegisterMainThreadSystems<nv3dvc::modules::behaviormodule::systems::BehaviorSystem,
                              nv3dvc::modules::rendermodule::systems::RenderSystem,
                              nv3dvc::modules::guimodule::systems::GuiSystem>();

    // Order GUI system to front, followed by the behavior system. This enables GUI events to be consumed
    OrderToFront<nv3dvc::modules::behaviormodule::systems::BehaviorSystem>();
    OrderToFront<nv3dvc::modules::guimodule::systems::GuiSystem>();
  }

  nv3dvc::core::Error BuildDefaultScene() override {
    try {
      auto sky_box_entity = GetEngine().CreateEntity("SkyBox");
      sky_box_entity.AddComponent<nv3dvc::modules::rendermodule::components::CubeMapComponent>();
      auto camera_entity = GetEngine().CreateEntity("Camera");
      camera_entity.AddComponent<nv3dvc::modules::commonmodule::components::TransformComponent>();
      camera_entity.AddComponent<nv3dvc::modules::commonmodule::components::CameraComponent>();  // Virtual camera
      auto display_entity = GetEngine().CreateEntity("Display");
      display_entity.AddComponent<nv3dvc::modules::rendermodule::components::DisplayComponent>();
    } catch (const std::exception& e) {
      return nv3dvc::core::Error::ERR_SCENE;
    }
    return nv3dvc::core::Error::SUCCESS;
  }
};
//! [Simple application sample]
}  // namespace samples

//! [Run MyApplication]
int main(int argc, char** argv) {
  std::shared_ptr<samples::MyApplication> app = std::make_shared<samples::MyApplication>();
  app->Run();
  return 0;
}
//! [Run MyApplication]
