#pragma once

#include "common/macro.h"

PIM_C_BEGIN

void window_sys_init(void);
void window_sys_update(void);
void window_sys_shutdown(void);

i32 window_width(void);
i32 window_height(void);
bool window_is_open(void);
void window_close(bool shouldClose);
float window_get_target(void);
void window_set_target(float fps);
void window_swapbuffers(void);
bool window_cursor_captured(void);
void window_capture_cursor(bool capture);
struct GLFWwindow* window_ptr(void);

PIM_C_END
