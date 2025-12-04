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

#include "Engine.h"

#include <chrono>  // NOLINT(build/c++11) (cpplint outside Chromium: https://github.com/google/styleguide/issues/483)
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <set>
#include <string>
#include <thread>  // NOLINT(build/c++11) (cpplint outside Chromium: https://github.com/google/styleguide/issues/483)
#include <vector>

#include "Core/Application/Inputs.h"
#include "Core/EntityComponentSystem/Entity.h"
#include "Core/EntityComponentSystem/System.h"
#include "Core/Error.h"
#include "Core/Serialization/PropertyDescription.h"
#include "Core/Serialization/Serialization.h"
#include "Core/Util/Logger.h"
#include "Module.h"

#define UNKNOWN_DIR ""

/// @brief Assure that the string ends in a directory separator character.
/// @param[in,out]  str the string to be tested and possibly modified.
static void AssureTerminalDirectorySeparator(std::string* str) {
  if (!str->empty() && ('/' == str->back()
#ifdef _WIN32
                        || '\\' == str->back()
#endif  // _WIN32
                            ))
    return;
  (*str) += '/';
}

namespace nv3dvc {
namespace core {
namespace engine {

Engine::Engine() { nv3dvc::core::util::gLogger.init(log_level, "stderr", nullptr, nullptr); }

Engine::~Engine() {}

nv3dvc::core::Error Engine::InitializeCudaDevice() {
  core::Error err = core::Error::SUCCESS;

  // Create CUDA context
  if (!m_cuContext) {
    // Initialize CUDA
    constexpr unsigned int flags = 0;
    LOG_DEBUG("Initializing CUDA with flags %u", flags);
    CHECK_CU_SUCCESS(cuInit(flags));

    // Select the first CUDA-capable device and get a handle to it
    int num_cuda_devices = 0;
    CHECK_CU_SUCCESS(cuDeviceGetCount(&num_cuda_devices));
    CHECK_TRUE(num_cuda_devices > 0, core::Error::ERR_NOT_SUPPORTED);
    LOG_DEBUG("Detected %d CUDA Capable device(s)", num_cuda_devices);
    int selected_cuda_device_index = 0;  // Select the first one
    CHECK_CU_SUCCESS(
        cuDeviceGet(&m_cuDevice, selected_cuda_device_index),  // Get device handle for selected device index
        "Failed to initialize CUDA device %d", selected_cuda_device_index);
    m_cuDeviceIndex = selected_cuda_device_index;  // Save selected device index

    // Create CUDA context for that device
    constexpr unsigned int kCtxCreationFlags = 0;
    LOG_DEBUG("Initializing context on selected CUDA device %d with flags %u", selected_cuda_device_index,
              kCtxCreationFlags);
    CHECK_CU_SUCCESS(cuCtxCreate(&m_cuContext, kCtxCreationFlags, m_cuDevice));

    LOG_DEBUG("CUDA context successfully created on device %d", m_cuDeviceIndex);
  }

bail:
  return err;
}

nv3dvc::core::Error Engine::InitializeModules() {
  // Give all modules a chance to initialize, but return the error code from the first one
  nv3dvc::core::Error err = nv3dvc::core::SUCCESS;
  for (auto& module : m_modules) {
    const core::Error e = module->Initialize();
    if (e != core::SUCCESS) {
      if (nv3dvc::core::SUCCESS == err) err = e;  // Preserve the first error
      LOG_DEBUG("Module %s failed to initialize: %s", module->Name().c_str(), nv3dvc::core::ErrorStringFromCode(e));
    }
  }
  return err;
}

nv3dvc::core::Error Engine::InitializeSystems() {
  m_graphicsApi.Initialize();
  // Give all systems a chance to initialize, but return the error code from the first one
  nv3dvc::core::Error err = nv3dvc::core::Error::SUCCESS;
  for (auto& system : m_systemRegistry.GetSystems()) {
    const core::Error e = system->Initialize();
    if (e != core::SUCCESS) {
      if (nv3dvc::core::SUCCESS == err) err = e;  // Preserve the first error
      LOG_DEBUG("System %s failed to initialize: %s", system->Name().c_str(), nv3dvc::core::ErrorStringFromCode(e));
    }
  }
  return err;
}

nv3dvc::core::Error Engine::Uninitialize() {
  core::Error err = core::Error::SUCCESS;
  m_entityRegistry.Clear();
  for (auto& system : m_systemRegistry.GetSystems()) {
    core::Error e = system->Uninitialize();
    if (nv3dvc::core::SUCCESS == err) err = e;  // Preserve the first error
  }
  for (auto& module : m_modules) {
    core::Error e = module->Uninitialize();
    if (nv3dvc::core::SUCCESS == err) err = e;  // Preserve the first error
  }
  if (m_cuContext) {
    cuCtxDestroy(m_cuContext);
    m_cuContext = nullptr;
  }
  return err;
}

nv3dvc::core::Error Engine::OnLoadScene() {
  nv3dvc::core::Error err = nv3dvc::core::SUCCESS;
  for (auto& module : m_modules) {
    const core::Error e = module->OnLoadScene(&m_entityRegistry);
    if (e != core::SUCCESS) {
      if (nv3dvc::core::SUCCESS == err) err = e;  // Preserve the first error
      LOG_ERROR("Module %s failed to start: %s", module->Name().c_str(), nv3dvc::core::ErrorStringFromCode(e));
    }
  }
  for (auto& system : m_systemRegistry.GetSystems()) {
    const core::Error e = system->OnLoadScene(&m_entityRegistry);
    if (e != core::SUCCESS) {
      if (nv3dvc::core::SUCCESS == err) err = e;  // Preserve the first error
      LOG_ERROR("System %s failed to start: %s", system->Name().c_str(), nv3dvc::core::ErrorStringFromCode(e));
    }
  }
  m_startTimestamp = std::chrono::system_clock::now().time_since_epoch();

  return err;
}

nv3dvc::core::Error Engine::OnUnloadScene() {
  nv3dvc::core::Error err = nv3dvc::core::SUCCESS;
  for (auto& system : m_systemRegistry.GetSystems()) {
    nv3dvc::core::Error e = system->OnUnloadScene(&m_entityRegistry);
    if (e != core::SUCCESS) {
      if (nv3dvc::core::SUCCESS == err) err = e;  // Preserve the first error
      LOG_ERROR("System %s failed to shut down: %s", system->Name().c_str(), nv3dvc::core::ErrorStringFromCode(e));
    }
  }
  for (auto& module : m_modules) {
    const core::Error e = module->OnUnloadScene(&m_entityRegistry);
    if (e != core::SUCCESS) {
      if (nv3dvc::core::SUCCESS == err) err = e;  // Preserve the first error
      LOG_ERROR("Module %s failed to shut down: %s", module->Name().c_str(), nv3dvc::core::ErrorStringFromCode(e));
    }
  }
  return err;
}

nv3dvc::core::Error Engine::EventCallback(events::Event* evt) {
  nv3dvc::core::Error err = nv3dvc::core::SUCCESS;
  for (auto& system : m_systemRegistry.GetSystems()) {
    if (evt->IsConsumed()) {
      break;
    }
    nv3dvc::core::Error e = system->OnEvent(&m_entityRegistry, evt);
    if (nv3dvc::core::SUCCESS == err) err = e;  // Preserve the first error
  }
  return err;
}

nv3dvc::core::Error Engine::BroadcastQueuedEvents() {
  nv3dvc::core::Error err = nv3dvc::core::SUCCESS;
  for (auto& event : m_eventQueue) {
    nv3dvc::core::Error e = EventCallback(event.get());
    if (nv3dvc::core::SUCCESS == err) err = e;  // Preserve the first error
  }
  m_eventQueue.clear();
  return err;
}

ecs::Entity Engine::CreateEntity() { return m_entityRegistry.CreateEntity(); }

ecs::Entity Engine::CreateEntity(const std::string& name) { return m_entityRegistry.CreateEntity(name); }

nv3dvc::core::Error Engine::DestroyEntity(ecs::Entity entity) {
  Error err = Error::SUCCESS;
  CHECK_TRUE(entity.IsValid(), Error::ERR_SCENE);
  m_entityRegistry.RemoveEntity(&entity);
bail:
  return err;
}

nv3dvc::core::Error Engine::CreateNewScene() {
  m_entityRegistry.Clear();
  return nv3dvc::core::SUCCESS;
}

nv3dvc::core::Error Engine::LoadCurrentScene() {
  if (m_sceneConfigFilePath.empty()) {
    return CreateNewScene();
  } else {
    return LoadScene(m_sceneConfigFilePath, m_additionalSceneConfigJsonString);
  }
}

nv3dvc::core::Error Engine::SetSceneConfigFilePath(const std::string& file_path) {
  m_sceneConfigFilePath = file_path;
  return core::Error::SUCCESS;
}

nv3dvc::core::Error Engine::SetAdditionalSceneConfigJsonString(const std::string& json_string) {
  m_additionalSceneConfigJsonString = json_string;
  return core::Error::SUCCESS;
}

nv3dvc::core::Error Engine::RunSystem(std::shared_ptr<ecs::System> system, const float dt) {
  nv3dvc::core::Error err = nv3dvc::core::SUCCESS;
  if (!system->IsPaused()) {
    system->SetIsRunning(true);
    err = system->Run(&m_entityRegistry, dt);
    if (err == Error::ERR_EOF) {
      if (exit_at_eof.get()->load()) Control().Queue<core::engine::command::Close>();
      err = core::Error::SUCCESS;  // EOF is ok
    }
    if (err != core::SUCCESS) {
      LOG_DEBUG("System %s failed to run: %s", system->Name().c_str(), nv3dvc::core::ErrorStringFromCode(err));
    }
  } else {
    system->SetIsRunning(false);
  }
  return err;
}

nv3dvc::core::Error Engine::RunSystems(std::vector<std::shared_ptr<ecs::System>> systems, const float dt) {
  nv3dvc::core::Error err = nv3dvc::core::SUCCESS;
  for (const auto& system : systems) {
    const core::Error e = RunSystem(system, dt);
    if (e != core::SUCCESS) {
      if (nv3dvc::core::SUCCESS == err) err = e;  // Preserve the first error
    }
  }
  return err;
}

nv3dvc::core::Error Engine::UpdateModules(float dt) {
  nv3dvc::core::Error err = nv3dvc::core::SUCCESS;
  for (auto& module : m_modules) {
    const core::Error e = module->Update(dt);
    if (e != core::SUCCESS) {
      if (nv3dvc::core::SUCCESS == err) err = e;  // Preserve the first error
      LOG_DEBUG("Module %s failed to update: %s", module->Name().c_str(), nv3dvc::core::ErrorStringFromCode(e));
    }
  }
  MaybeClearUserPrompts(dt);
  auto command_queue = GetAndClearCommandQueue();
  for (auto& command : command_queue) {
    const core::Error e = ExecuteCommand(*command);
    if (e != core::SUCCESS) {
      if (nv3dvc::core::SUCCESS == err) err = e;  // Preserve the first error
      LOG_DEBUG("Command %d failed to execute: %s", command->Type(), nv3dvc::core::ErrorStringFromCode(e));
    }
  }
  return err;
}

application::inputs::Input& Engine::Input() { return m_input; }

int64_t Engine::GetTimeStamp() const {
  int64_t microseconds_timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
                                       std::chrono::system_clock::now().time_since_epoch() - m_startTimestamp)
                                       .count();
  return microseconds_timestamp;
}

std::set<std::string> Engine::GetUserPrompts() const { return m_userPrompts; }

const std::vector<std::unique_ptr<Module>>& Engine::GetModules() { return m_modules; }

CUcontext Engine::GetCudaContext() const { return m_cuContext; }

nv3dvc::core::Error Engine::LoadAppConfig(const std::string& file_path) {
  m_appConfigFilePath = file_path;
  nv3dvc::core::Error err = nv3dvc::core::SUCCESS;
  serialization::PropertyDescription serialized_description = serialization::LoadDescription(file_path, &err);
  CHECK_SUCCESS(err, "Failed to load app config from \"%s\"", file_path.c_str());
  CHECK_SUCCESS(serialization::DeserializeRegistry(serialized_description, this, m_modules),
                "Failed to deserialize app config from \"%s\"", file_path.c_str());
  nv3dvc::core::util::gLogger.init(log_level, "stderr", nullptr, nullptr);
bail:
  if (err != Error::SUCCESS) {
    LOG_ERROR("The engine will proceed using default app config");
  }
  return err;
}

void Engine::SetBuildDefaultSceneProcedure(const std::function<Error(void)>& procedure) {
  m_buildDefaultSceneProcedure = procedure;
}

Error Engine::OrderToFront(ecs::System* system) { return m_systemRegistry.OrderToFront(system); }

void Engine::SetTimeStampForEntity(uint32_t entity, int64_t timestamp) { m_entityTimestampMap[entity] = timestamp; }

int64_t Engine::GetTimeStampForEntity(uint32_t entity) const {
  auto found = m_entityTimestampMap.find(entity);
  if (found != m_entityTimestampMap.end()) return found->second;
  return -1;
}

nv3dvc::core::Error Engine::LoadScene(const std::string& file_path, const std::string& additional_scene_config_json) {
  nv3dvc::core::Error err = nv3dvc::core::SUCCESS;
  serialization::PropertyDescription additional_properties;
  serialization::PropertyDescription serialized_scene = serialization::LoadDescription(file_path, &err);
  CHECK_SUCCESS(err, "Failed to load scene from \"%s\"", file_path.c_str());
  if (!additional_scene_config_json.empty()) {
    additional_properties = serialization::LoadDescriptionString(additional_scene_config_json, &err);
    CHECK_SUCCESS(err, "Failed to parse json: \"%s\"", additional_scene_config_json.c_str());
    serialized_scene.Update(additional_properties);
  }
  CHECK_SUCCESS(err, "Failed to load scene from \"%s\"", file_path.c_str());
  CHECK_SUCCESS(serialization::DeserializeRegistry(serialized_scene, &m_entityRegistry, m_modules),
                "Failed to deserialize scene from \"%s\"", file_path.c_str());
  m_sceneConfigFilePath = file_path;
bail:
  return err;
}

std::string Engine::GetAfxSdkDir() {
  const char* dir = std::getenv("AFXSDK");
  if (!dir) {
#ifdef NV3DVC_AFX_SDK_DIR
    dir = NV3DVC_AFX_SDK_DIR;
#else
    dir = UNKNOWN_DIR;
#endif
  }
  std::string dir_str = std::string(dir);
  std::string full_path;
  try {
    full_path = std::filesystem::absolute(dir_str).string();
    if (!std::filesystem::exists(full_path)) {
      LOG_ERROR(
          "The AFX SDK directory was not found. Make sure the environment variable AFXSDK is set to the root "
          "of the directory for the AFX SDK. Please see README.md for more information on installing the AFX SDK.");
    }
  } catch (const std::filesystem::filesystem_error& e) {
    LOG_ERROR("Filesystem error: %s.", e.what());
  }
  AssureTerminalDirectorySeparator(&full_path);
  return full_path;
}

std::string Engine::GetArSdkDir() {
  const char* dir = std::getenv("ARSDK");
  if (!dir) {
#ifdef NV3DVC_AR_SDK_DIR
    dir = NV3DVC_AR_SDK_DIR;
#else
    dir = UNKNOWN_DIR;
#endif
  }
  std::string dir_str = std::string(dir);
  std::string full_path;
  try {
    full_path = std::filesystem::absolute(dir_str).string();
    if (!std::filesystem::exists(full_path)) {
      LOG_ERROR(
          "The AR SDK directory was not found. Make sure the environment variable ARSDK is set to the root "
          "of the directory for the AR SDK. Please see README.md for more information on installing the AR SDK.");
    }
  } catch (const std::filesystem::filesystem_error& e) {
    LOG_ERROR("Filesystem error: %s.", e.what());
  }
  AssureTerminalDirectorySeparator(&full_path);
  return full_path;
}

std::string Engine::GetVfxSdkDir() {
  const char* dir = std::getenv("VFXSDK");
  if (!dir) {
#ifdef NV3DVC_VFX_SDK_DIR
    dir = NV3DVC_VFX_SDK_DIR;
#else
    dir = UNKNOWN_DIR;
#endif
  }
  std::string dir_str = std::string(dir);
  std::string full_path;
  try {
    full_path = std::filesystem::absolute(dir_str).string();
    if (!std::filesystem::exists(full_path)) {
      LOG_ERROR(
          "The AR SDK directory was not found. Make sure the environment variable VFXSDK is set to the root "
          "of the directory for the VFX SDK. Please see README.md for more information on installing the VFX SDK.");
    }
  } catch (const std::filesystem::filesystem_error& e) {
    LOG_ERROR("Filesystem error: %s.", e.what());
  }
  AssureTerminalDirectorySeparator(&full_path);
  return full_path;
}

std::string Engine::GetResourcesDir() {
  const char* dir = std::getenv("NV3DVC_RESOURCES_DIR");
  if (!dir) {
#ifdef NV3DVC_RESOURCES_DIR
    dir = NV3DVC_RESOURCES_DIR;
#else
    dir = UNKNOWN_DIR;
#endif
  }
  std::string dir_str = std::string(dir);
  std::string full_path;
  try {
    full_path = std::filesystem::absolute(dir_str).string();
    if (!std::filesystem::exists(full_path)) {
      // Check current directory / nv3dvc_resources
      dir_str = "nv3dvc_resources/";
      while (!std::filesystem::exists(dir_str) && dir_str.size() < 256) {
        dir_str = "../" + dir_str;
      }
      full_path = std::filesystem::absolute(dir_str).string();
      if (!std::filesystem::exists(full_path)) {
        LOG_ERROR(
            "Resources directory was not found. Make sure the environment variable NV3DVC_RESOURCES_DIR is set to the "
            "root of the resources folder. Please see README.md for more information on getting resources.");
      }
    }
  } catch (const std::filesystem::filesystem_error& e) {
    LOG_ERROR("Filesystem error: %s.", e.what());
  }
  AssureTerminalDirectorySeparator(&full_path);
  return full_path;
}

nv3dvc::core::Error Engine::SaveAppConfig(const std::string& file_path) {
  nv3dvc::core::Error err = nv3dvc::core::SUCCESS;
  serialization::PropertyDescription serialized_registry;

  serialized_registry = serialization::SerializeRegistry(*this, m_modules, &err);
  CHECK_SUCCESS(err, "Failed to serialize app config");
  CHECK_SUCCESS(serialization::WriteDescription(serialized_registry, file_path), "Failed to write app config to \"%s\"",
                file_path.c_str());
bail:
  return err;
}

nv3dvc::core::Error Engine::SaveScene(const std::string& file_path) {
  nv3dvc::core::Error err = nv3dvc::core::SUCCESS;
  serialization::PropertyDescription serialized_entity_registry;
  serialized_entity_registry = serialization::SerializeRegistry(&m_entityRegistry, m_modules, &err);
  CHECK_SUCCESS(err, "Failed to serialize scene");
  CHECK_SUCCESS(serialization::WriteDescription(serialized_entity_registry, file_path),
                "Failed to write scene to \"%s\"", file_path.c_str());
bail:
  return err;
}

void Engine::MaybeClearUserPrompts(const float dt) {
  m_userPromptClearTimer += dt;
  if (m_userPromptClearTimer >= m_userPromptClearIntervalSeconds) {
    m_userPromptClearTimer = 0.0f;
    m_userPrompts.clear();
  }
}

EngineControl& Engine::Control() { return *this; }

bool Engine::ShouldClose() { return m_shouldClose.load(); }

nv3dvc::core::Error Engine::ExecuteCommand(const command::Command& command) {
  Error err = Error::SUCCESS;
  // Record all pause states
  std::vector<bool> system_pause_states;
  const auto systems = m_systemRegistry.GetSystems();
  system_pause_states.reserve(systems.size());
  for (auto& system : systems) {
    system_pause_states.push_back(system->IsPaused());
  }
  // Pause all systems
  for (auto& system : systems) {
    system->SetPaused(true);
  }
  switch (command.Type()) {
    case command::CommandType::CLOSE: {
      m_shouldClose.store(true);
      break;
    }
    case command::CommandType::EMPTY_SCENE: {
      // Make sure no systems are running
      for (auto& system : systems) {
        // If the system is running on the main thread, i.e. this thread, we already know it has finished iterating
        while (!system->IsMainThreadSystem() && system->IsRunning()) {
          std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        CHECK_CU_SUCCESS(cuStreamSynchronize(system->GetStream()));
      }

      CHECK_SUCCESS(OnUnloadScene());

      // Create a new entity registry, destroying the current scene
      CHECK_SUCCESS(CreateNewScene());

      CHECK_SUCCESS(OnLoadScene());

      m_appConfigFilePath = "";
      break;
    }
    case command::CommandType::LOAD_DEFAULT_SCENE: {
      // Make sure no systems are running
      for (auto& system : systems) {
        // If the system is running on the main thread, i.e. this thread, we already know it has finished iterating
        while (!system->IsMainThreadSystem() && system->IsRunning()) {
          std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        CHECK_CU_SUCCESS(cuStreamSynchronize(system->GetStream()));
      }

      CHECK_SUCCESS(OnUnloadScene());

      // Create a new entity registry, destroying the current scene
      CHECK_SUCCESS(CreateNewScene());

      if (m_buildDefaultSceneProcedure) {
        CHECK_SUCCESS(m_buildDefaultSceneProcedure());
      }

      CHECK_SUCCESS(OnLoadScene());

      break;
    }
    case command::CommandType::SAVE_APP_CONFIG: {
      const auto& save_command = command.As<command::SaveAppConfig>();
      std::string& file_path = m_appConfigFilePath;
      if (!save_command.file_path.empty()) {
        file_path = save_command.file_path;
      }
      CHECK_SUCCESS(SaveAppConfig(file_path));
      m_appConfigFilePath = file_path;
      break;
    }
    case command::CommandType::LOAD_SCENE: {
      const auto& load_command = command.As<command::LoadScene>();
      std::string& file_path = m_sceneConfigFilePath;
      if (!load_command.file_path.empty()) {
        file_path = load_command.file_path;
      }

      // Make sure no systems are running
      for (auto& system : systems) {
        // If the system is running on the main thread, i.e. this thread, we already know it has finished iterating
        while (!system->IsMainThreadSystem() && system->IsRunning()) {
          std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        CHECK_CU_SUCCESS(cuStreamSynchronize(system->GetStream()));
      }

      CHECK_SUCCESS(OnUnloadScene());

      // Create a new entity registry, destroying the current scene
      CHECK_SUCCESS(CreateNewScene());

      // Deserialize the entity registry
      serialization::PropertyDescription serialized_scene = serialization::LoadDescription(file_path, &err);
      CHECK_SUCCESS(err, "Failed to load scene from \"%s\"", file_path.c_str());
      CHECK_SUCCESS(DeserializeRegistry(serialized_scene, &m_entityRegistry, m_modules),
                    "Failed to deserialize scene from \"%s\"", file_path.c_str());

      CHECK_SUCCESS(OnLoadScene());

      m_sceneConfigFilePath = file_path;
      break;
    }
    case command::CommandType::SAVE_SCENE: {
      const auto& save_command = command.As<command::SaveScene>();
      std::string& file_path = m_sceneConfigFilePath;
      if (!save_command.file_path.empty()) {
        file_path = save_command.file_path;
      }
      CHECK_SUCCESS(SaveScene(file_path));
      m_sceneConfigFilePath = file_path;
      break;
    }
    case command::CommandType::PROMPT_USER: {
      const auto& prompt_command = command.As<command::PromptUser>();
      if (!prompt_command.prompt.empty()) {
        m_userPrompts.insert(prompt_command.prompt);
      }
      break;
    }
    default: {
      break;
    }
  }
bail:
  // Reset pause states
  for (size_t i = 0; i < systems.size(); i++) {
    systems[i]->SetPaused(system_pause_states[i]);
    systems[i]->SetIsRunning(true);
  }
  return err;
}

}  // namespace engine
}  // namespace core
}  // namespace nv3dvc
