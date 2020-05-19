#include "common/random.h"

#include "common/time.h"
#include "common/fnv1a.h"
#include "threading/task.h"

static prng_t ms_prngs[kNumThreads];

prng_t prng_create(void)
{
    prng_t rng;
    u64 hash = HashStr("Piment");
    hash = Fnv64Qword(time_now(), hash);
    hash = Fnv64Dword(task_thread_id(), hash);
    rng.state = hash;
    return rng;
}

prng_t prng_get(void)
{
    prng_t rng = ms_prngs[task_thread_id()];
    if (rng.state == 0)
    {
        rng = prng_create();
    }
    return rng;
}

void prng_set(prng_t rng)
{
    ms_prngs[task_thread_id()] = rng;
}
