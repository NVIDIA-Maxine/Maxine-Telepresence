include(FetchContent)
# Download GLFW source and add it to the build.
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.4
  GIT_SHALLOW         TRUE
  UPDATE_DISCONNECTED TRUE
  EXCLUDE_FROM_ALL # don't build or install targets by default
)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)

message(STATUS "Populating glfw")
FetchContent_MakeAvailable(glfw)

FetchContent_GetProperties(glfw SOURCE_DIR GLFW_SRC_DIR)

set_target_properties(glfw PROPERTIES FOLDER External)

install(FILES ${GLFW_SRC_DIR}/LICENSE.md DESTINATION External/glfw)
