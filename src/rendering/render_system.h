#pragma once

#include "common/macro.h"

PIM_C_BEGIN

void render_sys_init(void);
void render_sys_update(void);
void render_sys_shutdown(void);

void render_sys_present(void);

void render_sys_gui(void);

PIM_C_END
