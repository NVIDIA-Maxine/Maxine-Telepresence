# Copyright 2020-2023 NVIDIA Corporation.  All rights reserved.
#
# Please refer to the NVIDIA end user license agreement (EULA) associated
# with this source code for terms and conditions that govern your use of
# this software. Any use, reproduction, disclosure, or distribution of
# this software and related documentation outside the terms of the EULA
# is strictly prohibited.

add_library(nv3dvc_CaptureModule
 CaptureModule.h
 CaptureModule.cpp
 Components/WebCameraComponent.h
 Components/WebCameraComponent.cpp
 Components/RecordingBehavior.cpp
 Components/RecordingBehavior.h
 Systems/CameraCaptureSystem.cpp
 Systems/CameraCaptureSystem.h
 CameraDescriptor.h
 CaptureDevice/CaptureDevice.cpp
 CaptureDevice/CaptureDevice.h
 CaptureDevice/MultimediaFile.cpp
 CaptureDevice/MultimediaFile.h
 CaptureDevice/NvWebCamera.cpp
 CaptureDevice/NvWebCamera.h
 CaptureDevice/nvCVOpenCV.h
 CaptureDevice/OpenCvCamera.cpp
 CaptureDevice/OpenCvCamera.h
 CaptureDevice/OpenCvVideo.cpp
 CaptureDevice/OpenCvVideo.h
)

target_link_libraries(nv3dvc_CaptureModule PUBLIC PkgConfig::gstreamer-app)

if(CMAKE_GENERATOR MATCHES "Visual Studio")
  set_target_properties(nv3dvc_CaptureModule PROPERTIES
    FOLDER Modules
  )
endif()
