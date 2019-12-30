
#include "common/int_types.h"

struct FStreamDesc
{
    cstr path;
    i32 offset;
    i32 size;
    void(*OnChunk)(FStreamDesc& desc, void* ptr, i32 size);
    void(*OnFinish)(FStreamDesc& desc);
    void(*OnError)(FStreamDesc& desc);
    void* userData;
};

namespace FStream
{
    void Request(FStreamDesc desc);
};
