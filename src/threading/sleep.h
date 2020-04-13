#pragma once

#include "common/macro.h"

PIM_C_BEGIN

void intrin_clockres_begin(u32 millisecondsPerTick);
void intrin_clockres_end(u32 millisecondsPerTick);

void intrin_sleep(u32 milliseconds);

PIM_C_END
