#include "components/compflag.h"
#include "common/valist.h"

u32 compflag_create(i32 count, ...)
{
    u32 result = 0;
    VaList va = VA_START(count);
    for (i32 i = 0; i < count; ++i)
    {
        compid_t id = VA_ARG(va, compid_t);
        result |= (1 << id);
    }
    return result;
}
