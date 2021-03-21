#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct Prng_s { u64 state; } Prng;

Prng Prng_New(void);
Prng Prng_Get(void);
void Prng_Set(Prng rng);

pim_inline u32 Prng_u32(Prng *const pim_noalias rng)
{
    u64 oldstate = rng->state;
    rng->state = oldstate * 6364136223846793005ull + 1ull;
    u64 xorshifted = ((oldstate >> 18) ^ oldstate) >> 27;
    u32 rrot = (u32)((oldstate >> 59) & 31u);
    u32 lrot = ((~rrot) + 1u) & 31u;
    u64 result = (xorshifted >> rrot) | (xorshifted << lrot);
    return (u32)(result & 0xffffffffu);
}

pim_inline bool Prng_bool(Prng *const pim_noalias rng)
{
    return Prng_u32(rng) & 1u;
}

pim_inline i32 Prng_i32(Prng *const pim_noalias rng)
{
    return (i32)(0x7fffffff & Prng_u32(rng));
}

pim_inline u64 Prng_u64(Prng *const pim_noalias rng)
{
    u64 y = Prng_u32(rng);
    y <<= 32;
    y |= Prng_u32(rng);
    return y;
}

pim_inline float VEC_CALL Prng_f32(Prng *const pim_noalias rng)
{
    u32 x = Prng_u32(rng) & 0xffffff;
    const float kScale = 1.0f / (1 << 24);
    return (float)x * kScale;
}

PIM_C_END
