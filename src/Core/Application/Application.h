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

#ifndef SRC_CORE_APPLICATION_APPLICATION_H_
#define SRC_CORE_APPLICATION_APPLICATION_H_

#include <cuda_runtime_api.h>

#include <memory>
#include <string>
#include <thread>  // NOLINT(build/c++11) (cpplint outside Chromium: https://github.com/google/styleguide/issues/483)
#include <type_traits>
#include <utility>
#include <vector>

#include "Core/Engine/Engine.h"
#include "Core/Error.h"

namespace nv3dvc {
namespace core {
/// @brief Namespace for application classes
namespace application {

/// @brief The Application class contains abstractions for building applications using systems and modules.
///
/// Typical usage:
/// \code{.cpp}
/// class MyApplication : public core::application::Application {
///  public:
///   MyApplication() {
///     RegisterModule<Module1>(); // Registers all components in module
///     RegisterModule<Module2>(); // Registers all components in module
///     // ...
///     // Register a sequence of systems to run on main thread.
///     RegisterMainThreadSystems<System1, System2, System3>();
///     // Register a sequence of systems to run on a separate thread.
///     RegisterThreadSystems<System4, System5, System6>();
///     // Register a sequence of systems to run on a separate thread.
///     RegisterThreadSystems<System7, System8, System9>();
///     // ...
///   }
///
///   // Create scene entities explicitly, or load a scene file.
///   nv3dfvc::core::Error BuildDefaultScene() override {
///     try {
///       // Load entities from a file...
///       GetEngine().LoadScene("scene_file.json");
///       // ...and/or create entities programmatically.
///       auto triplane_entity = GetEngine().CreateEntity();
///       auto& transform = triplane_entity.AddComponent<CommonModule::TransformComponent>();
///       transform.translation = glm::vec3(0.0f, 0.0f, -1.0f);
///       triplane_entity.AddComponent<TriplaneVolumeComponent>();
///     } catch (const std::exception& e) {
///       return nv3dvc::core::Error::ERR_SCENE;
///     }
///     return nv3dvc::core::Error::SUCCESS;
///   }
/// };
///
/// int main(int argc, char** argv) {
///   auto app = std::make_shared<MyApplication>();
///   app->LoadAppConfig("app_config.json");
///   app->Run();
///   return 0;
/// }
/// \endcode
class Application {
  /// @brief Pipeline of systems. Can run on the main thread or separate application threads
  struct SystemPipeline {
    std::vector<std::shared_ptr<ecs::System>> systems;
    std::chrono::system_clock::duration time_since_last_update = std::chrono::system_clock::now().time_since_epoch();
  };

 public:
  /// @brief Constructor. Extend this and register modules and systems for your application
  Application();
  virtual ~Application();

  /// @brief Load application configuration file to initialize property values for all registered modules and systems.
  ///
  /// @see engine::Engine::LoadAppConfig()
  /// @param[in] file_path  The path to the configuration file
  /// @return    SUCCESS    If the application config was loaded and deserialized successfully
  Error LoadAppConfig(const std::string& file_path);

  /// @brief Set the scene config file path
  ///
  /// @see engine::Engine::SetSceneConfigFilePath()
  /// @param[in] file_path The scene config file path to use for loading and saving the scene
  /// @return SUCCESS
  Error SetSceneConfigFilePath(const std::string& file_path);

  /// @brief Set the additional scene config JSON to load after the scene config file
  ///
  /// Any addition scene entities, components or properties in the JSON string will be merged into the scene config
  /// before being deserialized.
  /// @see engine::Engine::SetAdditionalSceneConfigJsonString()
  /// @param[in] json_string The scene config JSON string to merge into the scene config
  /// @return SUCCESS
  Error SetAdditionalSceneConfigJsonString(const std::string& json_string);

