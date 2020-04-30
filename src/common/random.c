#include "common/random.h"

#include "common/time.h"
#include "common/fnv1a.h"

prng_t prng_create(void)
{
    prng_t rng;
    u64 hash = HashStr("Piment");
    hash = (hash ^ time_now()) * 16777619u;
    rng.state = hash;
    return rng;
}
