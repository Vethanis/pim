#pragma once

#include "common/macro.h"

PIM_C_BEGIN

// AABBGGRR
#define C32_WHITE   0xffffffff
#define C32_GRAY    0xff808080
#define C32_BLACK   0xff000000
#define C32_RED     0xff0000ff
#define C32_GREEN   0xff00ff00
#define C32_BLUE    0xffff0000

void con_sys_init(void);
void con_sys_update(void);
void con_sys_shutdown(void);

void con_puts(u32 color, const char* line);
void con_printf(u32 color, const char* fmt, ...);
void con_clear(void);

PIM_C_END
