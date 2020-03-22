#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include <stdint.h>

void time_sys_init(void);
void time_sys_update(void);
void time_sys_shutdown(void);

uint32_t time_framecount(void);
uint64_t time_appstart(void);
uint64_t time_framestart(void);

uint64_t time_now(void);
uint64_t time_dt(void);
double time_dtf(void);

double time_sec(uint64_t ticks);
double time_milli(uint64_t ticks);
double time_micro(uint64_t ticks);

PIM_C_END
