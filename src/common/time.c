#include "common/time.h"
#include <sokol/sokol_time.h>

static u64 ms_appStart;
static u64 ms_lastTime;
static u64 ms_dt;
static double ms_dtf64;
static u32 ms_frameCount;

void time_sys_init(void)
{
    ms_frameCount = 0;
    stm_setup();
    ms_appStart = time_now();
    ms_lastTime = ms_appStart;
    ms_dt = 0;
    ms_dtf64 = 0.0;
}

void time_sys_update(void)
{
    ++ms_frameCount;
    ms_dt = stm_laptime(&ms_lastTime);
    ms_dtf64 = time_sec(ms_dt);
}

void time_sys_shutdown(void) {}

u32 time_framecount(void) { return ms_frameCount; }
u64 time_appstart(void) { return ms_appStart; }
u64 time_framestart(void) { return ms_lastTime; }
u64 time_now(void) { return stm_now(); }
u64 time_dt(void) { return ms_dt; }
double time_dtf(void) { return ms_dtf64; }
double time_sec(u64 ticks) { return stm_sec(ticks); }
double time_milli(u64 ticks) { return stm_ms(ticks); }
double time_micro(u64 ticks) { return stm_us(ticks); }
