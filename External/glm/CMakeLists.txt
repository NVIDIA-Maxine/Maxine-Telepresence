project(glm)

# Download glm from GitHub.
include(FetchContent)
FetchContent_Declare(
  glm
  GIT_REPOSITORY      https://github.com/g-truc/glm
  GIT_TAG             1.0.0
  GIT_SHALLOW         TRUE
  UPDATE_DISCONNECTED TRUE
  EXCLUDE_FROM_ALL # don't build or install targets by default
)

message(STATUS "Populating glm")
FetchContent_MakeAvailable(glm)

FetchContent_GetProperties(glm SOURCE_DIR glm_ROOT_DIR)

set(glm_LICENSE ${glm_ROOT_DIR}/copying.txt)

set_target_properties(glm PROPERTIES FOLDER External)
target_compile_definitions(glm PUBLIC GLM_ENABLE_EXPERIMENTAL)

install(FILES ${glm_LICENSE} DESTINATION External/glm/)
