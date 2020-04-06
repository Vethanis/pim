#include "common/random.h"

#include "common/time.h"
#include "common/hashstring.h"
#include <pcg32/pcg32.h>

static const float kInvFloat = 1.0f / (1 << 24);
static pim_thread_local pcg32_random_t ts_state;

void rand_sys_init(void) { rand_autoseed(); }
void rand_sys_update(void) {}
void rand_sys_shutdown(void) {}

void rand_autoseed(void)
{
    u64 hash = HashStr("Piment");
    hash = (hash ^ time_now()) * 16777619u;
    ts_state.state = hash;
}

void rand_seed(u64 seed)
{
    ts_state.state = seed;
}

i32 rand_int(void)
{
    return (i32)pcg32_random_r(&ts_state);
}

float rand_float(void)
{
    i32 x = rand_int() & 0xffffff;
    return (float)x * kInvFloat;
}

i32 rand_rangei(i32 lo, i32 hi)
{
    ASSERT(hi != lo);
    return lo + (rand_int() % (hi - lo));
}

float rand_rangef(float lo, float hi)
{
    return lo + (rand_float() * (hi - lo));
}
