#pragma once

#include "common/int_types.h"

struct a32 { volatile u32 Value; };
struct a64 { volatile u64 Value; };
struct aPtr { volatile ptr_t Value; };

enum MemOrder : u32
{
    MO_Relaxed = 0,
    MO_Acquire = 1 << 0,
    MO_Release = 1 << 1,
    MO_AcqRel = (MO_Acquire | MO_Release),
};
