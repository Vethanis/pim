#pragma once

#include "common/int_types.h"

namespace Random
{
    void Seed();

    u32 NextU32();
    f32 NextF32();
    f32 NextF32(f32 lo, f32 hi);
};
