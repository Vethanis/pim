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
            Header* pNew = (Header*)alloc.begin();
            pNew->size = count;
            pNew->type = Alloc_Linear;
            return pNew;
        }

        Header* Realloc(Header* pOld, i32 count)
        {
            Free(pOld);
            Header* pNew = Alloc(count);
            ASSERT(pNew == pOld);
            return pNew;
        }

        void Free(Header* prev)
        {
            Slice<u8> slice = prev->AsSlice();
            if (m_current.Adjacent(slice))
            {
                m_current.Combine(slice);
            }
        }

        static constexpr const VTable Table = VTable::Create<Linear>();
    };
};
