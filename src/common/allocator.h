#pragma once

#include "common/int_types.h"
#include "containers/slice.h"

using Allocation = Slice<u8>;

namespace Allocator
{
    void* _ImGuiAllocFn(size_t sz, void*);
    void  _ImGuiFreeFn(void* ptr, void*);

    Allocation _Alloc(i32 want);
    void _Realloc(Allocation& alloc, i32 want);
    void _Free(Allocation& prev);

    template<typename T>
    inline constexpr i32 ReserveStartingCount()
    {
        if (sizeof(T) <= 1)
        {
            return 64;
        }
        if (sizeof(T) <= 2)
        {
            return 32;
        }
        if (sizeof(T) <= 8)
        {
            return 16;
        }
        if (sizeof(T) <= 64)
        {
            return 8;
        }
        if (sizeof(T) <= 128)
        {
            return 4;
        }
        if (sizeof(T) <= 512)
        {
            return 2;
        }
        return 1;
    }

    template<typename T>
    inline Slice<T> Alloc(i32 count)
    {
        return _Alloc(sizeof(T) * count).cast<T>();
    }

    template<typename T>
    inline void Realloc(Slice<T>& alloc, i32 count)
    {
        Allocation prevAlloc = alloc.cast<u8>();
         _Realloc(prevAlloc, sizeof(T) * count);
        alloc = prevAlloc.cast<T>();
    }

    template<typename T>
    inline void Free(Slice<T>& prev)
    {
        Allocation prevAlloc = prev.cast<u8>();
        _Free(prevAlloc);
        prev = prevAlloc.cast<T>();
    }

    template<typename T>
    inline void Reserve(Slice<T>& alloc, i32 count)
    {
        DebugAssert(count >= 0);
        const i32 cur = alloc.size();
        if (count > cur)
        {
            constexpr i32 minCount = ReserveStartingCount<T>();
            i32 next = cur << 1;
            next = Max(next, minCount);
            next = Max(next, count);
            Realloc(alloc, next);
        }
    }
};
