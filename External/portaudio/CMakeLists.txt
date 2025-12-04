# Download PortAudio from GitHub.
include(FetchContent)
FetchContent_Declare(
  portaudio
  GIT_REPOSITORY      https://github.com/PortAudio/portaudio
  GIT_TAG             v19.7.0
  GIT_SHALLOW         TRUE
  UPDATE_DISCONNECTED TRUE
  EXCLUDE_FROM_ALL # don't build or install targets by default
)
set(PA_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(PA_DISABLE_INSTALL ON CACHE BOOL "" FORCE)

message(STATUS "Populating PortAudio")
FetchContent_MakeAvailable(portaudio)

FetchContent_GetProperties(portaudio SOURCE_DIR portaudio_SRC_DIR)

set_target_properties(portaudio_static PROPERTIES FOLDER External)

install(FILES ${portaudio_SRC_DIR}/LICENSE.txt DESTINATION External/portaudio)
