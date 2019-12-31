#pragma once

#include "common/macro.h"
#include "common/minmax.h"
#include "containers/slice.h"
#include "allocator/allocator_vtable.h"
#include <stdlib.h>
#include <string.h>

namespace Allocator
{
    struct Stack
    {
        Slice<u8> m_memory;
        Slice<u8> m_stack;
        Header** m_allocations;
        i32 m_length;
        i32 m_capacity;

        void Init(void* memory, i32 bytes)
        {
            memset(this, 0, sizeof(*this));
            m_memory = { (u8*)memory, bytes };
            m_stack = m_memory;
        }

        void Shutdown()
        {
            free(m_allocations);
            memset(this, 0, sizeof(*this));
        }

        void Clear()
        {
            m_stack = m_memory;
            m_length = 0;
        }

        Header* Alloc(i32 count)
        {
            i32 len = m_length;
            i32 cap = m_capacity;
            Header** allocations = m_allocations;
            if (len == cap)
            {
                cap = Max(cap * 2, 64);
                allocations = (Header**)realloc(allocations, cap * sizeof(Header**));
                ASSERT(allocations);
            }

            Slice<u8> allocation = m_stack.Head(count);
            m_stack = m_stack.Tail(count);

            Header* pNew = (Header*)allocation.ptr;
            pNew->size = count;
            pNew->type = Alloc_Stack;
            pNew->c = len;
            pNew->d = 0;

            allocations[len++] = pNew;

            m_length = len;
            m_capacity = cap;
            m_allocations = allocations;

            return pNew;
        }

        Header* Realloc(Header* pOld, i32 count)
        {
            Free(pOld);
            Header* pNew = Alloc(count);
            if (pNew != pOld)
            {
                Slice<u8> oldRegion = pOld->AsSlice().Tail(sizeof(Header));
                Slice<u8> newRegion = pNew->AsSlice().Tail(sizeof(Header));
                const i32 cpySize = Min(newRegion.Size(), oldRegion.Size());
                if (newRegion.Overlaps(oldRegion))
                {
                    memmove(newRegion.begin(), oldRegion.begin(), cpySize);
                }
                else
                {
                    memcpy(newRegion.begin(), oldRegion.begin(), cpySize);
                }
            }
            return pNew;
        }

        void Free(Header* hdr)
        {
            hdr->d = 1;

            Slice<u8> stack = m_stack;
            Header** allocations = m_allocations;
            i32 len = m_length;

            while (len > 0)
            {
                Header* pBack = allocations[len - 1];
                if (pBack->d == 1)
                {
                    --len;
                    stack.Combine(pBack->AsSlice());
                }
                else
                {
                    break;
                }
            }

            m_length = len;
            m_stack = stack;
        }

        static constexpr const VTable Table = VTable::Create<Stack>();
    };
};
