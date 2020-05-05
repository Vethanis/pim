#include "rendering/tile.h"
#include "rendering/framebuffer.h"
#include "common/atomics.h"
#include "threading/intrin.h"

void AcquireTile(struct framebuf_s *target, i32 iTile)
{
    u32* pLock = target->tileFlags + iTile;
    u32 expected = 0;
    u64 spins = 0;
    while (!cmpex_u32(pLock, &expected, 0xff, MO_Acquire))
    {
        expected = 0;
        intrin_spin(++spins);
    }
}

void ReleaseTile(struct framebuf_s *target, i32 iTile)
{
    u32* pLock = target->tileFlags + iTile;
    u32 prev = exch_u32(pLock, 0x00, MO_Release);
    ASSERT(prev == 0xff);
}
