#include "common/time.h"
#include <sokol/sokol_time.h>
#include "math/scalar.h"

static u64 ms_appStart;
static u64 ms_update;
static u64 ms_prevUpdate;
static u64 ms_dt;
static double ms_dtf64;
static u32 ms_frameCount;

void TimeSys_Init(void)
{
    ms_frameCount = 0;
    stm_setup();
    ms_appStart = Time_Now();
    ms_update = ms_appStart;
    ms_dt = 0;
    ms_dtf64 = 0.0;
}

void TimeSys_Update(void)
{
    ++ms_frameCount;
    ms_dt = stm_laptime(&ms_update);
    ms_dtf64 = Time_Sec(ms_dt);
}

void TimeSys_Shutdown(void) {}

u32 Time_FrameCount(void) { return ms_frameCount; }
u64 Time_AppStart(void) { return ms_appStart; }
u64 Time_FrameStart(void) { return ms_update; }
u64 Time_PrevFrame(void) { return ms_prevUpdate; }
u64 Time_Now(void) { return stm_now(); }
u64 Time_Delta(void) { return ms_dt; }
double Time_Deltaf(void) { return ms_dtf64; }
double Time_Sec(u64 ticks) { return stm_sec(ticks); }
double Time_Milli(u64 ticks) { return stm_ms(ticks); }
double Time_Micro(u64 ticks) { return stm_us(ticks); }

u64 Time_Lap(u64* tick)
{
    u64 prev = *tick;
    *tick = Time_Now();
    return prev;
}
