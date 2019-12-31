
#include "common/int_types.h"

struct FStreamDesc
{
    EResult* status;
    void* dst;
    i32 offset;
    i32 size;
};

namespace FStream
{
    void Request(cstr path, FStreamDesc desc);
};
