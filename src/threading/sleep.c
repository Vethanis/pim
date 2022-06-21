#include "sleep.h"
#include "intrin.h"

#if PLAT_WINDOWS
#pragma comment(lib, "Winmm.lib")

extern __declspec(dllimport) long __stdcall timeBeginPeriod(unsigned int uPeriod);
extern __declspec(dllimport) long __stdcall timeEndPeriod(unsigned int uPeriod);
extern __declspec(dllimport) void __stdcall Sleep(unsigned long dwMilliseconds);
#else

#include <time.h>

static void Sleep(unsigned long msec)
{
    struct timespec ts;

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;
    nanosleep(&ts, &ts);
}

#endif // PLAT_WINDOWS

void Intrin_BeginClockRes(u32 millisecondsPerTick)
{
    #if PLAT_WINDOWS
    timeBeginPeriod(millisecondsPerTick);
    #endif // PLAT_WINDOWS
}

void Intrin_EndClockRes(u32 millisecondsPerTick)
{
    #if PLAT_WINDOWS
    timeEndPeriod(millisecondsPerTick);
    #endif // PLAT_WINDOWS
}

void Intrin_Sleep(u32 milliseconds)
{
    Sleep(milliseconds);
}

