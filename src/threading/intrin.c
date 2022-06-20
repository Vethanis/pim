#include "threading/intrin.h"
#include <emmintrin.h>

void Intrin_Spin(u64 spins)
{
    u64 ticks = spins * 100;
    if (ticks >= 2500)
    {
        Intrin_Yield();
    }
    else
    {
        u64 end = Intrin_Timestamp() + ticks;
        do
        {
            // on sse2 _mm_pause indicates a spinloop to the cpu, in the hope that it clocks down temporarily
            Intrin_Pause();
        } while (Intrin_Timestamp() < end);
    }
}

#if PLAT_WINDOWS

// rdtsc
#include <windows.h>
#include <intrin.h>

#pragma intrinsic(__rdtsc)
#pragma intrinsic(_mm_pause)

void Intrin_Init(void) { }
u64 Intrin_Timestamp(void) { return __rdtsc(); }
void Intrin_Yield(void) { SwitchToThread(); }
#else
#include <time.h>
#include <sched.h>

void Intrin_Init(void)
{
    struct sched_param param = {
        .sched_priority = 1
    };
    ASSERT(sched_setscheduler(0, SCHED_RR, &param) == 0);
}

u64 Intrin_Timestamp(void)
{
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC_RAW, &tp);
    return tp.tv_nsec;
}

void Intrin_Yield(void) { sched_yield(); }
#endif // PLAT

void Intrin_Pause(void) { _mm_pause(); }
