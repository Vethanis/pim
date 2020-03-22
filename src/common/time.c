#include "common/time.h"
#include <sokol/sokol_time.h>

static uint64_t ms_appStart;
static uint64_t ms_lastTime;
static uint64_t ms_dt;
static double ms_dtf64;
static uint32_t ms_frameCount;

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

uint32_t time_framecount(void) { return ms_frameCount; }
uint64_t time_appstart(void) { return ms_appStart; }
uint64_t time_framestart(void) { return ms_lastTime; }
uint64_t time_now(void) { return stm_now(); }
uint64_t time_dt(void) { return ms_dt; }
double time_dtf(void) { return ms_dtf64; }
double time_sec(uint64_t ticks) { return stm_sec(ticks); }
double time_milli(uint64_t ticks) { return stm_ms(ticks); }
double time_micro(uint64_t ticks) { return stm_us(ticks); }
