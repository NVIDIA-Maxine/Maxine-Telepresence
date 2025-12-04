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

#ifndef SRC_MODULES_AUDIOMODULE_SYSTEMS_AUDIOINPUTSYSTEM_H_
#define SRC_MODULES_AUDIOMODULE_SYSTEMS_AUDIOINPUTSYSTEM_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Core/Engine/Engine.h"
#include "Core/EntityComponentSystem/Entity.h"
#include "Core/EntityComponentSystem/System.h"
#include "Modules/AudioModule/AcousticEchoCanceller.h"
#include "Modules/AudioModule/AudioRingBuffer.h"

namespace nv3dvc {
namespace core {
namespace ecs {
namespace registry {
class EntityRegistry;
}  // namespace registry
}  // namespace ecs
}  // namespace core

namespace modules {
namespace audiomodule {
namespace systems {

/// @defgroup AudioInputSystemProperties AudioInputSystem
/// @ingroup  SystemProperties
/// @brief    System for capturing audio
///
/// The system acts on entities with components components::AudioInputComponent attached. During scene load, the system
/// will create a callback that an AudioInputComponent can call whenever it has new audio data from its device. The
/// callback will iterate over all entities with AudioSinkComponent attached and push the audio samples to them.

/// See @ref AudioInputSystemProperties
class AudioInputSystem : public core::ecs::System {
 public:
  constexpr static const char* NAME = "AudioInputSystem";
  std::string Name() const override { return NAME; };

  AudioInputSystem();
  ~AudioInputSystem() override;

  core::Error OnLoadScene(core::ecs::registry::EntityRegistry* reg) override;

  core::Error OnUnloadScene(core::ecs::registry::EntityRegistry* reg) override;

  core::Error Run(core::ecs::registry::EntityRegistry* reg, float dt) override;

 public:
  /// @ingroup AudioInputSystemProperties
  /// @{
  core::properties::Property<std::string> afx_sdk_model_dir = {
      this,
      "afx_sdk_model_dir",
      "Path to AFX SDK model folder. The default value will be determined based on the environment variable AFXSDK "
      "which should be set before running the engine. See README.md for details on setting up environment variables.",
      core::engine::Engine::GetAfxSdkDir() + "bin/models",
  };
  core::properties::Property<bool> enable_aec = {
      this,
      "enable_aec",
      "Enable Acoustic Echo Cancellation (AEC).",
      true,
  };
  /// @}

 private:
  struct InputProcessor {
    struct AECProvider {
      std::unique_ptr<IEchoCanceller> echo_canceller;
      // Far-end ring buffer for AEC.
      std::shared_ptr<AudioRingBuffer> far_end_audio_data_ring_buffer = nullptr;
    };

    std::unique_ptr<AudioRingBuffer> input_ring_buffer;
    std::unordered_map<core::ecs::Entity, AECProvider, core::ecs::EntityHash> sources_aec_provider;
    int aec_frame_size = 1;
    std::vector<float> tmp_buffer, tmp_buffer2, tmp_far_end_frame;
  };
  std::unordered_map<core::ecs::Entity, InputProcessor, core::ecs::EntityHash> m_inputsProcessor;
  std::vector<float> m_tmpBuffer;
};

}  // namespace systems
}  // namespace audiomodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_AUDIOMODULE_SYSTEMS_AUDIOINPUTSYSTEM_H_
