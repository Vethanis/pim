#pragma once

#include "common/macro.h"

PIM_C_BEGIN

void Intrin_BeginClockRes(u32 millisecondsPerTick);
void Intrin_EndClockRes(u32 millisecondsPerTick);

void Intrin_Sleep(u32 milliseconds);

PIM_C_END
