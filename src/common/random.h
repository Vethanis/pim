#pragma once

#include "common/macro.h"

PIM_C_BEGIN

void rand_sys_init(void);
void rand_sys_update(void);
void rand_sys_shutdown(void);

void rand_autoseed(void);
void rand_seed(u64 seed);

i32 rand_int(void);
float rand_float(void);

i32 rand_rangei(i32 lo, i32 hi);
float rand_rangef(float lo, float hi);

PIM_C_END
