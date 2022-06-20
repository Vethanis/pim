#pragma once

#include "common/macro.h"

PIM_C_BEGIN

void Intrin_Init(void);                 // initialize settings for intrinsics
u64 Intrin_Timestamp(void);             // read timestamp counter
void Intrin_Pause(void);                // mm_pause
void Intrin_Spin(u64 spins);            // spin this thread
void Intrin_Yield(void);                // give up timeslice (don't use with non-default thread priority)

PIM_C_END
