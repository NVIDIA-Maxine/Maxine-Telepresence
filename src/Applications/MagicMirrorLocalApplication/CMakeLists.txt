# Copyright 2024 NVIDIA Corporation.  All rights reserved.
#
# Please refer to the NVIDIA end user license agreement (EULA) associated
# with this source code for terms and conditions that govern your use of
# this software. Any use, reproduction, disclosure, or distribution of
# this software and related documentation outside the terms of the EULA
# is strictly prohibited.

set(EXE_NAME MagicMirrorLocalApplication)

add_executable(${EXE_NAME}
  MagicMirrorLocalApplication.h
  MagicMirrorLocalApplication.cpp
  Main.cpp
  MagicMirrorLocal_scene.json
  run_MagicMirrorLocal.bat
  run_MagicMirrorLocal_Video.bat)

# Link
target_link_libraries(${EXE_NAME}
  nv3dvc_Core
  nv3dvc_CommonModule
  nv3dvc_BehaviorModule
  nv3dvc_CaptureModule
  nv3dvc_ControlModule
  nv3dvc_WindowModule
  nv3dvc_RenderModule
  nv3dvc_TrackingModule
  nv3dvc_TriplaneModule
  nv3dvc_VolumetricEncodingModule
  nv3dvc_GuiModule
  nv3dvc_VideoEffectsModule
)

# Currently, MagicMirrorLocalApplication uses the logger from NvCodec/Utils.
target_link_libraries(${EXE_NAME} nv3dvc_VideoEncDec)

# Debugger setup for Visual Studio.
if(CMAKE_GENERATOR MATCHES "Visual Studio")
  set(PATH_LIST "PATH=%PATH%" ${DEPENDENCIES_DLL_DIRS})
  set_target_properties(${EXE_NAME} PROPERTIES
    FOLDER Applications
    VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${EXE_NAME}>
    VS_DEBUGGER_ENVIRONMENT "${PATH_LIST}"
  )
endif()

# Add MagicMirrorLocalApplication to the install script.
install(TARGETS ${EXE_NAME})

install(DIRECTORY . DESTINATION bin FILES_MATCHING PATTERN "*.json")
install(DIRECTORY . DESTINATION bin FILES_MATCHING PATTERN "*.bat")
