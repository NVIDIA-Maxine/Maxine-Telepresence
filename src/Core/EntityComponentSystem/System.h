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

#ifndef SRC_CORE_ENTITYCOMPONENTSYSTEM_SYSTEM_H_
#define SRC_CORE_ENTITYCOMPONENTSYSTEM_SYSTEM_H_

#include <vector>

#include "Core/Error.h"
#include "Core/Properties/Property.h"
#include "Core/Util/Logger.h"
#include "driver_types.h"

namespace nv3dvc {
namespace core {

namespace events {
class Event;
}

namespace ecs {

namespace registry {
// Forward declaration
class EntityRegistry;
}  // namespace registry

/// @brief Extend the system class for acting on components in the entity registry
///
/// Sample extension of system:
/// \snippet src/Samples/MyModule/MyModule.h Simple system sample
class System : public properties::PropertyOwner {
 public:
  /// @brief Propertied should be added in the constructor
  System() : m_pauseRequested(true), m_isRunning({false}), m_isMainThreadSystem({true}) {}

  ~System() override = default;

  /// @brief Initialize or load any internal data dependent on the configuration of the system.
  ///
  /// This function will get called after the application config is loaded, meaning all properties have their right
  /// values.
  /// @return True if successful
  virtual nv3dvc::core::Error Initialize() { return nv3dvc::core::SUCCESS; }

  /// @brief Uninitialize or unload any previously loaded internal data
  /// @return True if successful
  virtual nv3dvc::core::Error Uninitialize() { return nv3dvc::core::SUCCESS; }

  /// @brief Run the system given a registry of entities.
  ///
  /// The run function is expected to run in an execution loop
  /// @param[in,out] reg The registry of entities existing in the scene
  /// @param[in]     dt  The delta time since the end of the last update on this thread
  virtual nv3dvc::core::Error Run(registry::EntityRegistry* reg, float dt) = 0;

  /// @brief Event callback called every time an event occurs.
  /// @param[in,out] reg The registry of entities existing in the scene
  /// @param[in,out] e   The event
  virtual nv3dvc::core::Error OnEvent(registry::EntityRegistry* reg, events::Event* e) { return nv3dvc::core::SUCCESS; }

  /// @brief Startup of the system called once per application run.
  ///
  /// Gets called after initialization, and after the scene has been loaded, but before Run
  /// @param[in,out] reg The registry of entities existing in the scene
  virtual nv3dvc::core::Error OnLoadScene(registry::EntityRegistry* reg) { return nv3dvc::core::SUCCESS; }

  /// @brief Shutdown of the system called once per application run.
  ///
  /// Gets called after the last execution of Run
  /// @param[in,out] reg The registry of entities existing in the scene
  virtual nv3dvc::core::Error OnUnloadScene(registry::EntityRegistry* reg) { return nv3dvc::core::SUCCESS; }

  /// @brief Set the system's CUDA stream.
  /// @param[in] stream The stream that should be used for CUDA commands in this system
  void SetStream(cudaStream_t stream) { m_stream = stream; }

  /// @brief Get the system's CUDA stream.
  /// @return The stream that is used for CUDA commands in this system
  cudaStream_t GetStream() const { return m_stream; }

  /// @brief Whether the system has been set to be paused.
  ///
  /// The system may still be running an iteration. To determine if the system is still running, use IsRunning
  /// @return Whether the system has been set to be paused
  bool IsPaused() const { return m_pauseRequested.load(); }

  /// @brief Whether the system should pause.
  ///
  /// After this function is called, the system may still be running an iteration. To determine if the system is still
  /// running, use IsRunning
  /// @param paused Whether the system should pause after its current iteration
  void SetPaused(const bool paused) { m_pauseRequested.store(paused); }

  /// @brief Whether the system has been set to run on the main thread using SetIsMainThreadSystem.
  /// @return Whether the system has been set to run on the main thread
  bool IsMainThreadSystem() const { return m_isMainThreadSystem.load(); }

  /// @brief Signal if this system will be running on the main thread.
  /// @param is_main_thread_system Whether the system will be running on the main thread
  void SetIsMainThreadSystem(const bool is_main_thread_system) { m_isMainThreadSystem.store(is_main_thread_system); }

  /// @brief The system is idle if it is both paused and not currently running.
  ///
  /// If the system is set to paused, it may still be finishing up the current iteration, where IsRunning will be true
  /// Typical usage when checking running systems from the main thread:
  /// @code
  /// // Make sure no systems are running
  /// for (auto& system : systems) {
  ///   // If the system is running on the main thread, i.e. this thread, we already know it has finished iterating.
  ///   // Blocking operation. Make sure all thread systems are set to paused using SetPaused(true) before this
  ///   while (!system->IsMainThreadSystem() && system->IsRunning()) {
  ///     using namespace std::chrono_literals;
  ///     std::this_thread::sleep_for(50ms);
  ///   }
  ///   cuStreamSynchronize(system->GetStream());
  /// }
  /// @endcode
  /// @return Whether the system is running
  bool IsRunning() const { return m_isRunning.load(); }

  /// @brief Signal that the system is paused and not running.
  /// @param running true if the system is set to be running, false otherwise
  void SetIsRunning(const bool running) { m_isRunning.store(running); }

 private:
  std::atomic<bool> m_pauseRequested;

  cudaStream_t m_stream = nullptr;
  std::atomic<bool> m_isRunning;
  std::atomic<bool> m_isMainThreadSystem;
};

namespace registry {

/// @brief A registry of systems
///
/// Associates systems with PropertyOwners
class SystemRegistry {
 public:
  SystemRegistry() = default;
  ~SystemRegistry() = default;

  /// @brief Register a System
  /// @param  system The System to add to the registry
  void RegisterSystem(System* system) {
    m_systems.push_back(system);
    m_propertyOwner.AddSubOwner(system);
  }

  /// @brief Get a PropertyOwner with references to all properties registered to all systems
  /// @return The PropertyOwner corresponding to the registered Systems
  properties::PropertyOwner& GetProperties() { return m_propertyOwner; }

  /// @brief Get a list of all registered Systems
  /// @return A list of all registered Systems
  const std::vector<System*>& GetSystems() { return m_systems; }

  /// @brief Internally orders the system to the front
  ///
  /// This will lead to this system being executed first when iterating over all systems. The engine may keep internal
  /// lists of systems for executing the Run function in order. This ordering only applies to generic system execution
  /// such as OnLoadScene, ShutDown and OnEvent
  /// @param[in] system A pointer to the system to order to the front
  /// @return    Error::ERR_GENERAL If the provided system does not exist in the registry
  ///            Error::SUCCESS     If successful
  Error OrderToFront(System* system) {
    Error err = Error::SUCCESS;
    auto pivot = std::find(m_systems.begin(), m_systems.end(), system);
    if (!system) return ERR_NULL_POINTER;  // Find still works when provided a NULL pointer
    CHECK_FALSE(pivot == m_systems.end(), ERR_ITEM_NOT_FOUND, "OrderToFront cannot find %s", system->Name().c_str());
    std::rotate(m_systems.begin(), pivot, pivot + 1);
  bail:
    return err;
  }

 private:
  properties::PropertyOwner m_propertyOwner;
  std::vector<System*> m_systems;
};

}  // namespace registry
}  // namespace ecs
}  // namespace core
}  // namespace nv3dvc

#endif  // SRC_CORE_ENTITYCOMPONENTSYSTEM_SYSTEM_H_
