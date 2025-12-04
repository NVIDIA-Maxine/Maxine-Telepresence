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

#include "MainMenuBar.h"

#include <Windows.h>
#include <shobjidl_core.h>

#include <string>

#include "Core/EntityComponentSystem/Entity.h"
#include "Core/Error.h"
#include "imgui.h"

/// @brief Pointer to the file open/save dialog. This allows us to get the HWND for the dialog and move it to the front.
static ::IFileDialog* gFileDialog = nullptr;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Static function declarations                                                                                     ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Custom callback function that attempts to move the file dialog to the front
///
/// @see https://learn.microsoft.com/en-us/windows/win32/winmsg/getmsgproc
static LRESULT CALLBACK GetMsgProc(int code, WPARAM w_param, LPARAM l_param);

/// @brief Open a file dialog
/// @param[out] out_path A location of where to write the file path
/// @param[in]  save     Whether this is a save dialog, otherwise use a load dialog
/// @return              nv3dvc::core::Error::SUCCESS if successful
static nv3dvc::core::Error FileDialog(std::string* out_path, bool save);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Static function definitions                                                                                      ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static LRESULT CALLBACK GetMsgProc(int code, WPARAM w_param, LPARAM l_param) {
  nv3dvc::core::Error err = nv3dvc::core::Error::SUCCESS;

  IOleWindow* ole_window = nullptr;
  if (code == HC_ACTION) {
    // We can use the IOleWindow interface to get the HWND for the dialog.
    HWND hwnd = nullptr;
    CHECK_TRUE(SUCCEEDED(gFileDialog->QueryInterface<IOleWindow>(&ole_window)), nv3dvc::core::Error::ERR_WINDOW);
    CHECK_NONNULL(ole_window, nv3dvc::core::Error::ERR_WINDOW);
    CHECK_TRUE(SUCCEEDED(ole_window->GetWindow(&hwnd)), nv3dvc::core::Error::ERR_WINDOW);

    MSG* msg = reinterpret_cast<MSG*>(l_param);
    if (msg->hwnd == hwnd) {
      // If this message is for the dialog's window, then we know the window exists. Try to move it to the front.
      CHECK_TRUE(0 != ::SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW),
                 nv3dvc::core::Error::ERR_GENERAL);
    }
  }

bail:
  if (ole_window) ole_window->Release();
  return ::CallNextHookEx(NULL, code, w_param, l_param);
}

static nv3dvc::core::Error FileDialog(std::string* out_path, bool save) {
  nv3dvc::core::Error err = nv3dvc::core::Error::SUCCESS;

  HRESULT co_result = 0, result = 0;
  HHOOK hhook = nullptr;
  ::IShellItem* shell_item = nullptr;
  wchar_t* file_path = nullptr;

  CHECK_NULL(gFileDialog, nv3dvc::core::Error::ERR_GENERAL, "File dialog already open");

  co_result = ::CoInitializeEx(NULL, ::COINIT_APARTMENTTHREADED | ::COINIT_DISABLE_OLE1DDE);
  CHECK_TRUE(SUCCEEDED(co_result), nv3dvc::core::Error::ERR_GENERAL, "Could not initialize COM");

  // Create dialog
  // Create an instance of FileSaveDialog or FileOpenDialog with the IFileDialog interface.
  // The base FileDialog interface is sufficient for basic save and open operations.
  result = ::CoCreateInstance(save ? ::CLSID_FileSaveDialog : ::CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
                              IID_PPV_ARGS(&gFileDialog));
  CHECK_TRUE(SUCCEEDED(result), nv3dvc::core::Error::ERR_GENERAL, "Could not create dialog");
  CHECK_NONNULL(gFileDialog, nv3dvc::core::Error::ERR_GENERAL);

  // By default, when the application is full-screen, the file dialog appears behind the application. In order to
  // bring it to the front, the only method that seems to work is to install a hook procedure that intercepts all
  // messages posted to the thread's message queue and repeatedly try to move the window to the front.
  hhook = ::SetWindowsHookEx(WH_GETMESSAGE, ::GetMsgProc, NULL, GetCurrentThreadId());
  CHECK_NONNULL(hhook, nv3dvc::core::Error::ERR_GENERAL, "Could not install hook");

  // Show the dialog.
  result = gFileDialog->Show(::GetActiveWindow());
  if (SUCCEEDED(result)) {
    // Get the file name
    result = gFileDialog->GetResult(&shell_item);
    CHECK_TRUE(SUCCEEDED(result), nv3dvc::core::Error::ERR_GENERAL, "Could not get shell item from dialog");

    result = shell_item->GetDisplayName(::SIGDN_FILESYSPATH, &file_path);
    CHECK_TRUE(SUCCEEDED(result), nv3dvc::core::Error::ERR_GENERAL, "Could not get file path for selected");

    int in_str_character_count = static_cast<int>(wcslen(file_path));
    int bytes_needed = WideCharToMultiByte(CP_UTF8, 0, file_path, in_str_character_count, NULL, 0, NULL, NULL);
    bytes_needed += 1;
    out_path->resize(bytes_needed);

    int bytes_written = WideCharToMultiByte(CP_UTF8, 0, file_path, -1, out_path->data(), bytes_needed, NULL, NULL);
    CHECK_TRUE(bytes_written > 0, nv3dvc::core::Error::ERR_GENERAL);
  } else if (result == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
    err = nv3dvc::core::Error::ERR_GENERAL;  // Cancelled by user error
  } else {
    CHECK_TRUE(SUCCEEDED(result), nv3dvc::core::Error::ERR_GENERAL, "File dialog box show failed");
  }

bail:
  if (file_path) ::CoTaskMemFree(file_path);
  if (shell_item) shell_item->Release();
  if (hhook) ::UnhookWindowsHookEx(hhook);
  if (gFileDialog) gFileDialog->Release();
  gFileDialog = nullptr;
  if (SUCCEEDED(co_result)) ::CoUninitialize();
  return err;
}

