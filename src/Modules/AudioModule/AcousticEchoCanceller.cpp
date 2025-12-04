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

#include "AcousticEchoCanceller.h"

#include <cuda_runtime.h>

#include <string>
#include <vector>

#include "Core/Util/Logger.h"
#include "nvAFXAec.h"
#include "nvAudioEffects.h"

namespace nv3dvc {
namespace modules {
namespace audiomodule {

static std::string GetComputeAlias() {
  // Get compute capability
  int device_id;
  cudaGetDevice(&device_id);
  int device_prop_major, device_prop_minor;
  if (cudaDeviceGetAttribute(&device_prop_major, cudaDevAttrComputeCapabilityMajor, device_id) != cudaSuccess ||
      cudaDeviceGetAttribute(&device_prop_minor, cudaDevAttrComputeCapabilityMinor, device_id) != cudaSuccess) {
    return "";
  }

  std::string compute_alias;
  if (device_prop_major == 7 && device_prop_minor == 5) {
    compute_alias = "turing";
  } else if (device_prop_major == 8 && device_prop_minor == 6) {
    compute_alias = "ampere";
  } else if (device_prop_major == 8 && device_prop_minor == 9) {
    compute_alias = "ada";
  } else if (device_prop_major >= 10) {
    compute_alias = "blackwell";
  } else {
    LOG_ERROR("Unsupported compute capability %d.%d", device_prop_major, device_prop_minor);
  }
  return compute_alias;
}

class AcousticEchoCanceller : public IEchoCanceller {
 public:
  AcousticEchoCanceller() {}

  ~AcousticEchoCanceller() override {}

  bool Initialize(const std::string& afx_model_dir) override {
    std::string compute_alias = GetComputeAlias();
    if (compute_alias.empty()) return false;
    const std::string model_file = afx_model_dir + '/' + compute_alias + '/' + m_aecModelFilename;
    LOG_DEBUG("AEC model file path: %s", model_file.c_str());

    NvAFX_Status status;

    status = NvAFX_CreateEffect(NVAFX_EFFECT_AEC, &m_handle);
    if (status != NVAFX_STATUS_SUCCESS) {
      LOG_ERROR("NvAFX_CreateEffect() failed with error %s", GetErrorCodeString(status));
      return false;
    }

    status = NvAFX_SetString(m_handle, NVAFX_PARAM_MODEL_PATH, model_file.c_str());
    if (status != NVAFX_STATUS_SUCCESS) {
      LOG_ERROR("NvAFX_SetString() failed with error %s", GetErrorCodeString(status));
      return false;
    } else {
      LOG_DEBUG("NVAFX_PARAM_MODEL_PATH set to %s", model_file.c_str());
    }

    // Another option could be to use cudaGetDeviceCount for num
    int num_supported_devices = 0;
    status = NvAFX_GetSupportedDevices(m_handle, &num_supported_devices, nullptr);
    if (status != NVAFX_STATUS_OUTPUT_BUFFER_TOO_SMALL) {
      LOG_ERROR("Could not get number of supported devices with error %s", GetErrorCodeString(status));
      return false;
    }

    LOG_DEBUG("Number of supported devices for this model: %d", num_supported_devices);

    std::vector<int> supported_devices(num_supported_devices);
    status = NvAFX_GetSupportedDevices(m_handle, &num_supported_devices, supported_devices.data());
    if (status != NVAFX_STATUS_SUCCESS) {
      LOG_ERROR("No supported devices found with error %s", GetErrorCodeString(status));
      return false;
    }

    LOG_DEBUG("Devices supported (sorted by preference)");
    for (int device : supported_devices) {
      LOG_DEBUG("- %d", device);
    }

    LOG_DEBUG("Loading effect... ");
    status = NvAFX_Load(m_handle);
    if (status != NVAFX_STATUS_SUCCESS) {
      LOG_ERROR("NvAFX_Load() failed with error %s", GetErrorCodeString(status));
      return false;
    }
    LOG_DEBUG("Done");

    status = NvAFX_GetU32(m_handle, NVAFX_PARAM_INPUT_SAMPLE_RATE, &m_aecInputSampleRate);
    if (status != NVAFX_STATUS_SUCCESS) {
      LOG_ERROR("NvAFX_GetU32() failed with error %s", GetErrorCodeString(status));
      return false;
    }
    status = NvAFX_GetU32(m_handle, NVAFX_PARAM_OUTPUT_SAMPLE_RATE, &m_aecOutputSampleRate);
    if (status != NVAFX_STATUS_SUCCESS) {
      LOG_ERROR("NvAFX_GetU32() failed with error %s", GetErrorCodeString(status));
      return false;
    }
    status = NvAFX_GetU32(m_handle, NVAFX_PARAM_NUM_INPUT_CHANNELS, &m_aecNumInputChannels);
    if (status != NVAFX_STATUS_SUCCESS) {
      LOG_ERROR("NvAFX_GetU32() failed with error %s", GetErrorCodeString(status));
      return false;
    }
    status = NvAFX_GetU32(m_handle, NVAFX_PARAM_NUM_OUTPUT_CHANNELS, &m_aecNumOutputChannels);
    if (status != NVAFX_STATUS_SUCCESS) {
      LOG_ERROR("NvAFX_GetU32() failed with error %s", GetErrorCodeString(status));
      return false;
    }
    status = NvAFX_GetU32(m_handle, NVAFX_PARAM_NUM_INPUT_SAMPLES_PER_FRAME, &m_aecNumInputSamplesPerFrame);
    if (status != NVAFX_STATUS_SUCCESS) {
      LOG_ERROR("NvAFX_GetU32() failed with error %s", GetErrorCodeString(status));
      return false;
    }
    status = NvAFX_GetU32(m_handle, NVAFX_PARAM_NUM_OUTPUT_SAMPLES_PER_FRAME, &m_aecNumOutputSamplesPerFrame);
    if (status != NVAFX_STATUS_SUCCESS) {
      LOG_ERROR("NvAFX_GetU32() failed with error %s", GetErrorCodeString(status));
      return false;
    }

    m_framesPerBuffer = m_aecNumInputSamplesPerFrame;

    if (m_aecNumInputSamplesPerFrame != m_aecNumOutputSamplesPerFrame) {
      LOG_ERROR("AEC: input and output frame size don't match: (input %d, output %d)\n", m_aecNumInputSamplesPerFrame,
                m_aecNumOutputSamplesPerFrame);
      return false;
    }

    m_intensityRatio = 0.5f;

    status = NvAFX_SetFloat(m_handle, NVAFX_PARAM_INTENSITY_RATIO, m_intensityRatio);
    if (status != NVAFX_STATUS_SUCCESS) {
      LOG_ERROR("NvAFX_SetFloat(Intensity Ratio: %f) failed with error %s", GetErrorCodeString(status),
                m_intensityRatio);
    } else {
      LOG_DEBUG("NVAFX_PARAM_INTENSITY_RATIO set to: %f", m_intensityRatio);
    }

    float intensity_ratio_local;
    status = NvAFX_GetFloat(m_handle, NVAFX_PARAM_INTENSITY_RATIO, &intensity_ratio_local);
    if (status != NVAFX_STATUS_SUCCESS) {
      LOG_ERROR("NvAFX_GetFloat() failed with error %s", GetErrorCodeString(status));
      return false;
    }

    LOG_DEBUG("AEC Input Sample rate          %u", m_aecInputSampleRate);
    LOG_DEBUG("AEC Output Sample rate         %u", m_aecOutputSampleRate);
    LOG_DEBUG("AEC Input Channels             %u", m_aecNumInputChannels);
    LOG_DEBUG("AEC Output Channels            %u", m_aecNumOutputChannels);
    LOG_DEBUG("AEC Input Samples per frame    %u", m_aecNumInputSamplesPerFrame);
    LOG_DEBUG("AEC Output Samples per frame   %u", m_aecNumOutputSamplesPerFrame);
    LOG_DEBUG("AEC Intensity Ratio            %u", intensity_ratio_local);

    return true;
  }

