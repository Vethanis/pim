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

void ConSys_Init(void);
void ConSys_Update(void);
void ConSys_Shutdown(void);

void Con_Exec(const char* cmdText);

void Con_Puts(u32 color, const char* line);
void Con_Printf(u32 color, const char* fmt, ...);
void Con_Clear(void);

typedef enum
{
    LogSev_Error,
    LogSev_Warning,
    LogSev_Info,
    LogSev_Verbose,

    LogSev_COUNT
} LogSev;

void Con_Logf(LogSev sev, const char* tag, const char* fmt, ...);
void Con_Logv(LogSev sev, const char* tag, const char* fmt, va_list ap);

PIM_C_END
