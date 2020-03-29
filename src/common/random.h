#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include <stdint.h>

void rand_sys_init(void);
void rand_sys_update(void);
void rand_sys_shutdown(void);

void rand_autoseed(void);
void rand_seed(uint64_t seed);

int32_t rand_int(void);
float rand_float(void);

int32_t rand_rangei(int32_t lo, int32_t hi);
float rand_rangef(float lo, float hi);

PIM_C_END
