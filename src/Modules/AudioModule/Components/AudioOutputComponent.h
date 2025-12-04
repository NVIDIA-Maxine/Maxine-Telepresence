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

#ifndef SRC_MODULES_AUDIOMODULE_COMPONENTS_AUDIOOUTPUTCOMPONENT_H_
#define SRC_MODULES_AUDIOMODULE_COMPONENTS_AUDIOOUTPUTCOMPONENT_H_

#include <memory>
#include <string>
#include <vector>

#include "Core/EntityComponentSystem/Component.h"
#include "Core/Error.h"
#include "Core/Properties/Property.h"

namespace oboe {
namespace resampler {
class MultiChannelResampler;
}  // namespace resampler
}  // namespace oboe

namespace nv3dvc {
namespace modules {
namespace audiomodule {

namespace components {

/// @defgroup AudioOutputComponentProperties AudioOutputComponent
/// @ingroup  ComponentProperties
/// @brief    Represents an audio output device associated with an entity
///
/// Acted on by the systems::AudioOutputSystem system

/// See @ref  AudioOutputComponentProperties
class AudioOutputComponent : public core::ecs::Component {
 public:
  constexpr static const char* NAME = "AudioOutputComponent";
  std::string Name() const override { return NAME; };

  AudioOutputComponent();
  ~AudioOutputComponent() override;

  core::Error Initialize();
  void Uninitialize();
  bool IsInitialized() const;

  /// @brief Get the sample rate of the device output.
  /// @return The sample rate in Hz.
  int GetDeviceOutputSampleRate() const;

  /// @brief Get the number of channels for the device output.
  /// @return The number of channels.
  int GetDeviceOutputChannels() const;

  /// @brief Sets the source callback function.
  /// @param[in] callback Function to be called when audio data is requested by the output device.
  void SetSourceCallback(std::function<void(float* dst_data, int dst_samples)> callback);

  /// @brief Adds an output capture callback function.
  /// @param[in] callback Function to be called just before audio data is sent to the output device.
  void AddOutputCaptureCallback(std::function<void(const float* src_data, int src_samples)> callback);

  /// @brief Starts the audio output thread.
  void StartOutputThread();

  /// @brief Stops the audio output thread.
  void StopOutputThread();

  /// @brief Checks if the audio output thread is started.
  /// @return True if the thread is started, false otherwise.
  bool IsOutputThreadStarted() const;

 public:
  /// @ingroup AudioOutputComponentProperties
  /// @{
  core::properties::Property<bool> use_default_device = {
      this,
      "use_default_device",
      "Whether to open the default playback device. If false, device_name will be used to choose the playback device",
      true,
  };
  core::properties::Property<std::string> device_name = {
      this,
      "device_name",
      "The name of the audio playback device",
      "",
  };
  /// @}
 private:
  bool OutputCallback(float* output_buffer, size_t num_output_samples);
  friend struct OutputCallbackWrapper;

  bool m_isInitialized = false;
  int m_deviceIndex = -1;
  // PortAudio stream for playback.
  void* m_audioStream = nullptr;
  int m_deviceChannels = 0;
  int m_deviceSampleRate = 0;
  int m_sourceSampleRate = 0;

  std::unique_ptr<oboe::resampler::MultiChannelResampler> m_resampler;

  // Source audio gets popped from each source ring buffer into this tmp buffer for intermediate processing.
  // Then the processed audio gets mixed into the PortAudio output buffer.
  std::vector<float> m_tmpSourceBuffer;
  std::function<void(float* dst_data, int dst_samples)> m_sourceCallback;

  // Additional callbacks that can receive the audio immediately before being passed to PortAudio.
  std::vector<std::function<void(const float* src_data, int src_samples)>> m_outputCaptureCallbacks;
};

}  // namespace components
}  // namespace audiomodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_AUDIOMODULE_COMPONENTS_AUDIOOUTPUTCOMPONENT_H_
