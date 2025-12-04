# Download Opus from GitHub.
include(FetchContent)
FetchContent_Declare(
  opus
  GIT_REPOSITORY      https://github.com/xiph/opus
  GIT_TAG             v1.5.2
  GIT_SHALLOW         TRUE
  UPDATE_DISCONNECTED TRUE
  EXCLUDE_FROM_ALL # don't build or install targets by default
)

message(STATUS "Populating opus")
FetchContent_MakeAvailable(opus)

FetchContent_GetProperties(opus SOURCE_DIR opus_SRC_DIR)

set_target_properties(opus PROPERTIES FOLDER External)

install(FILES ${opus_SRC_DIR}/COPYING DESTINATION External/opus)
install(FILES ${opus_SRC_DIR}/LICENSE_PLEASE_READ.txt DESTINATION External/opus)
