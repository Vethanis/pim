#pragma once

#include "common/macro.h"

PIM_C_BEGIN

void time_sys_init(void);
void time_sys_update(void);
void time_sys_shutdown(void);

u32 time_framecount(void);
u64 time_appstart(void);
u64 time_framestart(void);

u64 time_now(void);
u64 time_dt(void);
double time_dtf(void);

double time_sec(u64 ticks);
double time_milli(u64 ticks);
double time_micro(u64 ticks);

float time_avgms(u64 begin, float prev, float t);

PIM_C_END
