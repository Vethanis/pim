#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include <stdint.h>

uint64_t intrin_timestamp(void);            // read timestamp counter
void intrin_pause(void);                    // mm_pause
void intrin_spin(uint64_t spins);           // spin this thread
void intrin_yield(void);                    // give up timeslice (don't use with non-default thread priority)

PIM_C_END
