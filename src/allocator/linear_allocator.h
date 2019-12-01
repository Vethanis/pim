#pragma once

#include "allocator/base_allocator.h"
#include <stdlib.h>

struct LinearAllocator final : BaseAllocator
{
    Slice<u8> m_allocation;
    Slice<u8> m_current;

    inline void Init(i32 capacity) final
    {
        m_allocation =
        {
            (u8*)malloc(capacity),
            capacity,
        };
        m_current = m_allocation;
    }

    inline void Shutdown() final
    {
        free(m_allocation.begin());
        m_allocation = { 0, 0 };
        m_current = { 0, 0 };
    }

    inline void Clear() final
    {
        m_current = m_allocation;
    }

    inline Slice<u8> Alloc(i32 count) final
    {
        Slice<u8> alloc = m_current.Head(count);
        m_current = m_current.Tail(count);
        return alloc;
    }

    inline Slice<u8> Realloc(Slice<u8> prev, i32 count) final
    {
        Free(prev);
        return Alloc(count);
    }

    inline void Free(Slice<u8> prev) final
    {
        if (prev.end() == m_current.begin())
        {
            m_current.ptr = prev.ptr;
            m_current.len += prev.len;
        }
    }
};
