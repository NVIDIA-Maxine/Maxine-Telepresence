include(CMakeDependentOption)

option(NV3DVC_WITH_DIMENCO "Build with dependency and support for Dimenco SR Display" OFF)
option(NV3DVC_WITH_LOOKING_GLASS "Build with dependency and support for Looking Glass Display" OFF)
option(WITH_MOTIONPARALLAX "Build with dependency and support for Tracked Motion Parallax Display" OFF)
option(WITH_OPENXR "Build with dependency and support for OpenXR display" ON)

# Allow CMake to look for <lib>Config.cmake files in here.
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

# Relax minimum CMake version for external code.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

set(UUID_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(UUID_SYSTEM_GENERATOR ON CACHE BOOL "" FORCE)
cmake_dependent_option(UUID_USING_CXX20_SPAN "" ON "CMAKE_CXX_STANDARD GREATER_EQUAL 20" OFF)
add_subdirectory(stduuid EXCLUDE_FROM_ALL)

add_subdirectory(gflags)

add_subdirectory(portaudio)

add_subdirectory(opus)

add_subdirectory(oboe)

add_subdirectory(glm)

add_subdirectory(json)

add_subdirectory(TensorRT)

source_group("External Sources" REGULAR_EXPRESSION ".*")

set(SR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/sr)

# Library to capture external dependencies.
add_library(nv3dvc_External
  imgui/imgui.cpp
  imgui/imgui_draw.cpp
  imgui/imgui_tables.cpp
  imgui/imgui_widgets.cpp
)

add_subdirectory(GLFW)
target_sources(nv3dvc_External PRIVATE imgui/backends/imgui_impl_glfw.cpp)
target_link_libraries(nv3dvc_External glfw)

target_sources(nv3dvc_External PRIVATE imgui/backends/imgui_impl_opengl3.cpp)
add_library(glad glad/src/gl.c)
target_include_directories(glad PUBLIC glad/include)
target_link_libraries(nv3dvc_External glad)
target_link_libraries(nv3dvc_External glm)

target_include_directories(nv3dvc_External PUBLIC SYSTEM
  json
  imgui
  imgui/backends
  entt/single_include
)

target_link_libraries(nv3dvc_External ${CMAKE_DL_LIBS})

find_package(OpenCV CONFIG REQUIRED)
target_link_libraries(nv3dvc_External ${OpenCV_LIBS})

if(WIN32)
  target_link_libraries(nv3dvc_External Shcore.lib)
  list(APPEND EXTERNAL_DEPENDENCIES_DLL_DIR "${OpenCV_LIB_PATH}/../bin")
else()
  # Set link dependencies for nv3dvc_External
  find_library(CUDART_LIB cudart HINTS ${CUDA_TOOLKIT_ROOT_DIR}/lib64)
  target_link_libraries(nv3dvc_External ${CUDA_CUDA_LIBRARY} ${CUDART_LIB})
endif()

if(NV3DVC_WITH_LOOKING_GLASS)
  add_subdirectory(HoloPlay)
  target_link_libraries(nv3dvc_External HoloPlay)
  target_compile_definitions(nv3dvc_External PUBLIC NV3DVC_WITH_LOOKING_GLASS)
  get_target_property(HoloPlay_DLL_DIR HoloPlay DLL_DIRECTORIES)
  list(APPEND EXTERNAL_DEPENDENCIES_DLL_DIR ${HoloPlay_DLL_DIR})
endif()
if(NV3DVC_WITH_DIMENCO)
  message("Dimenco is not supported by default. Requires explicit installation of SR SDK")
  set(SR_LIB_DIR ${SR_DIR}/lib)
  set(SR_DLL_DIR ${SR_DIR}/bin/)
  set(SR_LIBS
    ${SR_LIB_DIR}/SimulatedReality.lib
    ${SR_LIB_DIR}/SimulatedRealityCameras.lib
    ${SR_LIB_DIR}/SimulatedRealityCore.lib
    ${SR_LIB_DIR}/SimulatedRealityDisplays.lib
    ${SR_LIB_DIR}/SimulatedRealityFaceTrackers.lib
    ${SR_LIB_DIR}/SimulatedRealityHandTrackers.lib
    ${SR_LIB_DIR}/SimulatedRealityOpenGL.lib
    ${SR_LIB_DIR}/SimulatedRealityUserModelers.lib)
  target_link_libraries(nv3dvc_External ${SR_LIBS})
  target_include_directories(nv3dvc_External PUBLIC SYSTEM ${SR_DIR}/include)
  target_compile_definitions(nv3dvc_External PUBLIC NV3DVC_WITH_DIMENCO)
  list(APPEND EXTERNAL_DEPENDENCIES_DLL_DIR ${SR_DLL_DIR})
endif()

# DLL dirs for debug environment
set(DEPENDENCIES_DLL_DIRS ${DEPENDENCIES_DLL_DIRS} ${EXTERNAL_DEPENDENCIES_DLL_DIR} PARENT_SCOPE)

if(WIN32)
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/ DESTINATION External
    FILES_MATCHING
    PATTERN "*license*"
    PATTERN "*License*"
    PATTERN "*licenses/*"
    PATTERN "*.dll")
endif(WIN32)
