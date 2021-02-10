#include "threading/intrin.h"
#include <emmintrin.h>

void intrin_spin(u64 spins)
{
    u64 ticks = spins * 100;
    if (ticks >= 2500)
    {
        intrin_yield();
    }
    else
    {
        u64 end = intrin_timestamp() + ticks;
        do
        {
            // on sse2 _mm_pause indicates a spinloop to the cpu, in the hope that it clocks down temporarily
            intrin_pause();
        } while (intrin_timestamp() < end);
    }
}

#if PLAT_WINDOWS

// rdtsc
#include <windows.h>
#include <intrin.h>

#pragma intrinsic(__rdtsc)
#pragma intrinsic(_mm_pause)

u64 intrin_timestamp(void) { return __rdtsc(); }
void intrin_yield(void) { SwitchToThread(); }
#else

#endif // PLAT

void intrin_pause(void) { _mm_pause(); }
