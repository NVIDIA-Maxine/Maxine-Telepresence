/*
 * SPDX-FileCopyrightText: Copyright (c) 2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: LicenseRef-NvidiaProprietary
 *
 * NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
 * property and proprietary rights in and to this material, related
 * documentation and any modifications thereto. Any use, reproduction,
 * disclosure or distribution of this material and related documentation
 * without an express license agreement from NVIDIA CORPORATION or
 * its affiliates is strictly prohibited.
 */

#if defined(linux) || defined(unix) || defined(__linux)
#warning NvCVVolumeProxy.cpp not ported
#else

#include <string>

#include "nvCVTriplaneVolume.h"

#ifdef _WIN32
#define _WINSOCKAPI_
#include <tchar.h>
#include <windows.h>
#else  // !_WIN32
#include <dlfcn.h>
typedef void *HMODULE;
typedef void *HANDLE;
typedef void *HINSTANCE;
#endif  // _WIN32

// Parameter string does not include the file extension
#ifdef _WIN32
#define nvLoadLibrary(library) LoadLibrary(TEXT(library ".dll"))
#else  // !_WIN32
#define nvLoadLibrary(library) dlopen("lib" library ".so", RTLD_LAZY)
#endif  // _WIN32

inline void *nvGetProcAddress(HINSTANCE handle, const char *proc) {
  if (nullptr == handle) return nullptr;
#ifdef _WIN32
  return GetProcAddress(handle, proc);
#else   // !_WIN32
  return dlsym(handle, proc);
#endif  // _WIN32
}

inline int nvFreeLibrary(HINSTANCE handle) {
#ifdef _WIN32
  return FreeLibrary(handle);
#else
  return dlclose(handle);
#endif
}

HINSTANCE GetNvCVVolumeLib() {
  TCHAR path[MAX_PATH], tmp_path[MAX_PATH], full_path[MAX_PATH];
  static HINSTANCE nvcv_volume_lib = NULL;
  static bool path_set = false;
  if (!path_set) {
    nvcv_volume_lib = nvLoadLibrary("NvCVVolume");
    if (nvcv_volume_lib) path_set = true;
  }
  if (!path_set) {
    // There can be multiple apps on the system,
    // some might include the SDK in the app package and
    // others might expect the SDK to be installed in Program Files
    GetEnvironmentVariable(TEXT("NV_VIDEO_EFFECTS_PATH"), path, MAX_PATH);
    GetEnvironmentVariable(TEXT("NV_AR_SDK_PATH"), tmp_path, MAX_PATH);
    if (_tcscmp(path, TEXT("USE_APP_PATH")) && _tcscmp(tmp_path, TEXT("USE_APP_PATH"))) {
      // App has not set environment variable to "USE_APP_PATH"
      // So pick up the dll from Program Files
      GetEnvironmentVariable(TEXT("ProgramFiles"), path, MAX_PATH);
      size_t max_len = sizeof(full_path) / sizeof(TCHAR);
      _stprintf_s(full_path, max_len, TEXT("%s\\NVIDIA Corporation\\NVIDIA AR SDK\\"), path);
      SetDllDirectory(full_path);
      nvcv_volume_lib = nvLoadLibrary("NvCVVolume");
    }
    path_set = true;
  }
  return nvcv_volume_lib;
}

NvCV_Status NvCV_API NvCVVolume_AllocateTriplaneVolume(NvCVTriplaneVolume *triplane_volume) {
  static const auto func_ptr = (decltype(NvCVVolume_AllocateTriplaneVolume) *)nvGetProcAddress(
      GetNvCVVolumeLib(), "NvCVVolume_AllocateTriplaneVolume");
  if (nullptr == func_ptr) return NVCV_ERR_LIBRARY;
  return func_ptr(triplane_volume);
}

NvCV_Status NvCV_API NvCVVolume_AllocateTriplaneVolumeBatch(NvCVTriplaneVolume *triplane_volume,
                                                            unsigned int batch_size) {
  static const auto func_ptr = (decltype(NvCVVolume_AllocateTriplaneVolumeBatch) *)nvGetProcAddress(
      GetNvCVVolumeLib(), "NvCVVolume_AllocateTriplaneVolumeBatch");
  if (nullptr == func_ptr) return NVCV_ERR_LIBRARY;
  return func_ptr(triplane_volume, batch_size);
}

void NvCV_API NvCVVolume_DeallocateTriplaneVolume(NvCVTriplaneVolume *triplane_volume) {
  static const auto func_ptr = (decltype(NvCVVolume_DeallocateTriplaneVolume) *)nvGetProcAddress(
      GetNvCVVolumeLib(), "NvCVVolume_DeallocateTriplaneVolume");
  if (nullptr == func_ptr) return;
  func_ptr(triplane_volume);
}

NvCV_Status NvCV_API NvCVVolume_TransferTriplaneVolume(const NvCVTriplaneVolume *src, NvCVTriplaneVolume *dst,
                                                       CUstream_st *stream) {
  static const auto func_ptr = (decltype(NvCVVolume_TransferTriplaneVolume) *)nvGetProcAddress(
      GetNvCVVolumeLib(), "NvCVVolume_TransferTriplaneVolume");
  if (nullptr == func_ptr) return NVCV_ERR_LIBRARY;
  return func_ptr(src, dst, stream);
}

void NvCV_API NvCVVolume_NthTriplaneVolume(unsigned int n, NvCVTriplaneVolume *full, NvCVTriplaneVolume *nth) {
  static const auto func_ptr =
      (decltype(NvCVVolume_NthTriplaneVolume) *)nvGetProcAddress(GetNvCVVolumeLib(), "NvCVVolume_NthTriplaneVolume");
  if (nullptr == func_ptr) return;
  return func_ptr(n, full, nth);
}

#endif  // defined(linux) || defined(unix) || defined(__linux)