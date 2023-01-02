#include "common/time.h"
#include "math/scalar.h"

// ----------------------------------------------------------------------------

#define Time_TicksPerSecond         (1000000000)
#define Time_TicksPerMillisecond    (1000000)
#define Time_TicksPerMicrosecond    (1000)

// ----------------------------------------------------------------------------

static u64 ms_appStart;
static u64 ms_freq;
static u64 ms_update;
static u64 ms_prevUpdate;
static u64 ms_dt;
static double ms_dtf;
static double ms_smoothdtf;
static u32 ms_frameCount;

// ----------------------------------------------------------------------------

void TimeSys_Init(void)
{
    ms_appStart = Time_GetRawCounter();
    ms_freq = Time_GetRawFreq();
    ms_frameCount = 0;
    ms_update = 0;
    ms_dt = 0;
    ms_dtf = 0.0;
    ms_smoothdtf = 0.0;
}

void TimeSys_Update(void)
{
    ++ms_frameCount;
    u64 now = Time_Now();
    ms_dt = now - ms_update;
    ms_prevUpdate = ms_update;
    ms_update = now;
    ms_dtf = Time_Sec(ms_dt);
    ms_smoothdtf = pim_lerp(ms_smoothdtf, ms_dtf, (1.0f / 120.0f));
}

void TimeSys_Shutdown(void) {}

// ----------------------------------------------------------------------------

u32 Time_FrameCount(void) { return ms_frameCount; }
u64 Time_AppStart(void) { return ms_appStart; }
u64 Time_FrameStart(void) { return ms_update; }
u64 Time_PrevFrame(void) { return ms_prevUpdate; }
u64 Time_Delta(void) { return ms_dt; }
double Time_Deltaf(void) { return ms_dtf; }
double Time_SmoothDeltaf(void) { return ms_smoothdtf; }
double Time_Sec(u64 ticks) { return (double)ticks / (double)Time_TicksPerSecond; }
double Time_Milli(u64 ticks) { return (double)ticks / (double)Time_TicksPerMillisecond; }
double Time_Micro(u64 ticks) { return (double)ticks / (double)Time_TicksPerMicrosecond; }

// ----------------------------------------------------------------------------

#if PLAT_WINDOWS

#include <windows.h>

u64 Time_GetRawCounter(void)
{
    LARGE_INTEGER li = { 0 };
    QueryPerformanceCounter(&li);
    SASSERT(sizeof(u64) == sizeof(li.QuadPart));
    return li.QuadPart;
}

u64 Time_GetRawFreq(void)
{
    LARGE_INTEGER li = { 0 };
    QueryPerformanceFrequency(&li);
    SASSERT(sizeof(u64) == sizeof(li.QuadPart));
    return li.QuadPart;
}

u64 Time_Now(void)
{
    u64 delta = Time_GetRawCounter() - ms_appStart;
    u64 freq = ms_freq;
    u64 seconds = delta / freq;
    u64 rem = delta % freq;
    u64 ticks = seconds * Time_TicksPerSecond + ((rem * Time_TicksPerSecond) / freq);
    return ticks;
}

#elif PLAT_LINUX

#include <time.h>

u64 Time_GetRawCounter(void)
{
    struct timespec ts = { 0 };
    clock_gettime(CLOCK_MONOTONIC, &ts);
    u64 seconds = ts.tv_sec;
    u64 nanoseconds = ts.tv_nsec;
    return seconds * Time_TicksPerSecond + nanoseconds;
}

u64 Time_GetRawFreq(void)
{
    return 1;
}

u64 Time_Now(void)
{
    return Time_GetRawCounter() - ms_appStart;
}

#endif // PLAT_XXX
