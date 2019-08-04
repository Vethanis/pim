#pragma once

#include "common/int_types.h"

enum AllocatorType : i32
{
    Allocator_Malloc = 0,
    Allocator_Linear,
    Allocator_Stack,

    Allocator_Count
};

namespace Allocator
{
    void* _Alloc(AllocatorType type, usize bytes);
    void* _Realloc(AllocatorType type, void* pPrev, usize prevBytes, usize newBytes);
    void _Free(AllocatorType type, void* pPrev);

    template<typename T>
    inline T* Alloc(AllocatorType type, i32 count)
    {
        return (T*)_Alloc(type, sizeof(T) * count);
    }

    template<typename T>
    inline T* Realloc(AllocatorType type, T* pPrev, i32 prevCount, i32 newCount)
    {
        return (T*)_Realloc(type, (void*)pPrev, sizeof(T) * prevCount, sizeof(T) * newCount);
    }

    template<typename T>
    inline void Free(AllocatorType type, T* pPrev)
    {
        _Free(type, (void*)pPrev);
    }
};
