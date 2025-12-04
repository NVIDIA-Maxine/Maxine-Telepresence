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

#include "Error.h"

#include "cuda.h"
#include "nvCVStatus.h"

namespace nv3dvc {
namespace core {

Error AppFromCudaErrorCode(::CUresult cuerr) {
  return (cuerr == CUDA_SUCCESS) ? SUCCESS
                                 : static_cast<Error>(static_cast<int>(NVCV_ERR_CUDA_BASE) - static_cast<int>(cuerr));
}

Error AppFromNVCVErrorCode(::NvCV_Status err) { return static_cast<Error>(err); }

const char* ErrorStringFromCode(nv3dvc::core::Error code) {
  struct Entry {
    Error code;
    const char* str;
  };
  static const Entry lut[] = {
      {SUCCESS, "The operation was completed successfully"},
      {ERR_GENERAL, "An otherwise unspecified app error has occurred"},
      {ERR_INITIALIZATION, "The object was not properly initialized"},
      {ERR_UNIMPLEMENTED, "The requested feature has not yet been implemented in the app"},
      {ERR_MEMORY, "There was a memory problem in the app"},
      {ERR_FILE, "The app-requested file is not found"},
      {ERR_READ, "There was a problem reading a file in the app"},
      {ERR_WRITE, "There was a problem writing a file in the app"},
      {ERR_NOT_PERMITTED, "The attempted operation is not permitted"},
      {ERR_NOT_SUPPORTED, "The attempted operation is not supported"},
      {ERR_EOF, "The input sequence has ended"},
      {ERR_PARSE, "The file was not able to be parsed correctly"},
      {ERR_SERIALIZE, "There was a problem serializing"},
      {ERR_DESERIALIZE, "There was a problem deserializing"},
      {ERR_REGISTRY, "The registry is missing"},
      {ERR_PARAMETER_MISMATCH, "There was a mismatch between a parameter and its expected value"},
      {ERR_CAMERA, "No camera is available"},
      {ERR_CAMERA_API, "The camera API function call failed"},
      {ERR_WINDOW, "There is no valid window"},
      {ERR_NOTHINGRENDERED, "Nothing was rendered"},
      {ERR_SCENE, "The scene is invalid"},
      {ERR_DISPLAY, "The display could not be initialized or used"},
      {ERR_DATA_UNAVAILABLE, "The requested data is unavailable"},
      {ERR_VIDEO_CONFIGURATION, "The video configuration is invalid"},
      {ERR_NETWORK_CONNECTION_INACTIVE, "The network connection is inactive"},
      {ERR_NETWORK_ERROR, "A network error has occured"},
      {ERR_NETWORK_EOS, "A network end of stream message was received"},
      {ERR_VIDEO_STREAM_ELEMENT_CREATION, "There was a problem creating video stream elements"},
      {ERR_AUDIO_STREAM_ELEMENT_CREATION, "There was a problem creating audio stream elements"},
      {ERR_VIDEO_STREAM_ELEMENT_LINK, "There was a problem linking video stream elements"},
      {ERR_AUDIO_STREAM_ELEMENT_LINK, "There was a problem linking audio stream elements"},
      {ERR_STREAM_STATE_CHANGE, "Could not change stream state"},
      {ERR_ITEM_NOT_FOUND, "The specified item was not found"},
      {ERR_NULL_POINTER, "A NULL pointer was supplied when not expected"},
      {ERR_TIMEOUT, "The operation timed out"},
      {ERR_GL_INVALID_ENUM, "the GL enum  is not valid for this function"},
      {ERR_GL_INVALID_VALUE, "the GL value is not valid for this function"},
      {ERR_GL_INVALID_OPERATION, "the GL operation is not valid in the current state"},
      {ERR_GL_STACK_OVERFLOW, "the GL stack cannot grow any larger"},
      {ERR_GL_STACK_UNDERFLOW, "the GL stack is empty"},
      {ERR_GL_OUT_OF_MEMORY, "the GL has run out of memory"},
      {ERR_GL_INVALID_FRAMEBUFFER_OPERATION, "the GL framebuffer is not complete enough for this operation"},
      {ERR_GL_CONTEXT_LOST, "the GL context has been lost"},
  };
  for (const Entry* p = lut; p != &lut[sizeof(lut) / sizeof(lut[0])]; ++p)
    if (p->code == code) return p->str;
  return ::NvCV_GetErrorStringFromCode((::NvCV_Status)code);
}

}  // namespace core
}  // namespace nv3dvc
