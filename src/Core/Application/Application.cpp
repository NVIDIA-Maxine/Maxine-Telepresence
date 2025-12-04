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

#include "Application.h"

#include <string>

namespace nv3dvc {
namespace core {
namespace application {

Application::Application() { m_engine = std::make_unique<engine::Engine>(); }

Application::~Application() { m_engine = nullptr; }

Error Application::LoadAppConfig(const std::string& file_path) { return m_engine->LoadAppConfig(file_path); }

Error Application::SetSceneConfigFilePath(const std::string& file_path) {
  return m_engine->SetSceneConfigFilePath(file_path);
}

Error Application::SetAdditionalSceneConfigJsonString(const std::string& file_path) {
  return m_engine->SetAdditionalSceneConfigJsonString(file_path);
}

void Application::Run() {
  m_engine->SetBuildDefaultSceneProcedure([this]() { return BuildDefaultScene(); });

  m_engine->InitializeCudaDevice();

  cuCtxSetCurrent(m_engine->GetCudaContext());
  for (std::function<void()>& config_stream_function : m_configStreamFunctions) {
    config_stream_function();
  }

  m_engine->InitializeModules();

  m_engine->InitializeSystems();

  m_engine->LoadCurrentScene();

  m_engine->OnLoadScene();

  SpawnThreads();
  RunMainThread();

  // Join threads and remove all reference to systems
  for (std::thread& thread : m_threads) {
    thread.join();
  }
  for (auto& pipeline : m_threadPipelines) {
    for (auto& system : pipeline.systems) {
      system = nullptr;
    }
  }
  for (auto& system : m_mainPipeline.systems) {
    system = nullptr;
  }

  m_engine->OnUnloadScene();
  m_engine->Uninitialize();
}

engine::Engine& Application::GetEngine() { return *m_engine; }

void Application::SpawnThreads() {
  for (SystemPipeline& pipeline : m_threadPipelines) {
    for (auto& system : pipeline.systems) {
      system->SetPaused(false);
      system->SetIsRunning(true);
      system->SetIsMainThreadSystem(false);
    }

    m_threads.push_back(std::thread([&pipeline, this]() {
      cuCtxSetCurrent(m_engine->GetCudaContext());
      pipeline.time_since_last_update = std::chrono::system_clock::now().time_since_epoch();
      while (!GetEngine().ShouldClose()) {
        using ms = std::chrono::duration<float, std::milli>;
        auto time_now = std::chrono::system_clock::now().time_since_epoch();
        float dt = std::chrono::duration_cast<ms>(time_now - pipeline.time_since_last_update).count() / 1000.0f;
        pipeline.time_since_last_update = time_now;
        const core::Error err = m_engine->RunSystems(pipeline.systems, dt);
        if (err != Error::SUCCESS) {
          LOG_WARNING("Error Running systems: %s", ErrorStringFromCode(err));
        }
      }
    }));
  }
}

nv3dvc::core::Error Application::RunMainThread() {
  for (auto& system : m_mainPipeline.systems) {
    system->SetPaused(false);
    system->SetIsRunning(true);
    system->SetIsMainThreadSystem(true);
  }
  nv3dvc::core::Error err = nv3dvc::core::SUCCESS;
  m_mainPipeline.time_since_last_update = std::chrono::system_clock::now().time_since_epoch();
  float dt = 1.0f / 60.0f;
  while (!GetEngine().ShouldClose()) {
    err = m_engine->BroadcastQueuedEvents();
    if (err != Error::SUCCESS) {
      LOG_WARNING("Error broadcasting events: %s", ErrorStringFromCode(err));
    }
    err = m_engine->RunSystems(m_mainPipeline.systems, dt);
    if (err != Error::SUCCESS) {
      LOG_WARNING("Error Running systems: %s", ErrorStringFromCode(err));
    }
    err = m_engine->UpdateModules(dt);
    if (err != Error::SUCCESS) {
      LOG_WARNING("Error Updating modules: %s", ErrorStringFromCode(err));
    }
    using ms = std::chrono::duration<float, std::milli>;
    auto time_now = std::chrono::system_clock::now().time_since_epoch();
    dt = std::chrono::duration_cast<ms>(time_now - m_mainPipeline.time_since_last_update).count() / 1000.0f;
    m_mainPipeline.time_since_last_update = time_now;
  }
bail:
  m_engine->Control().Queue<core::engine::command::Close>();
  m_engine->UpdateModules(1.0f / 60.0f);
  return err;
}

}  // namespace application
}  // namespace core
}  // namespace nv3dvc
