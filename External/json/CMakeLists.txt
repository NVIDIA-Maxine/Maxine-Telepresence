include(FetchContent)

FetchContent_Declare(
  json
  URL https://github.com/nlohmann/json/releases/download/v3.10.5/json.tar.xz
  UPDATE_DISCONNECTED TRUE
  EXCLUDE_FROM_ALL # don't build or install targets by default
)

message(STATUS "Populating json")
FetchContent_MakeAvailable(json)

FetchContent_GetProperties(json SOURCE_DIR json_SRC_DIR)

set_target_properties(nlohmann_json PROPERTIES FOLDER External)

install(FILES ${json_SRC_DIR}/LICENSE.MIT DESTINATION External/json)
