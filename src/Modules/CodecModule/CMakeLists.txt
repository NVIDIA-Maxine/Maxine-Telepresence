# Copyright 2020-2023 NVIDIA Corporation.  All rights reserved.
#
# Please refer to the NVIDIA end user license agreement (EULA) associated
# with this source code for terms and conditions that govern your use of
# this software. Any use, reproduction, disclosure, or distribution of
# this software and related documentation outside the terms of the EULA
# is strictly prohibited.

add_library(nv3dvc_CodecModule
 CodecModule.cpp
 CodecModule.h
 VideoDecoder.cpp
 VideoDecoder.h
 VideoEncoder.cpp
 VideoEncoder.h
 Components/AudioDecoderComponent.cpp
 Components/AudioDecoderComponent.h
 Components/AudioEncoderComponent.cpp
 Components/AudioEncoderComponent.h
 Components/FrameDecoderComponent.cpp
 Components/FrameDecoderComponent.h
 Components/FrameEncoderComponent.cpp
 Components/FrameEncoderComponent.h
 Systems/DecoderSystem.cpp
 Systems/DecoderSystem.h
 Systems/EncoderSystem.cpp
 Systems/EncoderSystem.h
)

target_link_libraries(nv3dvc_CodecModule PUBLIC nv3dvc_Core nv3dvc_Internal CUDA::cudart)
target_link_libraries(nv3dvc_CodecModule PUBLIC nv3dvc_VideoEncDec)
target_link_libraries(nv3dvc_CodecModule PRIVATE Opus::opus)
target_link_libraries(nv3dvc_CodecModule PUBLIC nvARPose)
target_link_libraries(nv3dvc_CodecModule PUBLIC stduuid)

if(CMAKE_GENERATOR MATCHES "Visual Studio")
  set_target_properties(nv3dvc_CodecModule PROPERTIES
    FOLDER Modules
  )
endif()
