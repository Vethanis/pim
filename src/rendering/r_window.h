#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct GLFWwindow GLFWwindow;

void WinSys_Init(void);
void WinSys_Shutdown(void);

GLFWwindow* Window_Get(void);

i32 Window_Width(void);
i32 Window_Height(void);
bool Window_IsOpen(void);
void Window_Close(bool shouldClose);
void Window_Wait(void);

PIM_C_END
