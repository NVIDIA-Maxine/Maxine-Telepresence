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
#ifndef SRC_SAMPLES_MYMODULE_MYMODULE_H_
#define SRC_SAMPLES_MYMODULE_MYMODULE_H_

#include <string>

#include "Core/Engine/Module.h"
#include "Core/EntityComponentSystem/Component.h"
#include "Core/EntityComponentSystem/Entity.h"
#include "Core/EntityComponentSystem/System.h"
#include "Modules/BehaviorModule/BehaviorModule.h"

namespace samples {

//! [Simple component sample]
class MyComponent : public nv3dvc::core::ecs::Component {
 public:
  constexpr static const char* NAME = "MyComponent";
  std::string Name() const override { return NAME; };

  MyComponent() = default;
  ~MyComponent() override = default;

 private:
};
//! [Simple component sample]

//! [Simple behavior sample]
class MyBehavior : public nv3dvc::modules::behaviormodule::components::BehaviorComponent {
 public:
  constexpr static const char* NAME = "MyBehavior";
  std::string Name() const override { return NAME; };

  MyBehavior() = default;
  ~MyBehavior() override = default;

  nv3dvc::core::Error OnEvent(nv3dvc::core::events::Event* e) override { return nv3dvc::core::Error::SUCCESS; }
  nv3dvc::core::Error OnUpdate(float dt) override { return nv3dvc::core::Error::SUCCESS; }
  nv3dvc::core::Error OnInitialize() override { return nv3dvc::core::Error::SUCCESS; }
  nv3dvc::core::Error OnUnloadScene() override { return nv3dvc::core::Error::SUCCESS; }
};
//! [Simple behavior sample]

//! [Simple system sample]
class MySystem : public nv3dvc::core::ecs::System {
 public:
  constexpr static const char* NAME = "MySystem";
  std::string Name() const override { return NAME; };

  MySystem() = default;

  nv3dvc::core::Error OnLoadScene(nv3dvc::core::ecs::registry::EntityRegistry* reg) override {
    return nv3dvc::core::Error::SUCCESS;
  }
  nv3dvc::core::Error Initialize() override { return nv3dvc::core::Error::SUCCESS; }
  nv3dvc::core::Error Uninitialize() override { return nv3dvc::core::Error::SUCCESS; }
  nv3dvc::core::Error Run(nv3dvc::core::ecs::registry::EntityRegistry* reg, float dt) override {
    return nv3dvc::core::Error::SUCCESS;
  }

 public:
 private:
};
//! [Simple system sample]

//! [Simple module sample]
class MyModule : public nv3dvc::core::engine::Module {
 public:
  constexpr static const char* NAME = "MyModule";
  std::string Name() const override { return NAME; };

  explicit MyModule(nv3dvc::modules::behaviormodule::BehaviorModule* behavior_module) {
    RegisterSystem<MySystem>();
    RegisterComponent<MyComponent>();
    behavior_module->RegisterBehaviorComponent<MyBehavior>();
  }
  ~MyModule() override = default;

  nv3dvc::core::Error Initialize() override { return nv3dvc::core::Error::SUCCESS; }
  nv3dvc::core::Error Uninitialize() override { return nv3dvc::core::Error::SUCCESS; }
  nv3dvc::core::Error Update(float dt) override { return nv3dvc::core::SUCCESS; }

 public:
 private:
};
//! [Simple module sample]

}  // namespace samples

#endif  // SRC_SAMPLES_MYMODULE_MYMODULE_H_
