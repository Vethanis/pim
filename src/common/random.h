#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct prng_s { u64 state; } prng_t;

prng_t prng_create(void);
prng_t prng_get(void);
void prng_set(prng_t rng);

#pragma warning(push)
#pragma warning(disable : 4146)
#pragma warning(disable : 4244)

pim_inline u32 prng_u32(prng_t* rng)
{
    u64 oldstate = rng->state;
    rng->state = oldstate * 6364136223846793005ull + 1ull;
    u32 xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    u32 rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((~rot) & 31u));
}

#pragma warning(pop)

pim_inline u64 prng_u64(prng_t* rng)
{
    u64 y = prng_u32(rng);
    y <<= 32;
    y |= prng_u32(rng);
    return y;
}

pim_inline i32 prng_i32(prng_t* rng)
{
    return (i32)(0x7fffffff & prng_u32(rng));
}
pim_inline float prng_f32(prng_t* rng)
{
    u32 x = prng_u32(rng) & 0xffff;
    const float kScale = 1.0f / (1 << 16);
    return (float)x * kScale;
}

pim_inline bool prng_bool(prng_t* rng) { return prng_u32(rng) & 1u; }

PIM_C_END
