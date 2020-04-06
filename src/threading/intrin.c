#include "threading/intrin.h"

void intrin_spin(uint64_t spins)
{
    uint64_t ticks = spins * 100;
    if (ticks >= 2500)
    {
        intrin_yield();
    }
    else
    {
        uint64_t end = intrin_timestamp() + ticks;
        do
        {
            // on sse2 _mm_pause indicates a spinloop to the cpu, in the hope that it clocks down temporarily
            intrin_pause();
        } while (intrin_timestamp() < end);
    }
}

#if PLAT_WINDOWS

#include <windows.h>
#include <intrin.h>

#pragma intrinsic(__rdtsc)
#pragma intrinsic(_mm_pause)

uint64_t intrin_timestamp(void) { return __rdtsc(); }
void intrin_pause(void) { _mm_pause(); }
void intrin_yield(void) { SwitchToThread(); }

#else

#endif // PLAT
