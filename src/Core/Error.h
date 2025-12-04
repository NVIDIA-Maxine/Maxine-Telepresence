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

#ifndef SRC_CORE_ERROR_H_
#define SRC_CORE_ERROR_H_

/// @file  Error.h
/// @brief Defines error codes and error-checking utilities

#include <cstdint>

#include "Core/Util/Logger.h"

#define BAIL_IF_APERR(err)                       \
  do {                                           \
    if ((err) != nv3dvc::core::Error::SUCCESS) { \
      goto bail;                                 \
    }                                            \
  } while (0)
#define BAIL_IF_NVCVERR(nverr, err)                   \
  do {                                                \
    NvCV_Status _nverr = (nverr);                     \
    if (_nverr != NVCV_SUCCESS) {                     \
      err = static_cast<nv3dvc::core::Error>(_nverr); \
      goto bail;                                      \
    }                                                 \
  } while (0)
#define BAIL_IF_GLERR(glerr, err)                     \
  do {                                                \
    GLenum _glerr = (glerr);                          \
    if (_glerr != GL_NO_ERROR) {                      \
      err = static_cast<nv3dvc::core::Error>(_glerr); \
      goto bail;                                      \
    }                                                 \
  } while (0)
#define BAIL_IF_CUERR(cuerr, err)                                                          \
  do {                                                                                     \
    cudaError_t _cuerr = (cuerr);                                                          \
    if (_cuerr != cudaSuccess) {                                                           \
      err = static_cast<nv3dvc::core::Error>(nv3dvc::core::Error::ERR_CUDA_BASE - _cuerr); \
      goto bail;                                                                           \
    }                                                                                      \
  } while (0)
#define BAIL_IF_NVERR(err)       \
  do {                           \
    if ((err) != NVCV_SUCCESS) { \
      goto bail;                 \
    }                            \
  } while (0)
#define BAIL_IF_ERR(err) \
  do {                   \
    if ((err) != 0) {    \
      goto bail;         \
    }                    \
  } while (0)
#define BAIL_IF_ZERO(x, err, code) \
  do {                             \
    if ((x) == 0) {                \
      err = code;                  \
      goto bail;                   \
    }                              \
  } while (0)
#define BAIL_IF_NONZERO(x, err, code) \
  do {                                \
    if ((x) != 0) {                   \
      err = code;                     \
      goto bail;                      \
    }                                 \
  } while (0)
#define BAIL_IF_FALSE(x, err, code) \
  do {                              \
    if ((x) == false) {             \
      err = code;                   \
      goto bail;                    \
    }                               \
  } while (0)
#define BAIL_IF_TRUE(x, err, code) \
  do {                             \
    if ((x) == true) {             \
      err = code;                  \
      goto bail;                   \
    }                              \
  } while (0)
#define BAIL_IF_NULL(x, err, code) \
  do {                             \
    if ((x) == nullptr) {          \
      err = code;                  \
      goto bail;                   \
    }                              \
  } while (0)
#define BAIL_IF_NEGATIVE(x, err, code) \
  do {                                 \
    if ((x) < 0) {                     \
      err = code;                      \
      goto bail;                       \
    }                                  \
  } while (0)
#define BAIL_IF_NONPOSITIVE(x, err, code) \
  do {                                    \
    if (!((x) > 0)) {                     \
      err = code;                         \
      goto bail;                          \
    }                                     \
  } while (0)
#define BAIL(err, code) \
  do {                  \
    err = code;         \
    goto bail;          \
  } while (0)
#define RETURN_IF_NVERR_LOG(err, log) \
  do {                                \
    if ((err) != NVCV_SUCCESS) {      \
      log;                            \
      return err;                     \
    }                                 \
  } while (0)
#define RETURN_IF_APERR_LOG(err, log)            \
  do {                                           \
    if ((err) != nv3dvc::core::Error::SUCCESS) { \
      log;                                       \
      return err;                                \
    }                                            \
  } while (0)

