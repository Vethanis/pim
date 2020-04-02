#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include <stdint.h>

void render_sys_init(void);
void render_sys_update(void);
void render_sys_shutdown(void);

void render_sys_frameend(void);
int32_t render_sys_onevent(const struct sapp_event* evt);

int32_t screen_width(void);
int32_t screen_height(void);

PIM_C_END
