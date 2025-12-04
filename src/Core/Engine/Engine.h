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

#ifndef SRC_CORE_ENGINE_ENGINE_H_
#define SRC_CORE_ENGINE_ENGINE_H_

#include <cuda.h>

#include <memory>
#include <mutex>  // NOLINT(build/c++11) (cpplint outside Chromium: https://github.com/google/styleguide/issues/483)
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Core/Application/Inputs.h"
#include "Core/Engine/Module.h"
#include "Core/EntityComponentSystem/Entity.h"
#include "Core/EntityComponentSystem/System.h"
#include "Core/Error.h"
#include "Core/Events/Event.h"
#include "Core/Graphics/GraphicsApi.h"
#include "Core/Properties/Property.h"

namespace nv3dvc {
namespace core {
namespace engine {
/// @brief Namespace for engine commands
namespace command {

/// @brief Command types
///
/// Every command type requires an implementation in the form of an extension of the base class Command
enum class CommandType {
  UNKNOWN,             ///< Not defined
  CLOSE,               ///< Command to indicate that the engine should stop running -- Close
  EMPTY_SCENE,         ///< Command to unload the current scene -- EmptyScene
  LOAD_DEFAULT_SCENE,  ///< Command to load the configured default scene -- LoadDefaultScene
  SAVE_APP_CONFIG,     ///< Command to save the current application configuration -- SaveAppConfig
  LOAD_SCENE,          ///< Command to load a scene from a file -- LoadScene
  SAVE_SCENE,          ///< Command to save the current scene to a file -- SaveScene
  PROMPT_USER          ///< Command to display a prompt to the user -- PromptUser
};

/// @brief Command base class. Extend this class for all commands in CommandType
class Command {
 public:
  /// @brief Constructor
  /// @param type The type of command
  explicit Command(CommandType type) : m_type(type) {}

  /// @brief Destructor
  virtual ~Command() = default;

  /// @brief  Get the type of command
  /// @return The type of command
  CommandType Type() const { return m_type; }

  /// @brief  Cast the command to a specific type
  /// @tparam CommandClass The type of command to cast to
  /// @return A reference to the command cast to the specified type
  template <typename CommandClass>
  const CommandClass& As() const {
#ifdef NDEBUG
    return *static_cast<const CommandClass*>(this);
#else   // DEBUG
    return *dynamic_cast<const CommandClass*>(this);
#endif  // *DEBUG
  }

 protected:
  const CommandType m_type;
};

/// @brief Command to indicate that the engine should stop running
class Close : public Command {
 public:
  /// @brief Constructor
  Close() : Command(CommandType::CLOSE) {}
};

/// @brief Command to unload the current scene
class EmptyScene : public Command {
 public:
  /// @brief Constructor
  EmptyScene() : Command(CommandType::EMPTY_SCENE) {}
};

/// @brief Command to load the configured default scene
class LoadDefaultScene : public Command {
 public:
  /// @brief Constructor
  LoadDefaultScene() : Command(CommandType::LOAD_DEFAULT_SCENE) {}
};

/// @brief Command to save the current application configuration
class SaveAppConfig : public Command {
 public:
  /// @brief Constructor
  /// @param file_path The file path to which the app config should be saved
  explicit SaveAppConfig(const std::string& file_path) : Command(CommandType::SAVE_APP_CONFIG), file_path(file_path) {}

  /// @brief Constructor
  ///
  /// Command to save the app config in its default file path, or where previously saved
  SaveAppConfig() : Command(CommandType::SAVE_APP_CONFIG), file_path("") {}

  const std::string file_path;
};

/// @brief Command to load a scene from a file
class LoadScene : public Command {
 public:
  /// @brief Constructor
  /// @param file_path The file path from which the scene should be loaded
  explicit LoadScene(const std::string& file_path) : Command(CommandType::LOAD_SCENE), file_path(file_path) {}

  /// @brief Constructor
  ///
  /// Command to load the scene from its default file path, or where previously loaded
  LoadScene() : Command(CommandType::LOAD_SCENE), file_path("") {}

  const std::string file_path;
};

/// @brief Command to save the current scene to a file
class SaveScene : public Command {
 public:
  /// @brief Constructor
  /// @param file_path The path to the file to save the scene to
  explicit SaveScene(const std::string& file_path) : Command(CommandType::SAVE_SCENE), file_path(file_path) {}

  /// @brief Constructor
  ///
  /// Command to save the scene in its default file path, or where previously saved
  SaveScene() : Command(CommandType::SAVE_SCENE), file_path("") {}

