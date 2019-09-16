#pragma once

#include "common/int_types.h"
#include "containers/slice.h"

using Allocation = Slice<u8>;

namespace Allocator
{
    Allocation _Alloc(usize want);
    Allocation _Realloc(Allocation& prev, usize want);
    void _Free(Allocation& prev);

    template<typename T>
    inline Slice<T> Alloc(usize count)
    {
        return _Alloc(sizeof(T) * count).cast<T>();
    }

    template<typename T>
    inline Slice<T> Realloc(Slice<T>& prev, usize count)
    {
        Allocation prevAlloc = prev.cast<u8>();
        Allocation newAlloc = _Realloc(prevAlloc, sizeof(T) * count);
        prev = prevAlloc.cast<T>();
        return newAlloc.cast<T>();
    }

    template<typename T>
    inline void Free(Slice<T>& prev)
    {
        Allocation prevAlloc = prev.cast<u8>();
        _Free(prevAlloc);
        prev = prevAlloc.cast<T>();
    }

    template<typename T>
    inline Slice<T> Reserve(Slice<T>& prev, usize count)
    {
        const usize cur = prev.size();
        if (count > cur)
        {
            const usize next = cur ? cur * 2 : 16;
            const usize chosen = next > count ? next : count;
            return Realloc(prev, chosen);
        }
        return prev;
    }
};
