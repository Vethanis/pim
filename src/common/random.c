#include "common/random.h"

#include "common/time.h"
#include "common/fnv1a.h"
#include "threading/task.h"
#include <string.h>

static Prng ms_prngs[kMaxThreads];
static bool ms_once[kMaxThreads];

Prng Prng_New(void)
{
    Prng rng;
    u64 hashA = Fnv64Bias;
    hashA = Fnv64String("Piment", hashA);
    hashA = Fnv64Qword(Time_Now(), hashA);
    hashA = Fnv64Dword(Task_ThreadId(), hashA);
    u64 hashB = hashA;
    hashB = Fnv64String("Piment", hashB);
    hashB = Fnv64Qword(Time_Now(), hashB);
    hashB = Fnv64Dword(Task_ThreadId(), hashB);
    memcpy(&rng.state.x, &hashA, sizeof(hashA));
    memcpy(&rng.state.z, &hashB, sizeof(hashB));
    Prng_Next(&rng);
    Prng_Next(&rng);
    Prng_Next(&rng);
    Prng_Next(&rng);
    return rng;
}

Prng Prng_Get(void)
{
    u32 tid = Task_ThreadId();
    Prng rng = ms_prngs[tid];
    if (!ms_once[tid])
    {
        ms_once[tid] = true;
        rng = Prng_New();
        ms_prngs[tid] = rng;
    }
    return rng;
}

void Prng_Set(Prng rng)
{
    ms_prngs[Task_ThreadId()] = rng;
}
