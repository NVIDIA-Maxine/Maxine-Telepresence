/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: LicenseRef-NvidiaProprietary
 *
 * NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
 * property and proprietary rights in and to this material, related
 * documentation and any modifications thereto. Any use, reproduction,
 * disclosure or distribution of this material and related documentation
 * without an express license agreement from NVIDIA CORPORATION or
 * its affiliates is strictly prohibited.
 */

#include "TriplaneLoaderBehaviorComponent.h"

#include <fstream>
#include <vector>

#include "TriplaneVolumeComponent.h"
#include "cuda.h"
#include "nvCVTriplaneVolume.h"

static std::vector<float> ReadFloatArrayFile(const std::string& file_path, int skip_header_lines) {
  std::fstream infile(file_path, std::ios_base::in);
  std::vector<float> res;
  float val;

  std::string header_line;
  for (int i = 0; i < skip_header_lines; i++) {
    std::getline(infile, header_line);
  }

  while (infile >> val) {
    char junk;
    infile >> junk;  // Skip over comma
    res.push_back(val);
  }
  return res;
}

namespace nv3dvc {
namespace modules {
namespace triplanemodule {
namespace components {

TriplaneLoaderBehaviorComponent::TriplaneLoaderBehaviorComponent() {}

TriplaneLoaderBehaviorComponent::~TriplaneLoaderBehaviorComponent() {}

nv3dvc::core::Error TriplaneLoaderBehaviorComponent::OnInitialize() {
  if (!HasComponent<TriplaneVolumeComponent>()) {
    return nv3dvc::core::ERR_GENERAL;
  }

  m_cvImage = cv::imread(image_file_path);
  if (!m_cvImage.data) return nv3dvc::core::ERR_GENERAL;

  static const NvCVImage_PixelFormat nvFormat[] = {NVCV_FORMAT_UNKNOWN, NVCV_Y, NVCV_YA, NVCV_BGR, NVCV_BGRA};
  static const NvCVImage_ComponentType nvType[] = {NVCV_U8,  NVCV_TYPE_UNKNOWN, NVCV_U16, NVCV_S16,
                                                   NVCV_S32, NVCV_F32,          NVCV_F64, NVCV_TYPE_UNKNOWN};
  NvCVImage nvcv_image;
  nvcv_image.pixels = m_cvImage.data;
  nvcv_image.width = m_cvImage.cols;
  nvcv_image.height = m_cvImage.rows;
  nvcv_image.pitch = static_cast<int>(m_cvImage.step[0]);
  const int n_channels = m_cvImage.channels();
  nvcv_image.pixelFormat = nvFormat[n_channels <= 4 ? n_channels : 0];
  nvcv_image.componentType = nvType[m_cvImage.depth() & 7];
  nvcv_image.bufferBytes = 0;
  nvcv_image.deletePtr = nullptr;
  nvcv_image.deleteProc = nullptr;
  nvcv_image.pixelBytes = static_cast<unsigned char>(m_cvImage.step[1]);
  nvcv_image.componentBytes = static_cast<unsigned char>(m_cvImage.elemSize1());
  nvcv_image.numComponents = static_cast<unsigned char>(m_cvImage.channels());
  nvcv_image.planar = NVCV_CHUNKY;
  nvcv_image.gpuMem = NVCV_CPU;
  nvcv_image.reserved[0] = 0;
  nvcv_image.reserved[1] = 0;

  std::vector<float> maxs = ReadFloatArrayFile(maxs_file_path, 0);
  std::vector<float> mins = ReadFloatArrayFile(mins_file_path, 0);

  NvCVTriplaneVolume triplane_volume_wrapper;
  triplane_volume_wrapper.config_params = {
      96,           // Number of effective triplane channels within the structure
      10,           // Used to splay out the triplane channels in a grid
      10,           // Used to splay out the triplane channels in a grid
      256,          // Width of one triplane
      256,          // Width of one triplane
      NVCV_Y,       // The format of the pixels in the triplanes
      NVCV_U8,      // The type of the components of the pixels.
      NVCV_CHUNKY,  // NVCV_CHUNKY, or NVCV_PLANAR
      NVCV_CPU,     // Location of the triplane buffer: one of { NVCV_CPU, NVCV_GPU, NVCV_CUDA }
      NVCV_CPU,     // Location of the mins, maxs buffers: one of { NVCV_CPU, NVCV_GPU, NVCV_CUDA }
      1,            // Row byte alignment.
  };
  triplane_volume_wrapper.triplanes = nvcv_image;
  triplane_volume_wrapper.quantization_mins = mins.data();
  triplane_volume_wrapper.quantization_maxs = maxs.data();

  auto& triplane_volume_component = GetComponent<TriplaneVolumeComponent>();
  triplane_volume_component.Allocate();
  NvCVTriplaneVolume* triplane_volume = triplane_volume_component.GetTriplaneVolumePtr();
  NvCV_Status err = NvCVVolume_TransferTriplaneVolume(&triplane_volume_wrapper, triplane_volume, nullptr);
  cuStreamSynchronize(nullptr);
  return nv3dvc::core::SUCCESS;
}

nv3dvc::core::Error TriplaneLoaderBehaviorComponent::OnUpdate(float dt) { return nv3dvc::core::SUCCESS; }

}  // namespace components
}  // namespace triplanemodule
}  // namespace modules
}  // namespace nv3dvc
