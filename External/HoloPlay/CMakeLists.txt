project(HoloPlay)

find_package(Git REQUIRED)

# Download HoloPlay from GitHub.
include(FetchContent)
FetchContent_Declare(
  HoloPlay
  GIT_REPOSITORY      https://github.com/Looking-Glass/LookingGlassCoreSDK
  GIT_TAG             v0.2.0
  UPDATE_DISCONNECTED 1
)

message(STATUS "Populating HoloPlay")
FetchContent_MakeAvailable(HoloPlay)

FetchContent_GetProperties(HoloPlay SOURCE_DIR HoloPlay_ROOT_DIR)

set(HoloPlay_DLL_DIR ${HoloPlay_ROOT_DIR}/HoloPlayCore/dylib/Win64)
set(HoloPlay_DLL ${HoloPlay_DLL_DIR}/HoloPlayCore.dll)
set(HoloPlay_LIB ${HoloPlay_DLL_DIR}/HoloPlayCore.lib)
set(HoloPlay_LICENSE ${HoloPlay_ROOT_DIR}/LICENSE.txt)
set(HoloPlay_INCLUDE_DIR ${HoloPlay_ROOT_DIR}/HoloPlayCore/include)

add_library(HoloPlay INTERFACE
  ${HoloPlay_INCLUDE_DIR}/HoloPlayCore.h
  ${HoloPlay_INCLUDE_DIR}/HoloPlayShaders.h
  ${HoloPlay_INCLUDE_DIR}/HoloPlayShadersOpen.h
  ${HoloPlay_INCLUDE_DIR}/libHoloPlayCore.h
)
target_include_directories(HoloPlay INTERFACE ${HoloPlay_INCLUDE_DIR})
target_link_libraries(HoloPlay INTERFACE ${HoloPlay_LIB})

set_target_properties(HoloPlay PROPERTIES FOLDER External)

set_target_properties(HoloPlay PROPERTIES DLL_DIRECTORIES ${HoloPlay_DLL_DIR})

install(FILES ${HoloPlay_DLL} DESTINATION External/HoloPlay/bin)
install(FILES ${HoloPlay_LICENSE} DESTINATION External/HoloPlay/)