  const std::string file_path;
};

/// @brief Command to display a prompt to the user
class PromptUser : public Command {
 public:
  /// @brief Constructor
  /// @param prompt The prompt to display to the user
  explicit PromptUser(const std::string& prompt) : Command(CommandType::PROMPT_USER), prompt(prompt) {}

  const std::string prompt;
};

}  // namespace command

class EngineControl {
 public:
  template <typename CmdType, typename... CmdArgTypes>
  void Queue(CmdArgTypes... args) {
    std::lock_guard lock(m_commandQueueMutex);
    m_commandQueue.emplace_back(std::make_shared<CmdType>(std::forward<CmdArgTypes>(args)...));
  }

  std::vector<std::shared_ptr<command::Command>> GetAndClearCommandQueue() {
    std::lock_guard lock(m_commandQueueMutex);
    auto command_queue = m_commandQueue;
    m_commandQueue.clear();
    return command_queue;
  }

 private:
  std::vector<std::shared_ptr<command::Command>> m_commandQueue;
  std::mutex m_commandQueueMutex;
};

/// @brief The Engine class is responsible for executing the steps in the lifetime of the application.
///
/// The Engine manages the registry of modules and systems loaded by its parent Application, as well as the registry of
/// entities in the current scene.
class Engine : public EngineControl, public properties::PropertyOwner {
 public:
  constexpr static const char* NAME = "Engine";
  std::string Name() const override { return NAME; }

  /// @brief Constructor
  Engine();

  /// @brief Destructor
  ~Engine();

  /// @brief Create a CUDA context to be used by all systems
  ///
  /// If the engine already has an active CUDA context, this function is a no-op. If not, the current CUDA context will
  /// be lost.
  /// @return SUCCESS If CUDA context was successfully created
  Error InitializeCudaDevice();

  /// @brief Initialize all the modules registered for the engine
  /// @return SUCCESS If all modules were successfully initialized
  Error InitializeModules();

  /// @brief Initialize all the systems registered for the engine
  /// @return SUCCESS If all systems were successfully initialized
  Error InitializeSystems();

  /// @brief Uninitialize all systems, then all modules registered for the engine, then destroy the CUDA context,
  /// then empty the scene
  /// @return SUCCESS If all systems and modules were successfully uninitialized
  Error Uninitialize();

  /// @brief Calls the OnLoadScene method for all modules and then all systems registered for the engine
  ///
  /// This should be called after a scene has been loaded
  /// @return SUCCESS if all modules and systems returned SUCCESS from OnLoadScene
  Error OnLoadScene();

  /// @brief Calls the OnUnloadScene method for all modules and then all systems registered for the engine
  ///
  /// This should be called before the scene has been unloaded
  /// @return SUCCESS if all modules and systems returned SUCCESS from OnUnloadScene
  Error OnUnloadScene();

  /// @brief Callback to broadcast an event to all the systems registered in the engine
  /// @param[in,out] e The event to broadcast
  Error EventCallback(events::Event* e);

  /// @brief Queue an event for broadcasting to all the systems registered in the engine
  /// @tparam EventType  Type of event to queue
  /// @tparam ArgTypes   Types of `EventType` constructor arguments
  /// @param args        `EventType` constructor arguments
  template <class EventType, typename... ArgTypes>
  void QueueEvent(ArgTypes&&... args);

  /// @brief Broadcast all queued events to all registered systems, and clear the event queue
  Error BroadcastQueuedEvents();

  /// @brief Create an entity
  /// @return A valid entity added to the registry
  ///         An invalid Entity if failed
  ecs::Entity CreateEntity();

  /// @brief Create an entity and give it a name
  ///
  /// The name will be stored as a component of type std::string. The name can be used as identifier and should be
  /// unique to this entity
  /// @param[in] name The name of the entity
  /// @return    A valid entity added to the registry
  ///            An invalid Entity if failed
  ecs::Entity CreateEntity(const std::string& name);

  /// @brief Destroy an entity and all its attached components
  ///
  /// @param entity The entity to destroy
  /// @return Error::SUCCESS if successful
  Error DestroyEntity(ecs::Entity entity);

  /// @brief Create a new scene defined as a registry of entities
  ///
  /// This function is not thread safe, as systems running on different threads may be accessing the entity registry.
  /// This function needs to run on the main thread, and should only be called if none of the threaded systems are
  /// currently running. See System::IsRunning()
  /// @return SUCCESS if successful
  Error CreateNewScene();

  /// @brief Load the scene from the current scene config file path
  ///
  /// If the current scene file path is not set, and empty scene will be created
  /// @return SUCCESS if successful
  Error LoadCurrentScene();

  /// @brief Set the scene config file path
  ///
  /// @param[in] file_path The scene config file path to use for loading and saving the scene
  /// @return SUCCESS
  Error SetSceneConfigFilePath(const std::string& file_path);

