# Copyright 2020-2023 NVIDIA Corporation.  All rights reserved.
#
# Please refer to the NVIDIA end user license agreement (EULA) associated
# with this source code for terms and conditions that govern your use of
# this software. Any use, reproduction, disclosure, or distribution of
# this software and related documentation outside the terms of the EULA
# is strictly prohibited.

add_library(nv3dvc_GuiModule
 GuiModule.cpp
 GuiModule.h
 ImGuiBackend.cpp
 Systems/GuiSystem.cpp
 Systems/GuiSystem.h
 Panels/MainMenuBar.cpp
 Panels/MainMenuBar.h
 Panels/ApplicationConfigPanel.cpp
 Panels/ApplicationConfigPanel.h
 Panels/EntityPropertiesPanel.cpp
 Panels/EntityPropertiesPanel.h
 Panels/RenderProperties.cpp
 Panels/RenderProperties.h
 Panels/SceneHierarchyPanel.cpp
 Panels/SceneHierarchyPanel.h
 Panels/VideoPanel.cpp
 Panels/VideoPanel.h
 Panels/UserPromptsPanel.cpp
 Panels/UserPromptsPanel.h
 Panels/WindowDisplayOptions.h
)

target_link_libraries(nv3dvc_GuiModule PRIVATE glad)

if(CMAKE_GENERATOR MATCHES "Visual Studio")
  set_target_properties(nv3dvc_GuiModule PROPERTIES
    FOLDER Modules
  )
endif()
