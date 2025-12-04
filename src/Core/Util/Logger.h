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

#ifndef SRC_CORE_UTIL_LOGGER_H_
#define SRC_CORE_UTIL_LOGGER_H_

#include "nvCVLogger.h"

namespace nv3dvc {
namespace core {
namespace util {

extern NvCVLogger gLogger;

// Before calling any of these LOG_* macros, you should configure the logger:
// int err = gLogger.init(int verbosity, const char *file, void (*)(void*, const char*), void *user_data);
// Log to stderr:
//   int err = gLogger.init(NVCV_LOG_ERROR, "stderr", nullptr, nullptr);
// Log to file:
//   int err = gLogger.init(NVCV_LOG_ERROR, "MyLogFile.txt", nullptr, nullptr);
// Log to an object's log(void* ptr, const char *msg) function.
//   int err = gLogger.init(NVCV_LOG_ERROR, nullptr, &myLogger::log), &myLogger);

// clang-format off
#ifdef NDEBUG
  #define _FILE_NAME NULL
  #define _LINE_NUMBER 0
  #define _FUNC_ NULL
#else  // DEBUG
  #ifdef __FILE_NAME__
    #define _FILE_NAME __FILE_NAME__
  #elif defined(__file__)
    #define _FILE_NAME __file__
  #elif defined(_MSC_VER)
    #define _FILE_NAME __FILE__
  #else  // gcc or clang
    #define _FILE_NAME (__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : __FILE__)
  #endif  // _MSC_VER
  #define _LINE_NUMBER __LINE__
  #define _FUNC_ __FUNCTION__
#endif  // DEBUG
// clang-format on

}  // namespace util
}  // namespace core
}  // namespace nv3dvc

// clang-format off
#define LOG_FATAL(...)   nv3dvc::core::util::gLogger.log(_FILE_NAME, _LINE_NUMBER, _FUNC_, NVCV_LOG_FATAL,   __VA_ARGS__)
#define LOG_ERROR(...)   nv3dvc::core::util::gLogger.log(_FILE_NAME, _LINE_NUMBER, _FUNC_, NVCV_LOG_ERROR,   __VA_ARGS__)
#define LOG_WARNING(...) nv3dvc::core::util::gLogger.log(_FILE_NAME, _LINE_NUMBER, _FUNC_, NVCV_LOG_WARNING, __VA_ARGS__)
#define LOG_INFO(...)    nv3dvc::core::util::gLogger.log(_FILE_NAME, _LINE_NUMBER, _FUNC_, NVCV_LOG_INFO,    __VA_ARGS__)
#define LOG_DEBUG(...)   nv3dvc::core::util::gLogger.log(_FILE_NAME, _LINE_NUMBER, _FUNC_, NVCV_LOG_DEBUG,   __VA_ARGS__)
#define LOG_VERBOSE(...) nv3dvc::core::util::gLogger.log(_FILE_NAME, _LINE_NUMBER, _FUNC_, NVCV_LOG_VERBOSE, __VA_ARGS__)
// clang-format on

#endif  // SRC_CORE_UTIL_LOGGER_H_
