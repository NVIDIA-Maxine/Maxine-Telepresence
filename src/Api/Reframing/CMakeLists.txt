# SPDX-FileCopyrightText: Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: LicenseRef-NvidiaProprietary
#
# NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
# property and proprietary rights in and to this material, related
# documentation and any modifications thereto. Any use, reproduction,
# disclosure or distribution of this material and related documentation
# without an express license agreement from NVIDIA CORPORATION or
# its affiliates is strictly prohibited.

add_library(Reframing Reframing.h ReframingDefs.h Reframing.cpp)
target_link_libraries(Reframing
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
if(CMAKE_GENERATOR MATCHES "Visual Studio")
  set(PATH_LIST "PATH=%PATH%" ${DEPENDENCIES_DLL_DIR})
  set_target_properties(Reframing PROPERTIES
    FOLDER Api
    VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:Reframing>
    VS_DEBUGGER_ENVIRONMENT "${PATH_LIST}"
  )
endif()
install(TARGETS Reframing)
