# Download TensorRT from GitHub.
include(FetchContent)
FetchContent_Declare(
  TensorRT
  URL                 https://developer.nvidia.com/downloads/compute/machine-learning/tensorrt/10.9.0/zip/TensorRT-10.9.0.34.Windows.win10.cuda-12.8.zip
  EXCLUDE_FROM_ALL # don't build or install targets by default
)

message(STATUS "Populating TensorRT")
FetchContent_MakeAvailable(TensorRT)

FetchContent_GetProperties(TensorRT SOURCE_DIR TensorRT_SRC_DIR)

install(DIRECTORY ${TensorRT_SRC_DIR}/ DESTINATION External/TensorRT)
