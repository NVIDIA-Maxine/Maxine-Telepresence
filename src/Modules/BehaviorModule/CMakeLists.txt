# Copyright 2020-2023 NVIDIA Corporation.  All rights reserved.
#
# Please refer to the NVIDIA end user license agreement (EULA) associated
# with this source code for terms and conditions that govern your use of
# this software. Any use, reproduction, disclosure, or distribution of
# this software and related documentation outside the terms of the EULA
# is strictly prohibited.

add_library(nv3dvc_BehaviorModule
 BehaviorModule.h
 Components/BehaviorComponent.h
 Systems/BehaviorSystem.h
 BehaviorModule.cpp
 Components/BehaviorComponent.cpp
 Systems/BehaviorSystem.cpp
)

if(CMAKE_GENERATOR MATCHES "Visual Studio")
  set_target_properties(nv3dvc_BehaviorModule PROPERTIES
    FOLDER Modules
  )
endif()
