#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct GLFWwindow GLFWwindow;

void window_sys_init(void);
void window_sys_update(void);
void window_sys_shutdown(void);

GLFWwindow* window_get(void);

i32 window_width(void);
i32 window_height(void);
bool window_is_open(void);
void window_close(bool shouldClose);
void window_swapbuffers(void);

PIM_C_END
