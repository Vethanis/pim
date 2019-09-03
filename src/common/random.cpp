#include "common/random.h"

#include "common/allocator.h"
#include "common/hash.h"
#include "os/thread.h"
#include "systems/time_system.h"
#include <pcg32/pcg32.h>

#include <stdio.h>

namespace Random
{
    static thread_local pcg32_random_t ts_state;

    void Seed()
    {
        constexpr u64 pimhash = Fnv64String("Piment");
        u64 hash = pimhash;
        hash = Fnv64Qword((u64)&printf, hash);
        hash = Fnv64Qword(Thread::Self().id, hash);
        hash = Fnv64Qword(TimeSystem::Now(), hash);
        ts_state.inc = 1;
        ts_state.state = hash;
    }

    u32 NextU32()
    {
        return pcg32_random_r(&ts_state);
    }
    f32 NextF32()
    {
        constexpr f32 r16 = 1.0f / 65536.0f;
        constexpr u32 mask = 0xffff;
        u32 uValue = NextU32() & mask;
        f32 fValue = (f32)uValue;
        return fValue * r16;
    }
    f32 NextF32(f32 lo, f32 hi)
    {
        return lo + NextF32() * (hi - lo);
    }
};
