# SPDX-FileCopyrightText: Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: LicenseRef-NvidiaProprietary
#
# NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
# property and proprietary rights in and to this material, related
# documentation and any modifications thereto. Any use, reproduction,
# disclosure or distribution of this material and related documentation
# without an express license agreement from NVIDIA CORPORATION or
# its affiliates is strictly prohibited.

add_executable(MyPlugin MyPlugin.cpp)
target_link_libraries(MyPlugin
  nv3dvc_Core
  nv3dvc_BehaviorModule
  nv3dvc_WindowModule
  nv3dvc_RenderModule
  nv3dvc_TriplaneModule
  nv3dvc_GuiModule
  MyModule
)
if(CMAKE_GENERATOR MATCHES "Visual Studio")
  set(PATH_LIST "PATH=%PATH%" ${DEPENDENCIES_DLL_DIR})
  set_target_properties(MyPlugin PROPERTIES
    FOLDER Samples
    VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:MyPlugin>
    VS_DEBUGGER_ENVIRONMENT "${PATH_LIST}"
  )
endif()
install(TARGETS MyPlugin)
