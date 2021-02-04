#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct prng_s { u64 state; } prng_t;

prng_t prng_create(void);
prng_t prng_get(void);
void prng_set(prng_t rng);

pim_inline u32 prng_u32(prng_t *const pim_noalias rng)
{
    u64 oldstate = rng->state;
    rng->state = oldstate * 6364136223846793005ull + 1ull;
    u64 xorshifted = ((oldstate >> 18) ^ oldstate) >> 27;
    u32 rrot = (u32)((oldstate >> 59) & 31u);
    u32 lrot = ((~rrot) + 1u) & 31u;
    u64 result = (xorshifted >> rrot) | (xorshifted << lrot);
    return (u32)(result & 0xffffffffu);
}

pim_inline bool prng_bool(prng_t *const pim_noalias rng)
{
    return prng_u32(rng) & 1u;
}

pim_inline i32 prng_i32(prng_t *const pim_noalias rng)
{
    return (i32)(0x7fffffff & prng_u32(rng));
}

pim_inline u64 prng_u64(prng_t *const pim_noalias rng)
{
    u64 y = prng_u32(rng);
    y <<= 32;
    y |= prng_u32(rng);
    return y;
}

pim_inline float VEC_CALL prng_f32(prng_t *const pim_noalias rng)
{
    u32 x = prng_u32(rng) & 0xffffff;
    const float kScale = 1.0f / (1 << 24);
    return (float)x * kScale;
}

PIM_C_END