  /// @brief Set the additional scene config JSON to load after the scene config file
  ///
  /// Any addition scene entities, components or properties in the JSON string will be merged into the scene config
  /// before being deserialized.
  /// @param[in] json_string The scene config JSON string to merge into the scene config
  /// @return SUCCESS
  Error SetAdditionalSceneConfigJsonString(const std::string& json_string);

  /// @brief Run single system
  /// @param[in] system A shared pointer to the system to run
  /// @param[in] dt     Deltatime [seconds]. Time spent since last update
  /// @return SUCCESS if system ran successfully
  Error RunSystem(std::shared_ptr<ecs::System> system, float dt);

  /// @brief Run a pipeline of systems
  /// @param[in] systems A vector of shared pointers to systems to run. Systems will run serially
  /// @param[in] dt      Deltatime [seconds]. Time spent since last update
  /// @return SUCCESS if all systems ran successfully
  Error RunSystems(std::vector<std::shared_ptr<ecs::System>> systems, float dt);

  /// @brief Update all modules registered for the engine
  /// @param[in] dt Deltatime [seconds]. Time spent since last update
  /// @return SUCCESS if all modules updated successfully
  Error UpdateModules(float dt);

  /// @brief Get a pointer to a system that has been registered to the engine
  template <typename SystemType>
  std::shared_ptr<SystemType> GetSystem() const;

  /// @brief Get a reference to the container of all modules that have been registered to the engine
  /// @return A vector of modules
  const std::vector<std::unique_ptr<Module>>& GetModules();

  /// @brief Get the Engine's input handler
  /// @return A reference to the Engine's application::inputs::Input object
  application::inputs::Input& Input();

  /// @brief Get the CUDA context
  /// @return A valid CUDA context If successful
  ///         nullptr              Otherwise, or if uninitialized
  CUcontext GetCudaContext() const;

  /// @brief Get the time stamp since the start up of the engine
  /// @return time stamp in microseconds
  int64_t GetTimeStamp() const;

  /// @brief Get user prompts
  /// @return A set of prompts to the user
  std::set<std::string> GetUserPrompts() const;

  /// @brief Register a module with all its defined components
  /// This allows the Module's components to be added to any entity created by the Engine
  /// @tparam    ModuleType The Module subclass to register
  /// @tparam    ArgTypes   Argument types for module creation
  /// @param[in] args,      Input argument for Module subclass constructor
  /// @return    a reference to the created module. Valid until the module is unregistered
  template <class ModuleType, typename... ArgTypes>
  ModuleType& RegisterModule(ArgTypes&&... args);

  /// @brief Remove a Module from the registry
  /// @tparam ModuleType The type of Module to unregister
  template <class ModuleType>
  Error UnregisterModule();

  /// @brief Get a reference to the Engine's command queue
  /// @return A reference to the Engine's EngineControl object
  EngineControl& Control();

  /// @brief Test whether the CLOSE command has been executed
  /// @return true if the CLOSE command has been executed
  /// @return false if the CLOSE command has not been executed
  bool ShouldClose();

  /// @brief Load application configuration file to initialize property values for all registered modules and systems.
  ///
  /// @param[in] file_path  The path to the configuration file
  /// @return    SUCCESS    If the application config was loaded and deserialized successfully
  Error LoadAppConfig(const std::string& file_path);

  /// @brief Set the function that constructs the application's default scene
  ///
  /// @param procedure a function to construct the default scene
  void SetBuildDefaultSceneProcedure(const std::function<Error(void)>& procedure);

  /// @brief See core::ecs::registry::SystemRegistry::OrderToFront
  Error OrderToFront(ecs::System* system);

  /// @brief Associate an entity with a time stamp accessible by the engine
  /// @param[in] entity    The id of the entity to associate the timestamp with
  /// @param[in] timestamp The timestamp
  void SetTimeStampForEntity(uint32_t entity, int64_t timestamp);

  /// @brief Get the associated timestamp of an entity, previously set with SetTimeStampForEntity
  /// @param[in] entity The id of the entity to associate the timestamp with
  /// @return    The timestamp if it exists
  ///            -1 otherwise
  int64_t GetTimeStampForEntity(uint32_t entity) const;

  /// @brief Get the AFX SDK directory path
  ///
  /// The path is determined based on the environment variable `AFXSDK`, which should be set before running the engine.
  /// See README.md for details on setting up environment variables.
  /// @return The path to the root of the AFX SDK
  static std::string GetAfxSdkDir();

  /// @brief Get the AR SDK directory path
  ///
  /// The path is determined based on the environment variable `ARSDK`, which should be set before running the engine.
  /// See README.md for details on setting up environment variables.
  /// @return The path to the root of the AR SDK
  static std::string GetArSdkDir();

