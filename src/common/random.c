#include "common/random.h"

#include "common/time.h"
#include "common/fnv1a.h"
#include "threading/task.h"

static Prng ms_prngs[kMaxThreads];

Prng Prng_New(void)
{
    Prng rng;
    u64 hash = HashStr("Piment");
    hash = Fnv64Qword(Time_Now(), hash);
    hash = Fnv64Dword(Task_ThreadId(), hash);
    rng.state = hash;
    return rng;
}

Prng Prng_Get(void)
{
    Prng rng = ms_prngs[Task_ThreadId()];
    if (rng.state == 0)
    {
        rng = Prng_New();
    }
    return rng;
}

void Prng_Set(Prng rng)
{
    ms_prngs[Task_ThreadId()] = rng;
}
