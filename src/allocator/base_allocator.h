#pragma once

#include "containers/slice.h"

struct BaseAllocator
{
    virtual void Init(i32 capacity) = 0;
    virtual void Shutdown() = 0;
    virtual void Clear() = 0;
    virtual Slice<u8> Alloc(i32 count) = 0;
    virtual Slice<u8> Realloc(Slice<u8> prev, i32 count) = 0;
    virtual void Free(Slice<u8> prev) = 0;
};
