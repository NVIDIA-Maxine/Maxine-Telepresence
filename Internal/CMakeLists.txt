set(NV_CODEC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/NvCodec)
set(NVCODEC_PUBLIC_INTERFACE_DIR ${NV_CODEC_DIR}/Interface)
set(NVCODEC_UTILS_DIR ${NV_CODEC_DIR}/Utils)
set(NV_ENC_DIR ${NV_CODEC_DIR}/NvEncoder)
set(NV_DEC_DIR ${NV_CODEC_DIR}/NvDecoder)

set(NVWEBCAM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/nvwebcam)

# Find library files.
if(WIN32)
  set(CUVID_LIB ${NV_CODEC_DIR}/Lib/x64/nvcuvid.lib)
  set(NVENCODEAPI_LIB ${NV_CODEC_DIR}/Lib/x64/nvencodeapi.lib)

  set(NVWEBCAM_LIB_RELEASE ${NVWEBCAM_DIR}/lib/Release/NvWebcamLib.lib)
  set(NVWEBCAM_LIB_DEBUG ${NVWEBCAM_DIR}/lib/Debug/NvWebcamLib.lib)
  set(NVWEBCAM_DLL_DIR ${NVWEBCAM_DIR}/bin/)
endif()

# nv3dvc_Internal collects common internal dependencies.
add_library(nv3dvc_Internal INTERFACE)
target_include_directories(nv3dvc_Internal INTERFACE
  ${NVWEBCAM_DIR}
)
target_include_directories(nv3dvc_Internal INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(nv3dvc_Internal INTERFACE nv3dvc_External)
target_link_libraries(nv3dvc_Internal INTERFACE ${CMAKE_DL_LIBS} ${NVTOOLSEXT_LIBS})
if(WIN32)
  target_link_libraries(nv3dvc_Internal INTERFACE debug ${NVWEBCAM_LIB_DEBUG} optimized ${NVWEBCAM_LIB_RELEASE} )
  # Add DLL directory to the list for copying/installing later.
  list(APPEND INTERNAL_DEPENDENCIES_DLL_DIR ${NVWEBCAM_DLL_DIR})
endif()

# nv3dvc_VideoEncDec
add_library(nv3dvc_VideoEncDec
  # NV_ENC_SOURCES
  ${NV_ENC_DIR}/NvEncoder.cpp
  ${NV_ENC_DIR}/NvEncoderCuda.cpp
  ${NV_ENC_DIR}/NvEncoderOutputInVidMemCuda.cpp
  # NV_ENC_HDRS
  ${NV_ENC_DIR}/NvEncoder.h
  ${NV_ENC_DIR}/NvEncoderCuda.h
  ${NV_ENC_DIR}/NvEncoderOutputInVidMemCuda.h
  ${NVCODEC_PUBLIC_INTERFACE_DIR}/nvEncodeAPI.h
  # NV_DEC_HDRS
  ${NV_DEC_DIR}/NvDecoder.h
  ${NVCODEC_PUBLIC_INTERFACE_DIR}/cuviddec.h
  ${NVCODEC_PUBLIC_INTERFACE_DIR}/nvcuvid.h
  # NV_DEC_SOURCES
  ${NV_DEC_DIR}/NvDecoder.cpp
  # Utils
  ${NVCODEC_UTILS_DIR}/NvCodecUtils.h
  ${NVCODEC_UTILS_DIR}/NvEncoderCLIOptions.h
  ${NVCODEC_UTILS_DIR}/Logger.h
)
target_include_directories(nv3dvc_VideoEncDec PUBLIC
  ${NV_CODEC_DIR}
  ${NVCODEC_PUBLIC_INTERFACE_DIR}
)

target_link_libraries(nv3dvc_VideoEncDec
  ${CMAKE_DL_LIBS}
  CUDA::cudart
  ${NVENCODEAPI_LIB}
  ${CUVID_LIB}
)

# DLL dirs for debug environment
set(DEPENDENCIES_DLL_DIRS ${DEPENDENCIES_DLL_DIRS} ${INTERNAL_DEPENDENCIES_DLL_DIR} PARENT_SCOPE)

if(WIN32)
  install(DIRECTORY ${NV_CODEC_DIR}/ DESTINATION NvCodec
    FILES_MATCHING
    PATTERN "*license*"
    PATTERN "*License*"
    PATTERN "*.dll")
  install(DIRECTORY ${NVWEBCAM_DIR}/ DESTINATION NvWebcam
    FILES_MATCHING
    PATTERN "*license*"
    PATTERN "*License*"
    PATTERN "*.dll")
endif(WIN32)