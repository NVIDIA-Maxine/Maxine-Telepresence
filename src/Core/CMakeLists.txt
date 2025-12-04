# Copyright 2020-2023 NVIDIA Corporation.  All rights reserved.
#
# Please refer to the NVIDIA end user license agreement (EULA) associated
# with this source code for terms and conditions that govern your use of
# this software. Any use, reproduction, disclosure, or distribution of
# this software and related documentation outside the terms of the EULA
# is strictly prohibited.

project(Core)

set(CORE_HDRS
 Application/Application.h
 Application/Callbacks.h
 Application/InputMapping.h
 Application/Inputs.h
 Application/Window.h
 Application/EventBroadcaster.h
 Engine/Engine.h
 Engine/Module.h
 EntityComponentSystem/Entity.h
 EntityComponentSystem/Component.h
 EntityComponentSystem/System.h
 Error.h
 Events/Event.h
 Events/FileEvent.h
 Events/KeyEvent.h
 Events/MouseEvent.h
 Events/WindowEvent.h
 Graphics/GraphicsApi.h
 Serialization/CustomJsonTypes.h
 Serialization/CustomJsonTypesGlm.h
 Serialization/PropertyDescription.h
 Serialization/Serialization.h
 Properties/Property.h
 Rendering/CubeMapModel.h
 Rendering/CubeMapTexture.h
 Rendering/MeshModel.h
 Rendering/Renderer.h
 Rendering/Display/Display.h
 Rendering/Display/BaseDisplay.h
 Rendering/Display/VirtualDisplay.h
 Rendering/Display/SimpleDisplay.h
 Rendering/Display/DimencoDisplay.h
 Rendering/Display/LookingGlassDisplay.h
 Rendering/Shader.h
 Util/Logger.h
 Util/OneEuroFilter.h
 Util/TemporalLinearExtrapolator.h
 Util/Timeable.h
 Util/nvCVLogger.h
 Util/TypeMap.h
 Util/Types.h
)

set(CORE_SOURCES
 Application/Application.cpp
 Application/Callbacks.cpp
 Application/Inputs.cpp
 Application/Window.cpp
 Engine/Engine.cpp
 Engine/Module.cpp
 Error.cpp
 Events/Event.cpp
 Graphics/GraphicsApi.cpp
 EntityComponentSystem/Entity.cpp
 Serialization/PropertyDescription.cpp
 Serialization/Serialization.cpp
 Rendering/CubeMapModel.cpp
 Rendering/CubeMapTexture.cpp
 Rendering/MeshModel.cpp
 Rendering/Renderer.cpp
 Rendering/Display/BaseDisplay.cpp
 Rendering/Display/VirtualDisplay.cpp
 Rendering/Display/SimpleDisplay.cpp
 Rendering/Display/DimencoDisplay.cpp
 Rendering/Display/LookingGlassDisplay.cpp
 Rendering/Shader.cpp
 Util/Logger.cpp
 Util/nvCVLogger.cpp
 Util/Timeable.cpp
)

set(CORE_SHADER_SOURCES
 Rendering/Shaders/Blit.vert
 Rendering/Shaders/Blit.frag
 Rendering/Shaders/Composite.frag
 Rendering/Shaders/CubeMap.vert
 Rendering/Shaders/CubeMap.frag
 Rendering/Shaders/Downsample.frag
 Rendering/Shaders/LookingglassInterlacer.vert
 Rendering/Shaders/LookingglassInterlacer.frag
 Rendering/Shaders/UnlitMesh.vert
 Rendering/Shaders/UnlitMesh.frag
 Rendering/Shaders/Vignette.frag
)

source_group("Shader Sources" REGULAR_EXPRESSION "^.*\.(glsl|vert|frag)$")

add_library(nv3dvc_Core ${CORE_HDRS} ${CORE_SOURCES} ${CORE_SHADER_SOURCES})

target_link_libraries(nv3dvc_Core PRIVATE CUDA::cudart)
target_link_libraries(nv3dvc_Core PRIVATE stduuid)
target_link_libraries(nv3dvc_Core PUBLIC nlohmann_json::nlohmann_json)
target_link_libraries(nv3dvc_Core PUBLIC nv3dvc_Internal)
target_link_libraries(nv3dvc_Core PUBLIC NvCVVolume)
target_link_libraries(nv3dvc_Core PUBLIC nvARPose)
target_link_libraries(nv3dvc_Core PUBLIC nvARVolumetricRendering)
target_link_libraries(nv3dvc_Core PUBLIC NVCVImage)

cmake_path(SET NV3DVC_AR_SDK_DIR NORMALIZE "\"${ARSDK_ROOT}\"")
cmake_path(SET NV3DVC_AFX_SDK_DIR NORMALIZE "\"${AFXSDK_ROOT}\"")
cmake_path(SET NV3DVC_VFX_SDK_DIR NORMALIZE "\"${VFXSDK_ROOT}\"")
target_compile_definitions(nv3dvc_Core PRIVATE NV3DVC_AR_SDK_DIR=${NV3DVC_AR_SDK_DIR})
target_compile_definitions(nv3dvc_Core PRIVATE NV3DVC_AFX_SDK_DIR=${NV3DVC_AFX_SDK_DIR})
target_compile_definitions(nv3dvc_Core PRIVATE NV3DVC_VFX_SDK_DIR=${NV3DVC_VFX_SDK_DIR})

target_include_directories(nv3dvc_Core PUBLIC ${CUDA_INCLUDE_DIRS})
target_include_directories(nv3dvc_Core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
