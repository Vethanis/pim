#include "common/random.h"

#include "common/time.h"
#include "common/hashstring.h"
#include <pcg32/pcg32.h>

static const float kInvFloat = 1.0f / (1 << 24);
static PIM_TLS pcg32_random_t ts_state;

void rand_autoseed(void)
{
    uint64_t hash = HashStr("Piment");
    hash = (hash ^ time_now()) * 16777619u;
    ts_state.state = hash;
}

void rand_seed(uint64_t seed)
{
    ts_state.state = seed;
}

int32_t rand_int(void)
{
    return (int32_t)pcg32_random_r(&ts_state);
}

float rand_float(void)
{
    int32_t x = rand_int() & 0xffffff;
    return (float)x * kInvFloat;
}

int32_t rand_rangei(int32_t lo, int32_t hi)
{
    ASSERT(hi != lo);
    return lo + (rand_int() % (hi - lo));
}

float rand_rangef(float lo, float hi)
{
    return lo + (rand_float() * (hi - lo));
}
