# Copyright 2020-2024 NVIDIA Corporation.  All rights reserved.
#
# Please refer to the NVIDIA end user license agreement (EULA) associated
# with this source code for terms and conditions that govern your use of
# this software. Any use, reproduction, disclosure, or distribution of
# this software and related documentation outside the terms of the EULA
# is strictly prohibited.

cmake_minimum_required(VERSION 3.25)

project(MaxineTelepresence)

# Set C++ standard and disable non-standard extensions.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

# Set install prefix if not explicitly specified to CMake.
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/package" CACHE PATH "Path to where package will be installed" FORCE)
endif()

# CUDA
find_package(CUDAToolkit REQUIRED)
set(CMAKE_CUDA_ARCHITECTURES 75 86 70 80 89 90)

if(MSVC)
  # Use new (more conformant) preprocessor.
  add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:/Zc:preprocessor>")
endif()

if(WIN32)
  add_compile_definitions(
    _CRT_SECURE_NO_WARNINGS
    NOMINMAX
  )
endif()

# Add additional dependencies macro for VS.
if(CMAKE_GENERATOR MATCHES "Visual Studio")
  set(CMAKE_CXX_STANDARD_LIBRARIES "%(AdditionalDependencies)")
endif()

if(MSVC)
  # Parallel compilation.
  add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:/MP>")
endif()

# Set output directory for .exe, .dll, and .pdb files.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG          ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL     ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# Find SDKs
if(NOT DEFINED ENV{ARSDK})
  set(ENV{ARSDK} "${CMAKE_SOURCE_DIR}/Internal/nvar")
endif()
if(NOT DEFINED ENV{VFXSDK})
  set(ENV{VFXSDK} "${CMAKE_SOURCE_DIR}/Internal/nvvfx")
endif()
if(NOT DEFINED ENV{AFXSDK})
  set(ENV{AFXSDK} "${CMAKE_SOURCE_DIR}/Internal/nvafx")
endif()

set(ARSDK_ROOT "$ENV{ARSDK}" CACHE PATH "Root directory of AR SDK installation")
set(VFXSDK_ROOT "$ENV{VFXSDK}" CACHE PATH "Root directory of VFX SDK installation")
set(AFXSDK_ROOT "$ENV{AFXSDK}" CACHE PATH "Path to AFX SDK root")

find_package(ARSDK)
find_package(VFXSDK)
find_package(AFXSDK)

if(WIN32)
  # DLLs for debug build environment
  list(APPEND DEPENDENCIES_DLL_DIRS ${ARSDK_ROOT}/bin)
  list(APPEND DEPENDENCIES_DLL_DIRS ${ARSDK_ROOT}/features/nvarfaceboxdetection/bin)
  list(APPEND DEPENDENCIES_DLL_DIRS ${ARSDK_ROOT}/features/nvarfaceexpressions/bin)
  list(APPEND DEPENDENCIES_DLL_DIRS ${ARSDK_ROOT}/features/nvarlandmarkdetection/bin)
  list(APPEND DEPENDENCIES_DLL_DIRS ${ARSDK_ROOT}/features/nvarvolumetricencoding/bin)
  list(APPEND DEPENDENCIES_DLL_DIRS ${ARSDK_ROOT}/features/nvarvolumetricrendering/bin)
  list(APPEND DEPENDENCIES_DLL_DIRS ${VFXSDK_ROOT}/bin)
  list(APPEND DEPENDENCIES_DLL_DIRS ${VFXSDK_ROOT}/features/nvvfxgreenscreen/bin)
  list(APPEND DEPENDENCIES_DLL_DIRS ${AFXSDK_ROOT}/bin)
  list(APPEND DEPENDENCIES_DLL_DIRS ${AFXSDK_ROOT}/features/nvafxaec/bin)

  option(NV3DVC_PACKAGE_SDK_DEV "Package source files and lib files for for AR, VFX and AFX SDKs" OFF)
  option(NV3DVC_PACKAGE_SDK_MODELS "Package TRT model files for for AR, VFX and AFX SDKs" ON)
  if(${NV3DVC_PACKAGE_SDK_DEV})
    set(PATTERN_INCLUDE_SRC "*.cpp")
    set(PATTERN_INCLUDE_HDR "*.h")
    set(PATTERN_INCLUDE_LIB "*.lib")
  endif()
  install(DIRECTORY ${ARSDK_ROOT}/ DESTINATION ARSDK
    FILES_MATCHING
    PATTERN ${PATTERN_INCLUDE_HDR}
    PATTERN ${PATTERN_INCLUDE_SRC}
    PATTERN ${PATTERN_INCLUDE_LIB}
    PATTERN "*license*"
    PATTERN "*License*"
    PATTERN "samples/*" EXCLUDE
    PATTERN "bin/*.dll")
  install(DIRECTORY ${VFXSDK_ROOT}/ DESTINATION VFXSDK
    FILES_MATCHING
    PATTERN ${PATTERN_INCLUDE_HDR}
    PATTERN ${PATTERN_INCLUDE_SRC}
    PATTERN ${PATTERN_INCLUDE_LIB}
    PATTERN "*license*"
    PATTERN "*License*"
    PATTERN "samples/*" EXCLUDE
    PATTERN "bin/*.dll")
  install(DIRECTORY ${AFXSDK_ROOT}/ DESTINATION AFXSDK
    FILES_MATCHING
    PATTERN ${PATTERN_INCLUDE_HDR}
    PATTERN ${PATTERN_INCLUDE_SRC}
    PATTERN ${PATTERN_INCLUDE_LIB}
    PATTERN "*license*"
    PATTERN "*License*"
    PATTERN "samples/*" EXCLUDE
    PATTERN "bin/*.dll"
    PATTERN "bin/external/*")

  if(${NV3DVC_PACKAGE_SDK_MODELS})
    install(DIRECTORY ${ARSDK_ROOT}/bin/models DESTINATION ARSDK/bin)
    install(DIRECTORY ${VFXSDK_ROOT}/bin/models DESTINATION VFXSDK/bin)
    install(DIRECTORY ${AFXSDK_ROOT}/bin/models DESTINATION AFXSDK/bin)
  endif()
