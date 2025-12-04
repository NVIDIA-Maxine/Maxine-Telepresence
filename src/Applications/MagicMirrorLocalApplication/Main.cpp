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

#include "MagicMirrorLocalApplication.h"
#include "Utils/Logger.h"

char* g_nvARSDKPath = NULL;
char* g_nvVFXSDKPath = NULL;
// NvCodec logger
simplelogger::Logger* logger = simplelogger::LoggerFactory::CreateConsoleLogger();

int main(int argc, char** argv) {
  std::shared_ptr<nv3dvc::applications::MagicMirrorLocalApplication> app;
  try {
    app = std::make_shared<nv3dvc::applications::MagicMirrorLocalApplication>();
  } catch (std::exception& e) {
    std::cerr << "Failed to create application: " << e.what() << std::endl;
    return 1;
  }
  std::string app_config_path = "MagicMirrorLocalApplication.json";
  std::string scene_file_path = "";
  std::string additional_scene_config_json = "";
  if (argc > 1) {
    app_config_path = argv[1];
  }
  if (argc > 2) {
    scene_file_path = argv[2];
  }
  if (argc > 3) {
    additional_scene_config_json = argv[3];
  }
  app->LoadAppConfig(app_config_path);
  app->SetSceneConfigFilePath(scene_file_path);
  app->SetAdditionalSceneConfigJsonString(additional_scene_config_json);
  app->Run();
  return 0;
}
