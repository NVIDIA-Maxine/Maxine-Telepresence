# Copyright 2020-2023 NVIDIA Corporation.  All rights reserved.
#
# Please refer to the NVIDIA end user license agreement (EULA) associated
# with this source code for terms and conditions that govern your use of
# this software. Any use, reproduction, disclosure, or distribution of
# this software and related documentation outside the terms of the EULA
# is strictly prohibited.

add_library(nv3dvc_NetworkModule
 NetworkModule.cpp
 NetworkModule.h
 RtpClient.h
 RtpClientSender.cpp
 RtpClientSender.h
 RtpClientReceiver.cpp
 RtpClientReceiver.h
 RtspServer.cpp
 RtspServer.h
 Components/StreamSourceComponent.cpp
 Components/StreamSourceComponent.h
 Components/StreamSinkComponent.cpp
 Components/StreamSinkComponent.h
 Systems/AudioSenderSystem.cpp
 Systems/AudioSenderSystem.h
 Systems/AudioReceiverSystem.cpp
 Systems/AudioReceiverSystem.h
 Systems/SenderSystem.cpp
 Systems/SenderSystem.h
 Systems/ReceiverSystem.cpp
 Systems/ReceiverSystem.h
)

target_link_libraries(nv3dvc_NetworkModule PUBLIC PkgConfig::gstreamer-app)
target_link_libraries(nv3dvc_NetworkModule PUBLIC PkgConfig::gstreamer-rtsp-server)

if(CMAKE_GENERATOR MATCHES "Visual Studio")
  set_target_properties(nv3dvc_NetworkModule PROPERTIES
    FOLDER Modules
  )
endif()