endif()

add_subdirectory(External SYSTEM)
add_subdirectory(Internal)

add_subdirectory(src/Core)
if (WIN32)
  add_subdirectory(src/Modules)
  add_subdirectory(src/Applications)
endif()

if(WIN32)
  # Generate .env file to set PATH with added DLL directories.
  file(WRITE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/.env "PATH=%PATH%;${DEPENDENCIES_DLL_DIRS}\n")
  file(APPEND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/.env "GST_PLUGIN_PATH_1_0=%GSTREAMER_1_0_ROOT_MSVC_X86_64%\\lib\\gstreamer-1.0\n")
endif()

option(NV3DVC_BUILD_SAMPLES OFF)
if(NV3DVC_BUILD_SAMPLES)
  add_subdirectory(src/Samples)
endif()

option(NV3DVC_BUILD_API OFF)
if(NV3DVC_BUILD_API)
  add_subdirectory(src/Api)
endif()

option(NV3DVC_PACKAGE_COVERITY OFF)
if(NV3DVC_PACKAGE_COVERITY)
  #Find coverity and install it to package directory
  find_path(COV_BUILD_DIR Win/bin/cov-build.exe
    PATHS
    $ENV{PATH}
    PATH_SUFFIXES
    /../../
  )
  find_path(COV_AUTOMATION_DIR SSLCert/ca-chain.crt
    PATHS
    $ENV{COVERITY_AUTOMATION}
    ${COV_BUILD_DIR}/../tools/scripts/CoverityAutomation
  )
  if ((DEFINED COV_BUILD_DIR) AND NOT ("${COV_BUILD_DIR}" STREQUAL "COV_BUILD_DIR-NOTFOUND"))
    install(DIRECTORY ${COV_BUILD_DIR}/ DESTINATION CP)
  endif()
  if ((DEFINED COV_AUTOMATION_DIR) AND NOT ("${COV_AUTOMATION_DIR}" STREQUAL "COV_AUTOMATION_DIR-NOTFOUND"))
    install(DIRECTORY ${COV_AUTOMATION_DIR}/ DESTINATION CoverityAutomation)
  endif()
endif()

# Remove empty folders generated during install(DIRECTORY ...)
install(CODE "execute_process(COMMAND ${PROJECT_SOURCE_DIR}/cmake/RemoveEmptyFolders.bat WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX})")

# Add target called `docs` to generate documentation.
find_package(Doxygen)
if(DOXYGEN_FOUND)
  # Run Doxygen. We cannot use doxygen_add_docs() because we need to redirect stderr/stdout to file on Windows.
  # MSBuild will scan stderr/stdout and report that the task failed if the word "error" appears anywhere.
  # https://developercommunity.visualstudio.com/t/MSBuild:-error:-output-of-custom-build/10554390
  # After running Doxygen, copy assets into the generated html folder.
  add_custom_target(docs VERBATIM
    COMMAND "${DOXYGEN_EXECUTABLE}" Doxyfile > "${CMAKE_BINARY_DIR}/docs.log" 2>&1
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different doc build/doc/html/doc
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    DEPENDS "${PROJECT_SOURCE_DIR}/Doxyfile"
  )
endif()
