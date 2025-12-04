project(oboe-resampler)

find_package(Git REQUIRED)

# Download oboe from GitHub.
include(FetchContent)
FetchContent_Declare(
  oboe
  GIT_REPOSITORY      https://github.com/google/oboe
  GIT_TAG             1.9.0
  GIT_SHALLOW         TRUE
  PATCH_COMMAND       "${GIT_EXECUTABLE}" apply "${CMAKE_CURRENT_SOURCE_DIR}/fix-includes.patch"
  UPDATE_DISCONNECTED TRUE
  SOURCE_SUBDIR       fakedir # No effect except to avoid add_subdirectory
)

message(STATUS "Populating oboe")
FetchContent_MakeAvailable(oboe)

FetchContent_GetProperties(oboe SOURCE_DIR OBOE_SRC_DIR)

# Build only the resampler code from oboe into a library.
set(OBOE_RESAMPLER_DIR ${OBOE_SRC_DIR}/src/flowgraph/resampler)

add_library(oboe_resampler EXCLUDE_FROM_ALL
  ${OBOE_RESAMPLER_DIR}/HyperbolicCosineWindow.h
  ${OBOE_RESAMPLER_DIR}/IntegerRatio.h
  ${OBOE_RESAMPLER_DIR}/KaiserWindow.h
  ${OBOE_RESAMPLER_DIR}/LinearResampler.h
  ${OBOE_RESAMPLER_DIR}/MultiChannelResampler.h
  ${OBOE_RESAMPLER_DIR}/PolyphaseResampler.h
  ${OBOE_RESAMPLER_DIR}/PolyphaseResamplerMono.h
  ${OBOE_RESAMPLER_DIR}/PolyphaseResamplerStereo.h
  ${OBOE_RESAMPLER_DIR}/ResamplerDefinitions.h
  ${OBOE_RESAMPLER_DIR}/SincResampler.h
  ${OBOE_RESAMPLER_DIR}/SincResamplerStereo.h

  ${OBOE_RESAMPLER_DIR}/IntegerRatio.cpp
  ${OBOE_RESAMPLER_DIR}/LinearResampler.cpp
  ${OBOE_RESAMPLER_DIR}/MultiChannelResampler.cpp
  ${OBOE_RESAMPLER_DIR}/PolyphaseResampler.cpp
  ${OBOE_RESAMPLER_DIR}/PolyphaseResamplerMono.cpp
  ${OBOE_RESAMPLER_DIR}/PolyphaseResamplerStereo.cpp
  ${OBOE_RESAMPLER_DIR}/SincResampler.cpp
  ${OBOE_RESAMPLER_DIR}/SincResamplerStereo.cpp
)
target_include_directories(oboe_resampler PUBLIC ${OBOE_RESAMPLER_DIR})
target_compile_definitions(oboe_resampler PUBLIC RESAMPLER_OUTER_NAMESPACE=oboe)
target_compile_definitions(oboe_resampler PRIVATE _USE_MATH_DEFINES)

set_target_properties(oboe_resampler PROPERTIES FOLDER External)

install(FILES ${OBOE_SRC_DIR}/LICENSE DESTINATION External/oboe)
