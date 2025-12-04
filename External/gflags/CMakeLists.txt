project(gflags)

# Download glm from GitHub.
include(FetchContent)
FetchContent_Declare(
  gflags
  GIT_REPOSITORY      https://github.com/gflags/gflags
  GIT_TAG             v2.2.2
  GIT_SHALLOW         TRUE
  UPDATE_DISCONNECTED TRUE
  EXCLUDE_FROM_ALL # don't build or install targets by default
)

message(STATUS "Populating gflags")
FetchContent_MakeAvailable(gflags)

FetchContent_GetProperties(gflags SOURCE_DIR gflags_SRC_DIR)

install(FILES ${gflags_SRC_DIR}/COPYING.txt DESTINATION External/gflags)
