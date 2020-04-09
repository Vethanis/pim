#include "common/random.h"

#include "common/time.h"
#include "common/hashstring.h"

void prng_create(prng_t* prng)
{
    u64 hash = HashStr("Piment");
    hash = (hash ^ time_now()) * 16777619u;
    prng->state = hash;
}
