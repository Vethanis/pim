#pragma once

#include "common/int_types.h"

namespace Random
{
    void Seed();

    u32 NextU32();
    u32 NextU32(u32 lo, u32 hi);
    i32 NextI32();
    i32 NextI32(i32 lo, i32 hi);
    f32 NextF32();
    f32 NextF32(f32 lo, f32 hi);
};
