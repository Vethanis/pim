#pragma once

#include "common/macro.h"
#include "common/minmax.h"
#include "containers/slice.h"
#include "containers/array.h"
#include "allocator/allocator_vtable.h"
#include <string.h>

namespace Allocator
{
    struct Stack
    {
        Slice<u8> m_memory;
        Slice<u8> m_stack;
        Array<Header*> m_allocations;

        void Init(void* memory, i32 bytes)
        {
            memset(this, 0, sizeof(*this));
            m_memory = { (u8*)memory, bytes };
            m_stack = m_memory;
            m_allocations.Init(Alloc_Stdlib);
        }

        void Shutdown()
        {
            m_allocations.Reset();
            memset(this, 0, sizeof(*this));
        }

        void Clear()
        {
            m_stack = m_memory;
            m_allocations.Clear();
        }

        Header* Alloc(i32 count)
        {
            Slice<u8> allocation = m_stack.Head(count);
            m_stack = m_stack.Tail(count);

            Header* pNew = (Header*)allocation.ptr;
            pNew->size = count;
            pNew->type = Alloc_Stack;
            pNew->c = m_allocations.Size();
            pNew->d = 0;

            m_allocations.Grow() = pNew;

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
            while (m_allocations.Size())
            {
                Header* pBack = m_allocations.back();
                if (pBack->d)
                {
                    m_allocations.Pop();
                    m_stack.Combine(pBack->AsSlice());
                }
                else
                {
                    break;
                }
            }
        }

        static constexpr const VTable Table = VTable::Create<Stack>();
    };
};