/// @brief If @c condition evaluates to @c false, this macro sets the variable @c err to @c errcode, calls @c LOG_ERROR
/// with any trailing arguments, then goto @c bail.
#define INTERNAL_CHECK(condition, errcode, msg, ...)                                                                  \
  do {                                                                                                                \
    /* Evaluate `condition` exactly once. */                                                                          \
    if (!(condition)) {                                                                                               \
      const nv3dvc::core::Error _tmp_errcode = errcode;                                                               \
      LOG_ERROR("Error %d (%s): " msg, _tmp_errcode, nv3dvc::core::ErrorStringFromCode(_tmp_errcode), ##__VA_ARGS__); \
      err = _tmp_errcode;                                                                                             \
      goto bail;                                                                                                      \
    }                                                                                                                 \
  } while (0)

/// @brief If @c condition evaluates to @c true, this macro sets the variable @c err to @c errcode, calls @c LOG_ERROR
/// with any trailing arguments, then goto @c bail.
#define CHECK_TRUE(condition, errcode, ...) \
  INTERNAL_CHECK((condition) == true, errcode, "Expected `" #condition "` to be true. " __VA_ARGS__)

/// @brief If @c condition evaluates to @c true, this macro sets the variable @c err to @c errcode, calls @c LOG_ERROR
/// with any trailing arguments, then goto @c bail.
#define CHECK_FALSE(condition, errcode, ...) \
  INTERNAL_CHECK((condition) == false, errcode, "Expected `" #condition "` to be false. " __VA_ARGS__)

/// @brief If @c ptr evaluates to @c nullptr, this macro sets the variable @c err to @c errcode, calls @c LOG_ERROR
/// with any trailing arguments, then goto @c bail.
#define CHECK_NONNULL(ptr, errcode, ...) \
  INTERNAL_CHECK((ptr) != nullptr, errcode, "Expected `" #ptr "` to be non-null. " __VA_ARGS__)

/// @brief If @c ptr does not evaluates to @c nullptr, this macro sets the variable @c err to @c errcode, calls @c
/// LOG_ERROR with any trailing arguments, then goto @c bail.
#define CHECK_NULL(ptr, errcode, ...) \
  INTERNAL_CHECK((ptr) == nullptr, errcode, "Expected `" #ptr "` to be null. " __VA_ARGS__)

/// @brief If @c val does not evaluate to @c core::Error::SUCCESS, this macro sets the variable @c err to @c val,
/// calls @c LOG_ERROR with any trailing arguments, then goto @c bail.
#define CHECK_SUCCESS(val, ...)                                           \
  do {                                                                    \
    /* Evaluate `val` exactly once. */                                    \
    const nv3dvc::core::Error _tmp_val = val;                             \
    INTERNAL_CHECK(_tmp_val == nv3dvc::core::Error::SUCCESS, _tmp_val,    \
                   "Expected `" #val "` to equal SUCCESS. " __VA_ARGS__); \
  } while (0)

/// @brief If @c val does not evaluate to @c NVCV_SUCCESS, this macro sets the variable @c err to @c val,
/// calls @c LOG_ERROR with any trailing arguments, then goto @c bail.
#define CHECK_NVCV_SUCCESS(val, ...)                                                     \
  do {                                                                                   \
    /* Evaluate `val` exactly once. */                                                   \
    const NvCV_Status _tmp_val = val;                                                    \
    INTERNAL_CHECK(_tmp_val == NVCV_SUCCESS, static_cast<nv3dvc::core::Error>(_tmp_val), \
                   "Expected `" #val "` to equal NVCV_SUCCESS. " __VA_ARGS__);           \
  } while (0)

/// @brief If @c val does not evaluate to @c CUDA_SUCCESS, this macro converts @c val to a @c core::Error, sets the
/// variable @c err to the converted value, calls @c LOG_ERROR with any trailing arguments, then goto @c bail.
#define CHECK_CU_SUCCESS(val, ...)                                                                  \
  do {                                                                                              \
    /* Evaluate `val` exactly once. */                                                              \
    const CUresult _tmp_val = val;                                                                  \
    INTERNAL_CHECK(_tmp_val == CUDA_SUCCESS,                                                        \
                   static_cast<nv3dvc::core::Error>(nv3dvc::core::Error::ERR_CUDA_BASE - _tmp_val), \
                   "Expected `" #val "` to equal CUDA_SUCCESS. " __VA_ARGS__);                      \
  } while (0)

/// @brief If @c val does not evaluate to @c cudaSuccess, this macro converts @c val to a @c core::Error, sets the
/// variable @c err to the converted value, calls @c LOG_ERROR with any trailing arguments, then goto @c bail.
#define CHECK_CUDA_SUCCESS(val, ...)                                                                \
  do {                                                                                              \
    /* Evaluate `val` exactly once. */                                                              \
    const cudaError_t _tmp_val = val;                                                               \
    INTERNAL_CHECK(_tmp_val == cudaSuccess,                                                         \
                   static_cast<nv3dvc::core::Error>(nv3dvc::core::Error::ERR_CUDA_BASE - _tmp_val), \
                   "Expected `" #val "` to equal cudaSuccess. " __VA_ARGS__);                       \
  } while (0)

/// @brief This macro evaluates the expression @c expr, then calls @c glGetError(). If @c glGetError() returns anything
/// other than @c GL_NO_ERROR, this macro converts the error code to a @c core::Error, sets the variable @c err to the
/// converted value, calls @c LOG_ERROR with any trailing arguments, then goto @c bail.
#define CHECK_GLGETERROR(expr, ...)                                                         \
  do {                                                                                      \
    expr;                                                                                   \
    const GLenum _tmp_val = glGetError();                                                   \
    /* glGetError should always be called in a loop, until it returns GL_NO_ERROR */        \
    for (GLenum _tmp_val2 = _tmp_val; _tmp_val2 != GL_NO_ERROR; _tmp_val2 = glGetError()) { \
    }                                                                                       \
    INTERNAL_CHECK(_tmp_val == GL_NO_ERROR, static_cast<nv3dvc::core::Error>(_tmp_val),     \
                   "Expected `" #expr "` not to set the error flag. " __VA_ARGS__);         \
  } while (0)

/// @brief Root namespace
namespace nv3dvc {
/// @brief Namespace for core library
namespace core {

/// @brief Application error codes. These error codes are all non-negative, so the Error type can successfully represent
/// the NVCV and NVCV-wrapped CUDA codes as well, since they are negative and will not collide.
enum Error : int32_t {
  SUCCESS = 0,                        ///< The operation was completed successfully
  ERR_GENERAL = 101,                  ///< An otherwise unspecified app error has occurred
  ERR_INITIALIZATION,                 ///< The object was not properly initialized
  ERR_UNIMPLEMENTED,                  ///< The requested feature has not yet been implemented in the app
  ERR_MEMORY,                         ///< There was a memory problem in the app
  ERR_FILE,                           ///< The app-requested file is not found
  ERR_READ,                           ///< There was a problem reading a file in the app
  ERR_WRITE,                          ///< There was a problem writing a file in the app
  ERR_NOT_PERMITTED,                  ///< The attempted operation is not permitted
  ERR_NOT_SUPPORTED,                  ///< The attempted operation is not supported
  ERR_EOF,                            ///< The input sequence has ended
  ERR_PARSE,                          ///< The file was not able to be parsed correctly
  ERR_SERIALIZE,                      ///< There was a problem serializing
  ERR_DESERIALIZE,                    ///< There was a problem deserializing
  ERR_REGISTRY,                       ///< The registry is missing
  ERR_PARAMETER_MISMATCH,             ///< There was a mismatch between a parameter and its expected value
  ERR_CAMERA,                         ///< No camera is available
  ERR_CAMERA_API,                     ///< The camera API function call failed
  ERR_WINDOW,                         ///< There is no valid window
  ERR_NOTHINGRENDERED,                ///< Nothing was rendered
  ERR_SCENE,                          ///< The scene is invalid
  ERR_DISPLAY,                        ///< The display could not be initialized or used
  ERR_DATA_UNAVAILABLE,               ///< The requested data is unavailable
  ERR_VIDEO_CONFIGURATION,            ///< The video configuration is invalid
  ERR_NETWORK_CONNECTION_INACTIVE,    ///< The network connection is inactive
  ERR_NETWORK_ERROR,                  ///< A network error has occured
  ERR_NETWORK_EOS,                    ///< A network end of stream message was received
  ERR_VIDEO_STREAM_ELEMENT_CREATION,  ///< There was a problem creating video stream elements
  ERR_AUDIO_STREAM_ELEMENT_CREATION,  ///< There was a problem creating audio stream elements
  ERR_VIDEO_STREAM_ELEMENT_LINK,      ///< There was a problem linking video stream elements
  ERR_AUDIO_STREAM_ELEMENT_LINK,      ///< There was a problem linking audio stream elements
  ERR_STREAM_STATE_CHANGE,            ///< Could not change stream state
  ERR_ITEM_NOT_FOUND,                 ///< The specified item was not found
  ERR_NULL_POINTER,                   ///< A NULL pointer was supplied when not expected
  ERR_TIMEOUT,                        ///< The operation timed out

  ERR_CUDA_BASE = -100,  ///< CUDA errors are offset from this value.

  // These are the exact OpenGL error codes, so we can easily convert them with a cast. Note 0x0500 == 1280.
  ERR_GL_INVALID_ENUM = 0x0500,                   ///< the GL enum  is not valid for this function
  ERR_GL_INVALID_VALUE = 0x0501,                  ///< the GL value is not valid for this function
  ERR_GL_INVALID_OPERATION = 0x0502,              ///< the GL operation is not valid in the current state
  ERR_GL_STACK_OVERFLOW = 0x0503,                 ///< the GL stack cannot grow any larger
  ERR_GL_STACK_UNDERFLOW = 0x0504,                ///< the GL stack is empty
  ERR_GL_OUT_OF_MEMORY = 0x0505,                  ///< the GL has run out of memory
  ERR_GL_INVALID_FRAMEBUFFER_OPERATION = 0x0506,  ///< the GL framebuffer is not complete enough for this operation
  ERR_GL_CONTEXT_LOST = 0x0507,                   ///< the GL context has been lost
};

const char* ErrorStringFromCode(nv3dvc::core::Error err);

}  // namespace core
}  // namespace nv3dvc
#endif  // SRC_CORE_ERROR_H_
