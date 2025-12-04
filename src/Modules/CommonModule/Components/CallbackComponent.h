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

#ifndef SRC_MODULES_COMMONMODULE_COMPONENTS_CALLBACKCOMPONENT_H_
#define SRC_MODULES_COMMONMODULE_COMPONENTS_CALLBACKCOMPONENT_H_

#include <mutex>  // NOLINT(build/c++11) (cpplint outside Chromium: https://github.com/google/styleguide/issues/483)
#include <string>

#include "Core/EntityComponentSystem/Component.h"
#include "Core/Error.h"
#include "nvCVImage.h"

namespace nv3dvc {
namespace modules {
namespace commonmodule {
namespace components {

/// @brief Callback component base class
///
/// For delegation of specific callback types, use extensions of this class.
class CallbackComponent : public core::ecs::Component {
 public:
  /// @brief Footprint of callbacks functions used by all derivatives of the callback component
  using OnFiredCallback =
      std::function<core::Error(void* data_ptr, size_t data_size, size_t* processed_size, int data_type)>;

  /// @brief Set the function to be called on Fire
  /// @param callback The callback function
  void SetOnFiredCallback(OnFiredCallback callback);

  /// @brief Check if the component has an onFired callback function set
  /// @return Whether the component has an onFired callback function set
  bool HasOnFiredCallback() const;

  /// @brief Run the callback function
  /// @param[in,out] data_ptr       Data pointer to feed to the callback function
  /// @param[in]     data_size      The size of the data pointer
  /// @param[out]    processed_size The size of the data chunk that was processed by the callback function
  /// @param[in]     data_type      The data type. Should be known by the callback function and the caller
  /// @return        core::Error::SUCCESS If successful
  core::Error Fire(void* data_ptr, size_t data_size, size_t* processed_size, int data_type = 0);

  ~CallbackComponent() override = default;

 protected:
  CallbackComponent() = default;

 private:
  mutable std::mutex m_callbackMutex;
  OnFiredCallback m_onFired = nullptr;
};

/// @defgroup EncodedVideoCallbackComponentProperties EncodedVideoCallbackComponent
/// @ingroup  ComponentProperties
/// @brief    Callback component specifically for encoded video data

/// See @ref EncodedVideoCallbackComponentProperties
class EncodedVideoCallbackComponent : public CallbackComponent {
 public:
  constexpr static const char* NAME = "EncodedVideoCallbackComponent";
  std::string Name() const override { return NAME; };
  struct Buffers {
    uint8_t* metadata_buffer;
    size_t metadata_size;
    uint8_t* package_buffer;
    size_t package_size;
    float dt;
  };
};

/// @defgroup WebCameraAudioCallbackComponentProperties WebCameraAudioCallbackComponent
/// @ingroup  ComponentProperties
/// @brief    Callback component specifically for encoded audio data

/// See @ref WebCameraAudioCallbackComponentProperties
class WebCameraAudioCallbackComponent : public CallbackComponent {
 public:
  constexpr static const char* NAME = "WebCameraAudioCallbackComponent";
  std::string Name() const override { return NAME; };

  void SetFormat(int sample_rate, int num_channels) {
    m_sampleRate = sample_rate;
    m_numChannels = num_channels;
  }
  int SampleRate() const { return m_sampleRate; }
  int NumChannels() const { return m_numChannels; }

 private:
  int m_sampleRate = 0, m_numChannels = 0;
};

/// @defgroup RawMediaCallbackComponentProperties RawMediaCallbackComponent
/// @ingroup  ComponentProperties
/// @brief    Callback component for different data types

/// See @ref RawMediaCallbackComponentProperties
class RawMediaCallbackComponent : public CallbackComponent {
 public:
  enum Type : int { UNKNOWN = 0, VIDEO_FORMAT, AUDIO_FORMAT, VIDEO_DATA, AUDIO_DATA };
  struct VideoFormat {
    int width;                               ///< The number of pixels horizontally in the image.
    int height;                              ///< The number of pixels  vertically  in the image.
    int pitch;                               ///< The byte stride between pixels vertically.
    NvCVImage_PixelFormat pixel_format;      ///< The format of the pixels in the image.
    NvCVImage_ComponentType component_type;  ///< The data type used to represent each component of the image.
    unsigned char planar;                    ///< NVCV_CHUNKY, NVCV_PLANAR, NVCV_UYVY, ....
    unsigned char gpu_mem;                   ///< NVCV_CPU, NVCV_CPU_PINNED, NVCV_CUDA, NVCV_GPU
  };
  struct AudioFormat {
    int sample_rate;   ///< The sample rate of the audio [Hz]
    int num_channels;  ///< The number of audio channels
  };
};

/// @defgroup RecordingCallbackComponentProperties RecordingCallbackComponent
/// @ingroup  ComponentProperties
/// @brief    Callback for recording data

/// See @ref RecordingCallbackComponentProperties
class RecordingCallbackComponent : public RawMediaCallbackComponent {
 public:
  constexpr static const char* NAME = "RecordingCallbackComponent";
  std::string Name() const override { return NAME; }
};

}  // namespace components
}  // namespace commonmodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_COMMONMODULE_COMPONENTS_CALLBACKCOMPONENT_H_
