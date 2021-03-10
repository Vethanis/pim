#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct GLFWwindow GLFWwindow;

bool igImplGlfw_InitForOpenGL(GLFWwindow* window, bool install_callbacks);
bool igImplGlfw_InitForVulkan(GLFWwindow* window, bool install_callbacks);
void igImplGlfw_Shutdown(void);
void igImplGlfw_NewFrame(void);

void igImplGlfw_MouseButtonCallback(GLFWwindow* window, i32 button, i32 action, i32 mods);
void igImplGlfw_ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void igImplGlfw_KeyCallback(GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 mods);
void igImplGlfw_CharCallback(GLFWwindow* window, u32 c);

PIM_C_END
