# Copyright 2020-2023 NVIDIA Corporation.  All rights reserved.
#
# Please refer to the NVIDIA end user license agreement (EULA) associated
# with this source code for terms and conditions that govern your use of
# this software. Any use, reproduction, disclosure, or distribution of
# this software and related documentation outside the terms of the EULA
# is strictly prohibited.

add_library(nv3dvc_CommonModule
 CommonModule.h
 CommonModule.cpp
 CameraCalibration.h
 OneEuroFilterProperties.h
 OneEuroFilterProperties.cpp
 Components/CameraCalibrationComponent.h
 Components/CameraCalibrationComponent.cpp
 Components/CameraComponent.h
 Components/TransformComponent.h
 Components/VideoFrameComponent.h
 Components/DataBufferComponent.h
 Components/CallbackComponent.h
 Components/CameraComponent.cpp
 Components/TransformComponent.cpp
 Components/VideoFrameComponent.cpp
 Components/DataBufferComponent.cpp
 Components/CallbackComponent.cpp
)
target_link_libraries(nv3dvc_CommonModule nv3dvc_Core nv3dvc_Internal CUDA::cudart CUDA::cuda_driver)
target_include_directories(nv3dvc_CommonModule PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_link_libraries(nv3dvc_CommonModule stduuid)

if(CMAKE_GENERATOR MATCHES "Visual Studio")
  set_target_properties(nv3dvc_CommonModule PROPERTIES
    FOLDER Modules
  )
endif()
