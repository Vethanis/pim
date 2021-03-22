#include "sleep.h"

#pragma comment(lib, "Winmm.lib")

extern __declspec(dllimport) long __stdcall timeBeginPeriod(unsigned int uPeriod);
extern __declspec(dllimport) long __stdcall timeEndPeriod(unsigned int uPeriod);
extern __declspec(dllimport) void __stdcall Sleep(unsigned long dwMilliseconds);

void Intrin_BeginClockRes(u32 millisecondsPerTick)
{
    timeBeginPeriod(millisecondsPerTick);
}

void Intrin_EndClockRes(u32 millisecondsPerTick)
{
    timeEndPeriod(millisecondsPerTick);
}

void Intrin_Sleep(u32 milliseconds)
{
    Sleep(milliseconds);
}

