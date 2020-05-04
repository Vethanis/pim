#include "common/random.h"

#include "common/time.h"
#include "common/fnv1a.h"
#include "threading/task.h"

prng_t prng_create(void)
{
    prng_t rng;
    u64 hash = HashStr("Piment");
    hash = Fnv64Qword(time_now(), hash);
    hash = Fnv64Dword(task_thread_id(), hash);
    rng.state = hash;
    return rng;
}
