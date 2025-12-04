# Copyright 2020-2023 NVIDIA Corporation.  All rights reserved.
#
# Please refer to the NVIDIA end user license agreement (EULA) associated
# with this source code for terms and conditions that govern your use of
# this software. Any use, reproduction, disclosure, or distribution of
# this software and related documentation outside the terms of the EULA
# is strictly prohibited.

project(Modules)

# Find GStreamer (on Windows, GStreamer bundles pkg-config.exe).
find_package(PkgConfig REQUIRED)
pkg_search_module(gstreamer-app REQUIRED IMPORTED_TARGET gstreamer-app-1.0)
pkg_search_module(gstreamer-rtsp-server REQUIRED IMPORTED_TARGET gstreamer-rtsp-server-1.0)

add_subdirectory(AudioModule)
add_subdirectory(BehaviorModule)
add_subdirectory(CaptureModule)
add_subdirectory(CodecModule)
add_subdirectory(CommonModule)
add_subdirectory(ControlModule)
add_subdirectory(GuiModule)
add_subdirectory(NetworkModule)
add_subdirectory(RenderModule)
add_subdirectory(TrackingModule)
add_subdirectory(TriplaneModule)
add_subdirectory(VideoEffectsModule)
add_subdirectory(VolumetricEncodingModule)
add_subdirectory(WindowModule)

# Module interdependencies
target_link_libraries(nv3dvc_AudioModule PUBLIC nv3dvc_CommonModule)
target_link_libraries(nv3dvc_BehaviorModule PUBLIC nv3dvc_CommonModule)
target_link_libraries(nv3dvc_CaptureModule PUBLIC nv3dvc_CommonModule)
target_link_libraries(nv3dvc_CodecModule PUBLIC nv3dvc_CommonModule)
target_link_libraries(nv3dvc_ControlModule PUBLIC nv3dvc_CommonModule)
target_link_libraries(nv3dvc_GuiModule PUBLIC nv3dvc_CommonModule)
target_link_libraries(nv3dvc_NetworkModule PUBLIC nv3dvc_CommonModule)
target_link_libraries(nv3dvc_RenderModule PUBLIC nv3dvc_CommonModule)
target_link_libraries(nv3dvc_TrackingModule PUBLIC nv3dvc_CommonModule)
target_link_libraries(nv3dvc_TriplaneModule PUBLIC nv3dvc_CommonModule)
target_link_libraries(nv3dvc_VideoEffectsModule PUBLIC nv3dvc_CommonModule)
target_link_libraries(nv3dvc_VolumetricEncodingModule PUBLIC nv3dvc_CommonModule)
target_link_libraries(nv3dvc_WindowModule PUBLIC nv3dvc_CommonModule)
