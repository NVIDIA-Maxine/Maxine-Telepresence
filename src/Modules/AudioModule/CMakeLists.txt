# Copyright 2020-2023 NVIDIA Corporation.  All rights reserved.
#
# Please refer to the NVIDIA end user license agreement (EULA) associated
# with this source code for terms and conditions that govern your use of
# this software. Any use, reproduction, disclosure, or distribution of
# this software and related documentation outside the terms of the EULA
# is strictly prohibited.

# Find GStreamer (on Windows, GStreamer bundles pkg-config.exe).
find_package(PkgConfig REQUIRED)
pkg_search_module(gstreamer-app REQUIRED IMPORTED_TARGET gstreamer-app-1.0)
pkg_search_module(gstreamer-rtsp-server REQUIRED IMPORTED_TARGET gstreamer-rtsp-server-1.0)

add_library(nv3dvc_AudioModule
 AudioModule.h
 AudioRingBuffer.h
 AudioUtils.h
 AcousticEchoCanceller.h
 Components/AudioInputComponent.h
 Components/AudioOutputComponent.h
 Components/AudioSinkComponent.h
 Components/AudioSourceComponent.h
 Systems/AudioFeedbackSystem.h
 Systems/AudioInputSystem.h
 Systems/AudioOutputSystem.h
 AudioModule.cpp
 AudioRingBuffer.cpp
 AudioUtils.cpp
 AcousticEchoCanceller.cpp
 Components/AudioInputComponent.cpp
 Components/AudioOutputComponent.cpp
 Components/AudioSinkComponent.cpp
 Components/AudioSourceComponent.cpp
 Systems/AudioFeedbackSystem.cpp
 Systems/AudioInputSystem.cpp
 Systems/AudioOutputSystem.cpp
)

target_link_libraries(nv3dvc_AudioModule PRIVATE oboe_resampler portaudio_static NVAudioEffects nvAFXAec)
target_include_directories(nv3dvc_AudioModule PRIVATE
  $<TARGET_PROPERTY:portaudio_static,INCLUDE_DIRECTORIES> # Needed for pa_ringbuffer.h
)

if(CMAKE_GENERATOR MATCHES "Visual Studio")
  set_target_properties(nv3dvc_AudioModule PROPERTIES
    FOLDER Modules
  )
endif()