  /// @brief Get the VFX SDK directory path
  ///
  /// The path is determined based on the environment variable `VFXSDK`, which should be set before running the engine.
  /// See README.md for details on setting up environment variables.
  /// @return The path to the root of the VFX SDK
  static std::string GetVfxSdkDir();

  /// @brief Get the resources directory path
  ///
  /// The path is determined based on the environment variable `NV3DVC_RESOURCES_DIR`, which should be set before
  /// running the engine. See README.md for details on setting up environment variables.
  /// @return The path to the resources directory
  static std::string GetResourcesDir();

 public:
  core::properties::Property<int> log_level = {
      this,
      "log_level",
      nullptr,
      1,
  };
  core::properties::Property<std::atomic<bool>> exit_at_eof = {
      this,
      "exit_at_eof",
      nullptr,
      false,
  };

 private:
  Error ExecuteCommand(const command::Command& command);
  Error SaveAppConfig(const std::string& file_path);
  Error LoadScene(const std::string& file_path, const std::string& additional_scene_config_json = "");
  Error SaveScene(const std::string& file_path);

  /// @brief Clear user prompts every second
  /// @param dt Delta time [seconds]
  void MaybeClearUserPrompts(float dt);

  std::string m_appConfigFilePath = "app_config.json";
  std::string m_sceneConfigFilePath = "scene.json";
  std::string m_additionalSceneConfigJsonString = "";

  std::atomic<bool> m_shouldClose = {false};
  CUdevice m_cuDevice = -1;
  int m_cuDeviceIndex = 0;
  CUcontext m_cuContext = nullptr;

  util::TypeMap<std::shared_ptr<ecs::System>> m_systemsTypeMap;

  ecs::registry::EntityRegistry m_entityRegistry;
  ecs::registry::SystemRegistry m_systemRegistry;
  std::vector<std::unique_ptr<Module>> m_modules;

  graphics::GraphicsApi m_graphicsApi;
  application::inputs::Input m_input;
  std::chrono::system_clock::duration m_startTimestamp = {};
  std::vector<std::unique_ptr<events::Event>> m_eventQueue;
  std::function<Error(void)> m_buildDefaultSceneProcedure;
  std::set<std::string> m_userPrompts;
  std::unordered_map<uint32_t, int64_t> m_entityTimestampMap;  /// < entity id -> timestamp map

  float m_userPromptClearIntervalSeconds = 1.0f;
  float m_userPromptClearTimer = 0.0f;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Template function definitions                                                                                      //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <class EventType, typename... ArgTypes>
void Engine::QueueEvent(ArgTypes&&... args) {
  m_eventQueue.emplace_back(std::make_unique<EventType>(std::forward<ArgTypes>(args)...));
}

template <typename SystemType>
std::shared_ptr<SystemType> Engine::GetSystem() const {
  const auto search = m_systemsTypeMap.find<SystemType>();
  return search == m_systemsTypeMap.end() ? nullptr : std::static_pointer_cast<SystemType>(search->second);
}

template <class ModuleType, typename... ArgTypes>
ModuleType& Engine::RegisterModule(ArgTypes&&... args) {
  for (auto& module : m_modules) {
    if (dynamic_cast<ModuleType*>(module.get())) {
      throw std::runtime_error("Module already registered");
    }
  }
  m_modules.push_back(std::make_unique<ModuleType>(std::forward<ArgTypes>(args)...));
  AddSubOwner(m_modules.back().get());

  // Add all systems to type map so that we can get them using GetSystem
  m_systemsTypeMap.Merge(m_modules.back()->SystemTypeMap());

  // Register all systems
  for (auto& [_, system] : m_modules.back()->SystemTypeMap()) {
    m_systemRegistry.RegisterSystem(system.get());
    m_modules.back()->AddSubOwner(system.get());
  }
  return *static_cast<ModuleType*>(m_modules.back().get());
}

template <class ModuleType>
Error Engine::UnregisterModule() {
  Error err = Error::SUCCESS;
  int module_idx = -1;
  for (int i = 0; i < m_modules.size(); i++) {
    if (dynamic_cast<ModuleType*>(m_modules[i].get())) {
      module_idx = i;
      break;
    }
  }
  if (module_idx == -1) {
    BAIL(err, Error::ERR_ITEM_NOT_FOUND);
  }
  CHECK_SUCCESS(m_modules[module_idx]->Uninitialize());
  m_modules.erase(m_modules.begin() + module_idx);
bail:
  return err;
}

}  // namespace engine
}  // namespace core
}  // namespace nv3dvc

#endif  // SRC_CORE_ENGINE_ENGINE_H_
