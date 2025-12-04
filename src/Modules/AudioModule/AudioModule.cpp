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

#include "AudioModule.h"

#include <portaudio.h>

#include "AudioUtils.h"
#include "Core/Error.h"
#include "Core/Serialization/Serialization.h"
#include "Core/Util/Logger.h"

namespace nv3dvc {
namespace modules {
namespace audiomodule {

NLOHMANN_JSON_SERIALIZE_ENUM(AudioInputApi, {{AudioInputApi::UNKNOWN, "UNKNOWN"},
                                             {AudioInputApi::PORT_AUDIO, "PORT_AUDIO"},
                                             {AudioInputApi::WEB_CAMERA, "WEB_CAMERA"}});

AudioModule::AudioModule() {
  RegisterComponent<components::AudioInputComponent>();
  RegisterComponent<components::AudioOutputComponent>();
  RegisterComponent<components::AudioSinkComponent>();
  RegisterComponent<components::AudioSourceComponent>();
  RegisterSystem<systems::AudioFeedbackSystem>();
  RegisterSystem<systems::AudioInputSystem>();
  RegisterSystem<systems::AudioOutputSystem>();
}

core::Error AudioModule::Initialize() {
  PaError err = paNoError;

  if (m_isInitialized) {
    LOG_ERROR("AudioModule already initialized");
    return core::ERR_INITIALIZATION;
  }

  err = Pa_Initialize();
  if (err != paNoError) {
    LOG_ERROR("Pa_Initialize returned %#x", err);
    return core::ERR_INITIALIZATION;
  }

  m_isInitialized = true;

  const PaHostApiIndex host_api = Pa_HostApiTypeIdToHostApiIndex(kDefaultAudioAPI);
  const PaHostApiInfo* host_api_info = Pa_GetHostApiInfo(host_api);

  const int num_devices = host_api_info->deviceCount;
  if (num_devices < 0) {
    LOG_ERROR("Pa_GetDeviceCount returned %#x", num_devices);
    err = num_devices;
    return core::ERR_INITIALIZATION;
  }
  LOG_DEBUG("Number of audio devices = %d", num_devices);

#ifndef NDEBUG
  LOG_DEBUG("PortAudio version: 0x%08X", Pa_GetVersion());
  LOG_DEBUG("Version text: '%s'", Pa_GetVersionInfo()->versionText);
  for (int host_api_device_index = 0; host_api_device_index < host_api_info->deviceCount; host_api_device_index++) {
    const PaDeviceIndex device_index = Pa_HostApiDeviceIndexToDeviceIndex(host_api, host_api_device_index);
    const PaDeviceInfo* device_info = Pa_GetDeviceInfo(device_index);
    const PaHostApiInfo* host_info = Pa_GetHostApiInfo(device_info->hostApi);
    const bool is_default_input = device_index == Pa_GetDefaultInputDevice();
    const bool is_api_default_input = device_index == host_info->defaultInputDevice;
    const bool is_default_output = device_index == Pa_GetDefaultOutputDevice();
    const bool is_api_default_output = device_index == host_info->defaultOutputDevice;

    LOG_DEBUG("Audio device #%d", device_index);
    LOG_DEBUG("Name                        = %s", device_info->name);
    LOG_DEBUG("Host API                    = %s", host_info->name);
    LOG_DEBUG("Is Default Input: %d", (int)is_default_input);
    LOG_DEBUG("Is Default %s Input: %d", host_info->name, (int)is_api_default_input);
    LOG_DEBUG("Is Default Output: %d", (int)is_default_output);
    LOG_DEBUG("Is Default %s Output: %d", host_info->name, (int)is_api_default_output);
    LOG_DEBUG("Max inputs                  = %d", device_info->maxInputChannels);
    LOG_DEBUG("Max outputs                 = %d", device_info->maxOutputChannels);
    LOG_DEBUG("Default low input latency   = %8.4f", device_info->defaultLowInputLatency);
    LOG_DEBUG("Default low output latency  = %8.4f", device_info->defaultLowOutputLatency);
    LOG_DEBUG("Default high input latency  = %8.4f", device_info->defaultHighInputLatency);
    LOG_DEBUG("Default high output latency = %8.4f", device_info->defaultHighOutputLatency);
    LOG_DEBUG("Default sample rate         = %8.2f", device_info->defaultSampleRate);
  }
#endif

  return core::SUCCESS;
}

core::Error AudioModule::Uninitialize() {
  if (!m_isInitialized) return core::SUCCESS;

  PaError err = Pa_Terminate();
  if (err != paNoError) {
    LOG_ERROR("Pa_Terminate returned %#x", err);
    return core::ERR_GENERAL;
  }

  m_isInitialized = false;
  return core::SUCCESS;
}

core::Error AudioModule::Update(float dt) { return core::SUCCESS; }

core::Error AudioModule::EncodeProperties(nlohmann::json* json_description, const PropertyOwner* property_owner) const {
  core::Error err = core::SUCCESS;
  CHECK_SUCCESS(core::serialization::EncodeProperties<AudioInputApi>(json_description, property_owner));
bail:
  return err;
}

core::Error AudioModule::DecodeProperties(const nlohmann::json& json_description, PropertyOwner* property_owner) const {
  core::Error err = core::SUCCESS;
  CHECK_SUCCESS(core::serialization::DecodeProperties<AudioInputApi>(json_description, property_owner));
bail:
  return err;
}

}  // namespace audiomodule
}  // namespace modules
}  // namespace nv3dvc
