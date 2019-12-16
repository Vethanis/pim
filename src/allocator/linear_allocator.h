#pragma once

#include "common/macro.h"
#include "allocator/allocator_vtable.h"
#include "containers/slice.h"

namespace Allocator
{
    struct Linear
    {
        Slice<u8> m_memory;
        Slice<u8> m_current;

        void Init(void* memory, i32 bytes)
        {
            m_memory =
            {
                (u8*)memory,
                bytes,
            };
            m_current = m_memory;
        }

        void Shutdown()
        {
            m_memory = { 0, 0 };
            m_current = { 0, 0 };
        }

        void Clear()
        {
            m_current = m_memory;
        }

        Header* Alloc(i32 count)
        {
            Slice<u8> alloc = m_current.Head(count);
            m_current = m_current.Tail(count);
            return (Header*)alloc.begin();
        }

        Header* Realloc(Header* prev, i32 count)
        {
            Free(prev);
            return Alloc(count);
        }

        void Free(Header* prev)
        {
            Slice<u8> slice = { (u8*)prev, prev->size };
            if (slice.end() == m_current.begin())
            {
                // free the prior allocation only if it was the most recent
                // this is like a stack allocator but much less strict
                m_current.ptr = slice.ptr;
                m_current.len += slice.len;
            }
        }

        static constexpr const VTable Table = VTable::Create<Linear>();
    };
};
