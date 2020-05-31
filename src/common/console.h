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
#define C32_YELLOW  (C32_RED | C32_GREEN)
#define C32_MAGENTA (C32_BLUE | C32_RED)
#define C32_CYAN    (C32_BLUE | C32_GREEN)

void con_sys_init(void);
void con_sys_update(void);
void con_sys_shutdown(void);

void con_puts(u32 color, const char* line);
void con_printf(u32 color, const char* fmt, ...);
void con_clear(void);

typedef enum
{
    LogSev_Error,
    LogSev_Warning,
    LogSev_Info,
    LogSev_Verbose,

    LogSev_COUNT
} LogSev;

void con_logf(LogSev sev, const char* tag, const char* fmt, ...);

PIM_C_END
