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

#ifndef SRC_CORE_EVENTS_FILEEVENT_H_
#define SRC_CORE_EVENTS_FILEEVENT_H_

#include <string>

#include "Event.h"

namespace nv3dvc {
namespace core {
namespace events {

/// @brief Event triggered when a file should be loaded
class FileEvent : public Event {
 public:
  explicit FileEvent(const std::string& file_path) : m_filePath(file_path) {}
  EventType Type() const override { return EventType::FILE_EVENT; }
  std::string ToString() const override { return "FileEvent : { file_path " + m_filePath + " }"; }
  const std::string GetFilePath() const { return m_filePath; }

 private:
  const std::string m_filePath;
};

}  // namespace events
}  // namespace core
}  // namespace nv3dvc

#endif  // SRC_CORE_EVENTS_FILEEVENT_H_
