#pragma once

#include "allocator/base_allocator.h"
#include <stdlib.h>

struct StackAllocator final : BaseAllocator
{
    inline void Init(i32 capacity) final
    {

    }

    inline void Shutdown() final
    {

    }

    inline void Clear() final
    {

    }

    inline Slice<u8> Alloc(i32 count) final
    {
        return { 0, 0 };
    }

    inline Slice<u8> Realloc(Slice<u8> prev, i32 count) final
    {
        return { 0, 0 };
    }

    inline void Free(Slice<u8> prev) final
    {
    }
};
