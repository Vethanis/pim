#include "common/iid.h"
#include "common/atomics.h"

static u32 ms_counter;

u32 iid_new(void)
{
    return inc_u32(&ms_counter, MO_Relaxed);
}
