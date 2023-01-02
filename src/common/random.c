#include "common/random.h"

#include "common/time.h"
#include "math/pcg.h"
#include "math/uint4_funcs.h"
#include "common/atomics.h"
#include "threading/task.h"
#include <string.h>

static Prng ms_prngs[kMaxThreads];
static u32 ms_counter;
static char const* const ms_seeds[] =
{
    "Xsapc69Gp4XLUEjn",
    "ZfbDFT8AvK46mv5r",
    "9zpqamNLPCR9tVQ3",
    "wq3BCn9ccnebyvbm",
    "mt9YVp8RpLbgMdkE",
    "hsRdvvapGzZEP7Fq",
    "AXFBhB5UYYKMXASZ",
    "T6t9QgANXez3h7tK",
    "ZteqCnbbAFp3Gjrw",
    "HhJR3zeEqAbZhxfs",
    "3E8SJmULPetTTrjc",
    "XPbuZtLE7pArMC7U",
    "68uPVggNF3aVB4ft",
    "f3HmXpHu2n4RbA3d",
    "8Mb4quWcmkhVLh4y",
    "TTUzBDpE5wbhEdQJ",
};

static const char* GetSeed(u32 x)
{
    return ms_seeds[Pcg1(x) % NELEM(ms_seeds)];
}

void Random_Init(void)
{
    for (i32 i = 0; i < NELEM(ms_prngs); ++i)
    {
        ms_prngs[i] = Prng_New();
    }
}

Prng Prng_New(void)
{
    Prng rng;

    u64 tick = Time_GetRawCounter();
    uint4 vtick;
    vtick.x = Pcg1((u32)((tick >> 0) & 0xffff));
    vtick.y = Pcg1((u32)((tick >> 16) & 0xffff));
    vtick.z = Pcg1((u32)((tick >> 32) & 0xffff));
    vtick.w = Pcg1((u32)((tick >> 48) & 0xffff));
    u32 id0 = inc_u32(&ms_counter, MO_Relaxed) + 1;
    u32 id1 = Pcg1(id0);
    uint4 hash = { 0 };
    hash = Pcg4_String(GetSeed(id0), hash);
    hash = Pcg4_Permute(u4_add(Pcg4_Lcg(hash), vtick));
    hash = Pcg4_String(GetSeed(id1), hash);
    hash = Pcg4_Permute(Pcg4_Lcg(hash));
    rng.state = hash;

    return rng;
}

Prng* Prng_Get(void)
{
    return &ms_prngs[Task_ThreadId()];
}
