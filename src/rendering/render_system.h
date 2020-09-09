#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct framebuf_s framebuf_t;

void render_sys_init(void);
void render_sys_update(void);
void render_sys_shutdown(void);

void render_sys_gui(bool* pEnabled);

framebuf_t* render_sys_frontbuf(void);
framebuf_t* render_sys_backbuf(void);

PIM_C_END
