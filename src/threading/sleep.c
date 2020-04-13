#include "sleep.h"

#pragma comment(lib, "Winmm.lib")

extern __declspec(dllimport) long __stdcall timeBeginPeriod(unsigned int uPeriod);
extern __declspec(dllimport) long __stdcall timeEndPeriod(unsigned int uPeriod);
extern __declspec(dllimport) void __stdcall Sleep(unsigned long dwMilliseconds);

void intrin_clockres_begin(u32 millisecondsPerTick)
{
    timeBeginPeriod(millisecondsPerTick);
}

void intrin_clockres_end(u32 millisecondsPerTick)
{
    timeEndPeriod(millisecondsPerTick);
}

void intrin_sleep(u32 milliseconds)
{
    Sleep(milliseconds);
}

