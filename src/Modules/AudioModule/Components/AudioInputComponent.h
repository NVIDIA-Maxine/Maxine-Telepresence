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

#ifndef SRC_MODULES_AUDIOMODULE_COMPONENTS_AUDIOINPUTCOMPONENT_H_
#define SRC_MODULES_AUDIOMODULE_COMPONENTS_AUDIOINPUTCOMPONENT_H_

#include <memory>
#include <string>
#include <vector>

#include "Core/EntityComponentSystem/Component.h"
#include "Core/Error.h"
#include "Core/Properties/Property.h"
#include "Core/Util/Logger.h"

namespace oboe {
namespace resampler {
class MultiChannelResampler;
}  // namespace resampler
}  // namespace oboe

namespace nv3dvc {
namespace modules {
namespace audiomodule {

/// @brief Enumerates the available audio input APIs.
enum class AudioInputApi { UNKNOWN, PORT_AUDIO, WEB_CAMERA };

namespace systems {
class AudioInputSystem;
}

namespace components {

/// @defgroup AudioInputComponentProperties AudioInputComponent
/// @ingroup  ComponentProperties
/// @brief    Represents an audio input device associated with an entity
///
/// Acted on by the systems::AudioInputSystem system

/// See @ref  AudioInputComponentProperties
class AudioInputComponent : public core::ecs::Component {
 public:
  constexpr static const char* NAME = "AudioInputComponent";
  std::string Name() const override { return NAME; };

  AudioInputComponent();
  ~AudioInputComponent() override;

  core::Error Initialize();
  void Uninitialize();
  bool IsInitialized() const;

  /// @brief Gets the sample rate of the audio input device.
  /// @return The sample rate in Hz.
  int GetSampleRate() const;

  /// @brief Gets the number of channels of the audio input device.
  /// @return The number of channels.
  int GetNumChannels() const;

  /// @brief Sets the sink callback function.
  /// @param[in] callback The callback function to be called when audio data is available from the input device.
  void SetSinkCallback(std::function<void(const float* src_data, int src_samples)> callback);

  /// @brief Starts the audio input thread.
  void StartInputThread();

  /// @brief Stops the audio input thread.
  void StopInputThread();

  /// @brief Checks if the audio input thread is started.
  /// @return True if the thread is started, false otherwise.
  bool IsInputThreadStarted() const;

 public:
  /// @ingroup AudioInputComponentProperties
  /// @{
  core::properties::Property<AudioInputApi> capture_api{
      this,
      "capture_api",
      "Which capture API to use for grabbing audio input",
      AudioInputApi::PORT_AUDIO,
  };
  core::properties::Property<std::atomic<float>> recording_gain{
      this,
      "recording_gain",
      "Gain to apply to captured audio samples",
      1.0f,
  };
  core::properties::Property<bool> use_default_device{
      this,
      "use_default_device",
      "Whether to open the default capture device for the chosen capture API. If false, the device_name will be used "
      "to choose the capture device",
      true,
  };
  core::properties::Property<std::string> device_name{
      this,
      "device_name",
      "The name of the audio capture device",
      "",
  };
  /// @}
 private:
  void SetDeviceFormat(int sample_rate, int num_channels);
  bool InputCallback(const float* input_buffer, size_t num_input_samples);
  friend struct InputCallbackWrapper;
  friend class systems::AudioInputSystem;

  bool m_isInitialized = false;
  int m_deviceIndex = 0;
  // PortAudio stream for recording.
  void* m_audioStream = nullptr;

  int m_deviceChannels = 0, m_captureChannels = 0;
  int m_deviceSampleRate = 0, m_captureSampleRate = 0;

  std::unique_ptr<oboe::resampler::MultiChannelResampler> m_resampler;

  // Audio gets copied from the PortAudio input buffer into these tmp buffers for intermediate processing.
  // Then the processed audio gets passed to m_sinkCallback.
  std::vector<float> m_tmpMixedBuffer, m_tmpResampledBuffer;
  std::function<void(const float* src_data, int src_samples)> m_sinkCallback;
};

}  // namespace components
}  // namespace audiomodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_AUDIOMODULE_COMPONENTS_AUDIOINPUTCOMPONENT_H_