namespace nv3dvc {
namespace modules {
namespace guimodule {
namespace panels {

MainMenuBar::MainMenuBar() {}
MainMenuBar::~MainMenuBar() {}

void MainMenuBar::Render(core::ecs::Entity selected_entity, WindowDisplayOptions* window_display_options,
                         core::engine::Engine* engine) {
  const bool show_menu_items = true;
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File", show_menu_items)) {
      ShowFileMenu(engine);
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Window", show_menu_items)) {
      ShowWindowMenu(window_display_options, selected_entity);
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }
}

void MainMenuBar::OnKeyPressed(const core::events::KeyPressEvent& e, core::engine::Engine* engine) {
  if (e.GetModifier() & core::application::inputs::Modifier::CONTROL &&
      e.GetModifier() & core::application::inputs::Modifier::SHIFT && e.GetKey() == core::application::inputs::Key::S) {
    ActionSaveApplicationConfig(engine);
  } else if (e.GetModifier() == core::application::inputs::Modifier::CONTROL &&
             e.GetKey() == core::application::inputs::Key::O) {
    ActionLoadScene(engine);
  } else if (e.GetModifier() == core::application::inputs::Modifier::CONTROL &&
             e.GetKey() == core::application::inputs::Key::S) {
    ActionSaveScene(engine);
  }
}

void MainMenuBar::ActionSaveApplicationConfig(core::engine::Engine* engine) {
  engine->Queue<core::engine::command::SaveAppConfig>();
}

void MainMenuBar::ActionSaveApplicationConfigAs(core::engine::Engine* engine) {
  std::string file_path;
  core::Error err = FileDialog(&file_path, true);
  if (err != core::Error::SUCCESS) return;
  engine->Queue<core::engine::command::SaveAppConfig>(file_path);
}

void MainMenuBar::ActionEmptyScene(core::engine::Engine* engine) { engine->Queue<core::engine::command::EmptyScene>(); }

void MainMenuBar::ActionLoadDefaultScene(core::engine::Engine* engine) {
  engine->Queue<core::engine::command::LoadDefaultScene>();
}

void MainMenuBar::ActionLoadScene(core::engine::Engine* engine) {
  std::string file_path;
  core::Error err = FileDialog(&file_path, false);
  if (err != core::Error::SUCCESS) return;
  engine->Queue<core::engine::command::LoadScene>(file_path);
}

void MainMenuBar::ActionSaveScene(core::engine::Engine* engine) { engine->Queue<core::engine::command::SaveScene>(); }

void MainMenuBar::ActionSaveSceneAs(core::engine::Engine* engine) {
  std::string file_path;
  core::Error err = FileDialog(&file_path, true);
  if (err != core::Error::SUCCESS) return;
  engine->Queue<core::engine::command::SaveScene>(file_path);
}

void MainMenuBar::ActionQuit(core::engine::Engine* engine) { engine->Queue<core::engine::command::Close>(); }

void MainMenuBar::ShowFileMenu(core::engine::Engine* engine) {
  if (ImGui::MenuItem("Save Application Config", "Shift+Ctrl+S")) {
    ActionSaveApplicationConfig(engine);
  }
  if (ImGui::MenuItem("Save Application Config As...")) {
    ActionSaveApplicationConfigAs(engine);
  }
  ImGui::Separator();
  if (ImGui::MenuItem("Empty Scene")) {
    ActionEmptyScene(engine);
  }
  if (ImGui::MenuItem("Load Default Scene")) {
    ActionLoadDefaultScene(engine);
  }
  if (ImGui::MenuItem("Load Scene...", "Ctrl+O")) {
    ActionLoadScene(engine);
  }
  if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
    ActionSaveScene(engine);
  }
  if (ImGui::MenuItem("Save Scene As...")) {
    ActionSaveSceneAs(engine);
  }
  ImGui::Separator();
  if (ImGui::MenuItem("Quit", "Alt+F4")) {
    ActionQuit(engine);
  }
}

void MainMenuBar::ShowWindowMenu(WindowDisplayOptions* window_display_options, core::ecs::Entity selected_entity) {
  ImGui::MenuItem("Application Config", nullptr, &window_display_options->show_app_config_panel, true);
  ImGui::MenuItem("Scene Hierarchy", nullptr, &window_display_options->show_scene_hierarchy_panel, true);
  ImGui::MenuItem("Video", nullptr, &window_display_options->show_video_panel, true);
  ImGui::MenuItem("Entity Properties", nullptr, &window_display_options->show_entity_properties_panel,
                  selected_entity.IsValid());
}

}  // namespace panels
}  // namespace guimodule
}  // namespace modules
}  // namespace nv3dvc
