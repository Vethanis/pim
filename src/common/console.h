#pragma once

#include "common/macro.h"

PIM_C_BEGIN

// AABBGGRR
#define C32_WHITE   0xffffffff
#define C32_GRAY    0xff808080
#define C32_BLACK   0xff000000
#define C32_RED     0xff0000ff
#define C32_ORANGE  0xff0080ff
#define C32_YELLOW  0xff00ffff
#define C32_LIME    0xff00ff80
#define C32_GREEN   0xff00ff00
#define C32_SPRING  0xff80ff00
#define C32_CYAN    0xffffff00
#define C32_AZURE   0xffff8000
#define C32_BLUE    0xffff0000
#define C32_VIOLET  0xffff0080
#define C32_MAGENTA 0xffff00ff
#define C32_ROSE    0xff8000ff

#if _DEBUG
#   define Con_Assertf(x, tag, fmt, ...) do { if (!(x)) { Con_Logf(LogSev_Assert, (tag), (fmt), __VA_ARGS__); } } while(0)
#else
#   define Con_Assertf(x, tag, fmt, ...) 
#endif // _DEBUG

typedef enum
{
    LogSev_Assert,
    LogSev_Error,
    LogSev_Warning,
    LogSev_Info,
    LogSev_Verbose,

    LogSev_COUNT
} LogSev;

void ConSys_Init(void);
void ConSys_Update(void);
void ConSys_Shutdown(void);

void Con_Exec(const char* cmdText);
void Con_Clear(void);
void Con_Puts(u32 color, const char* line);

void Con_Logf(LogSev sev, const char* tag, const char* fmt, ...);
void Con_Logv(LogSev sev, const char* tag, const char* fmt, va_list ap);

// flush the logfile
void Con_Flush(void);

PIM_C_END
