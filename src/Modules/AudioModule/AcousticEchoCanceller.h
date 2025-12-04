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

#ifndef SRC_MODULES_AUDIOMODULE_ACOUSTICECHOCANCELLER_H_
#define SRC_MODULES_AUDIOMODULE_ACOUSTICECHOCANCELLER_H_

#include <memory>
#include <string>

namespace nv3dvc {
namespace modules {
namespace audiomodule {

class IEchoCanceller {
 public:
  IEchoCanceller() {}
  virtual ~IEchoCanceller() {}

  virtual bool Initialize(const std::string& afx_model_dir) = 0;

  virtual int GetNumSamplesPerFrame() const = 0;

  virtual bool Filter(const float* near_end, const float* far_end, float* filtered) = 0;

 protected:
  int m_framesPerBuffer = 480;
};

std::unique_ptr<IEchoCanceller> CreateEchoCanceller();

}  // namespace audiomodule
}  // namespace modules
}  // namespace nv3dvc

#endif  // SRC_MODULES_AUDIOMODULE_ACOUSTICECHOCANCELLER_H_
