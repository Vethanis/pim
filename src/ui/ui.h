#pragma once

#include "common/macro.h"

PIM_C_BEGIN

PIM_FWD_DECL(GLFWwindow);

void ui_sys_init(GLFWwindow* window);
void ui_sys_beginframe(void);
void ui_sys_endframe(void);
void ui_sys_shutdown(void);

PIM_C_END
