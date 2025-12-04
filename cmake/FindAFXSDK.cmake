# Copyright 2025 NVIDIA Corporation.  All rights reserved.
#
# Please refer to the NVIDIA end user license agreement (EULA) associated
# with this source code for terms and conditions that govern your use of
# this software. Any use, reproduction, disclosure, or distribution of
# this software and related documentation outside the terms of the EULA
# is strictly prohibited.

# Finds the NVAudioEffects library and its features
#
# And the following targets:
#   NVAudioEffects          - The main AFXSDK library
#   nvAFX<FeatureName>      - For every feature found in the lib directory

if(NOT TARGET NVAudioEffects)
  # Allow the user to specify where to find AFXSDK
  set(AFXSDK_ROOT "" CACHE PATH "Root directory of AFX SDK installation")

  # Find the dynamic library directory
  find_path(NVAudioEffects_LIBRARY_DIR
    NAMES NVAudioEffects.dll
    PATHS
    ${AFXSDK_ROOT}/bin # If explicitly set
    ${CMAKE_INSTALL_PREFIX}/bin # If installed
    ${CMAKE_CURRENT_SOURCE_DIR}/../bin # If local
    ${AFXSDK_ROOT}/lib # Linux, if explicitly set
    ${CMAKE_INSTALL_PREFIX}/lib # Linux, if installed
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib # Linux, if local
    NO_DEFAULT_PATH
  )

  # Find the model directory
  find_path(NVAudioEffects_MODEL_DIR
    NAMES models
    PATHS
    ${AFXSDK_ROOT}/bin # If explicitly set
    ${CMAKE_INSTALL_PREFIX}/bin # If installed
    ${CMAKE_CURRENT_SOURCE_DIR}/../bin # If local
    ${AFXSDK_ROOT}/lib # Linux, if explicitly set
    ${CMAKE_INSTALL_PREFIX}/lib # Linux, if installed
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib # Linux, if local
    NO_DEFAULT_PATH
  )
  set(NVAudioEffects_MODEL_DIR ${NVAudioEffects_MODEL_DIR}/models)

  if("${AFXSDK_ROOT}" STREQUAL "" AND DEFINED NVAudioEffects_LIBRARY_DIR AND EXISTS ${NVAudioEffects_LIBRARY_DIR})
    set(AFXSDK_ROOT "${NVAudioEffects_LIBRARY_DIR}/../")
  endif()

  # Find the import library file for NVAudioEffects
  find_library(NVAudioEffects_IMPORT_LIBRARY
    NAMES NVAudioEffects.lib
    PATHS
    ${AFXSDK_ROOT}/lib
    ${AFXSDK_ROOT}/bin
    NO_DEFAULT_PATH
  )

  # Find the include directory
  find_path(NVAudioEffects_INCLUDE_DIR
    NAMES NVAudioEffects.h
    PATHS
    ${AFXSDK_ROOT}/include
    ${AFXSDK_ROOT}/nvafx/include
    NO_DEFAULT_PATH
  )

  # Set version from header if available
  if(NVAudioEffects_INCLUDE_DIR)
    # Find all version*.h files
    file(GLOB VERSION_FILES "${AFXSDK_ROOT}/version*.h")
    if(VERSION_FILES)
      # Sort to ensure consistent order, and read the first one
      list(SORT VERSION_FILES)
      file(READ "${VERSION_FILES}" VERSION_HEADER)

      string(REGEX MATCH "NVIDIA_AUDIOFX_SDK_VERSION_MAJOR[ \t]+([0-9]+)" _ "${VERSION_HEADER}")
      set(NVAudioEffects_VERSION_VERSION_MAJOR "${CMAKE_MATCH_1}")
      string(REGEX MATCH "NVIDIA_AUDIOFX_SDK_VERSION_MINOR[ \t]+([0-9]+)" _ "${VERSION_HEADER}")
      set(NVAudioEffects_VERSION_VERSION_MINOR "${CMAKE_MATCH_1}")
      string(REGEX MATCH "NVIDIA_AUDIOFX_SDK_VERSION_RELEASE[ \t]+([0-9]+)" _ "${VERSION_HEADER}")
      set(NVAudioEffects_VERSION_VERSION_RELEASE "${CMAKE_MATCH_1}")
      string(REGEX MATCH "NVIDIA_AUDIOFX_SDK_VERSION_BUILD[ \t]+([0-9]+)" _ "${VERSION_HEADER}")
      set(NVAudioEffects_VERSION_VERSION_BUILD "${CMAKE_MATCH_1}")
      set(NVAudioEffects_VERSION_STRING "${NVAudioEffects_VERSION_VERSION_MAJOR}.${NVAudioEffects_VERSION_VERSION_MINOR}.${NVAudioEffects_VERSION_VERSION_RELEASE}.${NVAudioEffects_VERSION_VERSION_BUILD}")
    endif()
  endif()

  if (NOT NVAudioEffects_VERSION_STRING)
    message(WARNING "Unable to deduce SDK version")
    set(NVAudioEffects_VERSION_STRING "")
  endif()

  # Create targets
  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(AFXSDK
    FAIL_MESSAGE
    "AFXSDK not found. Please set AFXSDK_ROOT to the root directory of the AFX SDK installation."
    REQUIRED_VARS
    AFXSDK_ROOT
    NVAudioEffects_IMPORT_LIBRARY
    NVAudioEffects_INCLUDE_DIR
    VERSION_VAR NVAudioEffects_VERSION_STRING
  )

  if(AFXSDK_FOUND)
    # Main library target
    if(NOT TARGET NVAudioEffects)
      add_library(NVAudioEffects SHARED IMPORTED)
    endif()
    set_target_properties(NVAudioEffects PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${NVAudioEffects_INCLUDE_DIR}"
      IMPORTED_IMPLIB "${NVAudioEffects_IMPORT_LIBRARY}"
      IMPORTED_LOCATION "${NVAudioEffects_IMPORT_LIBRARY}"
      DYNAMIC_LIBRARY_DIR "${NVAudioEffects_LIBRARY_DIR}"
      MODEL_DIR "${NVAudioEffects_MODEL_DIR}"
    )

    # Dynamically discover feature libraries
    file(GLOB FEATURE_FOLDERS LIST_DIRECTORIES true "${AFXSDK_ROOT}/features/*")
    list(FILTER FEATURE_FOLDERS INCLUDE REGEX "${AFXSDK_ROOT}/features/[^.]+$")

    foreach(FEATURE_FOLDER ${FEATURE_FOLDERS})
      file(GLOB HEADERS "${FEATURE_FOLDER}/include/*.h")
      foreach(HEADER ${HEADERS})
        get_filename_component(HEADER_NAME ${HEADER} NAME_WE)
        get_filename_component(FEATURE_FOLDER_NAME ${FEATURE_FOLDER} NAME)
        string(TOLOWER "${HEADER_NAME}" HEADER_NAME_LOWER)
        if("${HEADER_NAME_LOWER}" STREQUAL "${FEATURE_FOLDER_NAME}")
          set(FEATURE_NAME ${HEADER_NAME})
        endif()
      endforeach()
      if(NOT FEATURE_NAME)
        message(FATAL_ERROR "Invalid feature. Header not matching feature name: ${FEATURE_FOLDER}")
      endif()
      # Create interface library for the feature
      add_library(${FEATURE_NAME} INTERFACE)

      # Read the version string from the header file and set the version property
      file(READ "${FEATURE_FOLDER}/include/${FEATURE_NAME}.h" FEATURE_HEADER)
      string(TOUPPER "${FEATURE_NAME}" FEATURE_NAME_UPPER)
      string(REGEX MATCH "#define[ \t]+NVAFX_[A-Z]+_VERSION_STRING[ \t]+\"([^\"]+)\"" VERSION_MATCH "${FEATURE_HEADER}")
      set(FEATURE_VERSION_STRING "${CMAKE_MATCH_1}")
      set_target_properties(${FEATURE_NAME} PROPERTIES
        INTERFACE_VERSION ${FEATURE_VERSION_STRING}
      )
      
      target_include_directories(${FEATURE_NAME} INTERFACE ${FEATURE_FOLDER}/include)
      if(UNIX)
        set_target_properties(${FEATURE_NAME} PROPERTIES INTERFACE_DYNAMIC_LIBRARY_DIRECTORY ${FEATURE_FOLDER}/lib)
      else()
        set_target_properties(${FEATURE_NAME} PROPERTIES INTERFACE_DYNAMIC_LIBRARY_DIRECTORY ${FEATURE_FOLDER}/bin)
      endif()
    endforeach()
  endif()
endif()
