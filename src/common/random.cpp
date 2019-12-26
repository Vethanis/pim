#include "common/random.h"

#include "components/system.h"
#include "common/hash.h"
#include "time/time_system.h"
#include <pcg32/pcg32.h>
#include "math/math.h"

#include <stdio.h>

namespace Random
{
    static thread_local pcg32_random_t ts_state;

    static void Init()
    {
        constexpr u64 pimhash = Fnv64String("Piment");
        u64 hash = pimhash;
        hash = Fnv64Qword(TimeSystem::Now(), hash);
        hash = Fnv64Qword((u64)&printf, hash);
        ts_state.state = hash;
    }

    static void Update()
    {

    }

    static void Shutdown()
    {

    }

    static constexpr System ms_system =
    {
        ToGuid("Random"),
        { 0, 0 },
        Init,
        Update,
        Shutdown,
    };
    static RegisterSystem ms_register(ms_system);

    // ------------------------------------------------------------------------

    void Seed(u64 seed)
    {
        ts_state.state = seed;
    }

    u32 NextU32()
    {
        return pcg32_random_r(&ts_state);
    }
    u32 NextU32(u32 hi)
    {
        return NextU32() % hi;
    }
    u32 NextU32(u32 lo, u32 hi)
    {
        return lo + (NextU32() % (hi - lo));
    }
    i32 NextI32()
    {
        return math::abs((i32)NextU32());
    }
    i32 NextI32(i32 hi)
    {
        return NextI32() % hi;
    }
    i32 NextI32(i32 lo, i32 hi)
    {
        return lo + (NextI32() % (hi - lo));
    }
    f32 NextF32()
    {
        constexpr f32 r16 = 1.0f / 65536.0f;
        constexpr u32 mask = 0xffff;
        u32 uValue = NextU32() & mask;
        f32 fValue = (f32)uValue;
        return fValue * r16;
    }
    f32 NextF32(f32 hi)
    {
        return NextF32() * hi;
    }
    f32 NextF32(f32 lo, f32 hi)
    {
        return lo + NextF32() * (hi - lo);
    }

    u64 NextU64()
    {
        u32 a = NextU32();
        u32 b = NextU32();
        u64 x = a;
        x <<= 32;
        x |= b;
        return x;
    }
    u64 NextU64(u64 hi)
    {
        return NextU64() % hi;
    }
    u64 NextU64(u64 lo, u64 hi)
    {
        return lo + NextU64() % (hi - lo);
    }

    uint2 NextUint2()
    {
        return { NextU32(), NextU32() };
    }
    uint2 NextUint2(uint2 hi)
    {
        return NextUint2() % hi;
    }
    uint2 NextUint2(uint2 lo, uint2 hi)
    {
        return lo + NextUint2() % (hi - lo);
    }
    uint3 NextUint3()
    {
        return { NextU32(), NextU32(), NextU32() };
    }
    uint3 NextUint3(uint3 hi)
    {
        return NextUint3() % hi;
    }
    uint3 NextUint3(uint3 lo, uint3 hi)
    {
        return lo + NextUint3() % (hi - lo);
    }
    uint4 NextUint4()
    {
        return { NextU32(), NextU32(), NextU32(), NextU32() };
    }
    uint4 NextUint4(uint4 hi)
    {
        return NextUint4() % hi;
    }
    uint4 NextUint4(uint4 lo, uint4 hi)
    {
        return lo + NextUint4() % (hi - lo);
    }

    float2 NextFloat2()
    {
        return { NextF32(), NextF32() };
    }
    float2 NextFloat2(float2 hi)
    {
        return NextFloat2() * hi;
    }
    float2 NextFloat2(float2 lo, float2 hi)
    {
        return lo + NextFloat2() * (hi - lo);
    }
    float3 NextFloat3()
    {
        return { NextF32(), NextF32(), NextF32() };
    }
    float3 NextFloat3(float3 hi)
    {
        return NextFloat3() * hi;
    }
    float3 NextFloat3(float3 lo, float3 hi)
    {
        return lo + NextFloat3() * (hi - lo);
    }
    float4 NextFloat4()
    {
        return { NextF32(), NextF32(), NextF32(), NextF32() };
    }
    float4 NextFloat4(float4 hi)
    {
        return NextFloat4() * hi;
    }
    float4 NextFloat4(float4 lo, float4 hi)
    {
        return lo + NextFloat4() * (hi - lo);
    }
};
