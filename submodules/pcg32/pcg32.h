#pragma once

#include <stdint.h>

//#pragma warning(push)
//#pragma warning(disable : 4146)
//#pragma warning(disable : 4244)

// *Really* minimal PCG32 code / (c) 2014 M.E. O'Neill / pcg-random.org
// Licensed under Apache License 2.0 (NO WARRANTY, etc. see website)

typedef struct { uint64_t state; } pcg32_random_t;

static uint32_t pcg32_random_r(pcg32_random_t* rng)
{
    uint64_t oldstate = rng->state;
    // Advance internal state
    rng->state = oldstate * (uint64_t)6364136223846793005 + (uint64_t)1;
    // Calculate output function (XSH RR), uses old state for max ILP
    uint64_t xorshifted = ((oldstate >> 18) ^ oldstate) >> 27;
    uint64_t rot = oldstate >> 59;
    uint64_t res = (xorshifted >> rot) | (xorshifted << ((~rot + 1) & 31));
    return (uint32_t)(res & 0xffffffff);
}

//#pragma warning(pop)