  /// @brief Register a module to the engine
  ///
  /// All the module's internal components will be registered so that they can be serialized and deserialized for your
  /// application
  /// @tparam    ModuleType The Module subclass to register
  /// @tparam    Args       Argument types for module creation
  /// @param[in] args,      Input argument for Module subclass constructor
  /// @return    a reference to the created module. Valid until the module is unregistered
  template <class ModuleType, typename... Args>
  ModuleType& RegisterModule(Args&&... args) {
    return m_engine->RegisterModule<ModuleType>(std::forward<Args>(args)...);
  }

  /// @brief Register all systems you want to run on the main thread.
  ///
  /// The systems will be created and added to a pipeline, so the order of the registered systems matters
  /// @tparam ...SystemTypes The types of systems to register to the application's main thread
  template <typename... SystemTypes>
  void RegisterMainThreadSystems() {
    (m_mainPipeline.systems.push_back(m_engine->GetSystem<SystemTypes>()), ...);
  }

  /// @brief Order a registered system to the front of the execution queue
  ///
  /// This will lead to events being passed to this system first, where they can be consumed
  /// Additionally, OnLoadScene and OnUnloadScene will get called on the provided system type first
  /// @tparam SystemType The system to order to the front
  /// @return Error::ERR_GENERAL If the system doesent exist,
  ///                            Or if the application was incorrectly configured
  ///         Error::SUCCESS     If successful
  template <typename SystemType>
  Error OrderToFront() {
    auto system = m_engine->GetSystem<SystemType>();
    if (!system) return Error::ERR_GENERAL;  // Incorrectly configured application
    return m_engine->OrderToFront(system.get());
  }

  /// @brief Create a new CUDA stream and configure the specified systems to use it.
  /// @tparam ...SystemTypes the systems to configure with the new CUDA stream
  template <typename... SystemTypes>
  void ConfigureStream() {
    // Function for creating the stream and setting it to the systems
    // We don't call this function yet, as the engine may not have been initialized and a CUDA context may not exist. We
    // perform calls to m_configStreamFunctions after initializing the engine
    std::function late_stream_config = [&]() {
      cudaStream_t stream = nullptr;
      cudaStreamCreate(&stream);
      (m_engine->GetSystem<SystemTypes>()->SetStream(stream), ...);
    };
    m_configStreamFunctions.push_back(late_stream_config);
  }

  /// @brief Register all systems you want torun on a separate thread.
  ///
  /// The systems will be created and added to a pipeline, so the order of the registered systems matters.
  /// Multiple application threads can be created to run separate systems.
  /// @tparam ...SystemTypes The systems to register to a separate application thread
  template <typename... SystemTypes>
  void RegisterThreadSystems() {
    SystemPipeline thread_pipeline;
    (thread_pipeline.systems.push_back(m_engine->GetSystem<SystemTypes>()), ...);
    m_threadPipelines.push_back(thread_pipeline);
  }

  /// @brief Extend this function to load a scene from a scene registry file, or build a scene explicitly by creating
  /// entities and adding components to them
  virtual Error BuildDefaultScene() { return Error::SUCCESS; }

  /// @brief Run the applicaiton
  ///
  /// Spawns threads defined using RegisterThreadSystems, and runs the registered systems on the main thread registered
  /// using RegisterMainThreadSystems
  void Run();

  /// @brief Get a reference to the Application's engine
  ///
  /// Can be used to create new entities in the scene
  /// @return a reference to the engine
  engine::Engine& GetEngine();

 private:
  /// @brief Spawn the threads defined in RegisterThreadSystems
  void SpawnThreads();
  /// @brief Run the main systems on the main thread
  nv3dvc::core::Error RunMainThread();

  std::unique_ptr<engine::Engine> m_engine;
  SystemPipeline m_mainPipeline;

  std::vector<SystemPipeline> m_threadPipelines;
  std::vector<std::thread> m_threads;
  std::vector<std::function<void()>> m_configStreamFunctions;  // Called after the engine has been initialized, in Run
};

}  // namespace application
}  // namespace core
}  // namespace nv3dvc

#endif  // SRC_CORE_APPLICATION_APPLICATION_H_