  int GetNumSamplesPerFrame() const override { return m_aecNumInputSamplesPerFrame; }

  bool Filter(const float* near_end, const float* far_end, float* filtered) override {
    // first input is mic input
    // second input is what we would send to output device (far end)
    const float* input[2] = {near_end, far_end};
    // the output is what is the model produces
    float* output[1] = {filtered};
    NvAFX_Status status = NvAFX_Run(m_handle, input, output, m_aecNumInputSamplesPerFrame, m_aecNumInputChannels);
    if (status != NVAFX_STATUS_SUCCESS) {
      LOG_ERROR("NvAFX_Run() failed with error %s", GetErrorCodeString(status));
      return false;
    }

    return true;
  }

 private:
  // NvAFX
  NvAFX_Handle m_handle = nullptr;
  float m_intensityRatio = 1.0f;
  // NvAFX model Params
  std::string m_aecModelFilename = "aec_48k.trtpkg";
  unsigned int m_aecInputSampleRate = 0;
  unsigned int m_aecOutputSampleRate = 0;
  unsigned int m_aecNumInputChannels = 0;
  unsigned int m_aecNumOutputChannels = 0;
  unsigned int m_aecNumInputSamplesPerFrame = 0;
  unsigned int m_aecNumOutputSamplesPerFrame = 0;

  const char* GetErrorCodeString(NvAFX_Status status) {
    switch (status) {
      case NVAFX_STATUS_SUCCESS:
        return "NVAFX_STATUS_SUCCESS";
      case NVAFX_STATUS_FAILED:
        return "NVAFX_STATUS_FAILED";
      case NVAFX_STATUS_INVALID_HANDLE:
        return "NVAFX_STATUS_INVALID_HANDLE";
      case NVAFX_STATUS_INVALID_PARAM:
        return "NVAFX_STATUS_INVALID_PARAM";
      case NVAFX_STATUS_IMMUTABLE_PARAM:
        return "NVAFX_STATUS_IMMUTABLE_PARAM";
      case NVAFX_STATUS_INSUFFICIENT_DATA:
        return "NVAFX_STATUS_INSUFFICIENT_DATA";
      case NVAFX_STATUS_EFFECT_NOT_AVAILABLE:
        return "NVAFX_STATUS_EFFECT_NOT_AVAILABLE";
      case NVAFX_STATUS_OUTPUT_BUFFER_TOO_SMALL:
        return "NVAFX_STATUS_OUTPUT_BUFFER_TOO_SMALL";
      case NVAFX_STATUS_MODEL_LOAD_FAILED:
        return "NVAFX_STATUS_MODEL_LOAD_FAILED";
      case NVAFX_STATUS_32_SERVER_NOT_REGISTERED:
        return "NVAFX_STATUS_32_SERVER_NOT_REGISTERED";
      case NVAFX_STATUS_32_COM_ERROR:
        return "NVAFX_STATUS_32_COM_ERROR";
      case NVAFX_STATUS_GPU_UNSUPPORTED:
        return "NVAFX_STATUS_GPU_UNSUPPORTED";
      default:
        return "NVAFX_STATUS_SUCCESS";
    }
  }
};

std::unique_ptr<IEchoCanceller> CreateEchoCanceller() { return std::make_unique<AcousticEchoCanceller>(); }

}  // namespace audiomodule
}  // namespace modules
}  // namespace nv3dvc
