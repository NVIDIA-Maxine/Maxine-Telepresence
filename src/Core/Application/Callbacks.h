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

#ifndef SRC_CORE_APPLICATION_CALLBACKS_H_
#define SRC_CORE_APPLICATION_CALLBACKS_H_

#include <GLFW/glfw3.h>

namespace nv3dvc {
namespace core {
namespace application {

void GlfwErrorCallback(int error_code, const char* description);
void GlfwWindowPosCallback(GLFWwindow* window, int xpos, int ypos);
void GlfwWindowSizeCallback(GLFWwindow* window, int width, int height);
void GlfwWindowCloseCallback(GLFWwindow* window);
void GlfwWindowRefreshCallback(GLFWwindow* window);
void GlfwWindowFocusCallback(GLFWwindow* window, int focused);
void GlfwWindowIconifyCallback(GLFWwindow* window, int iconified);
void GlfwWindowMaximizeCallback(GLFWwindow* window, int maximized);
void GlfwFramebufferSizeCallback(GLFWwindow* window, int width, int height);
void GlfwWindowContentScaleCallback(GLFWwindow* window, float xscale, float yscale);
void GlfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void GlfwCharCallback(GLFWwindow* window, unsigned int codepoint);
void GlfwCursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void GlfwCursorEnterCallback(GLFWwindow* window, int entered);
void GlfwMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void GlfwScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void GlfwDropCallback(GLFWwindow* window, int path_count, const char* paths[]);

}  // namespace application
}  // namespace core
}  // namespace nv3dvc

#endif  // SRC_CORE_APPLICATION_CALLBACKS_H_
