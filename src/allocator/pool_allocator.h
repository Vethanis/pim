#pragma once

#include "common/macro.h"
#include "common/minmax.h"
#include "containers/slice.h"
#include "containers/heap.h"
#include "allocator/allocator_vtable.h"
#include <string.h>
#include <stdlib.h>

namespace Allocator
{
    struct Pool
    {
        Slice<u8> m_memory;
        Heap m_heap;

        void Init(void* memory, i32 bytes)
        {
            m_memory = { (u8*)memory, bytes };
            m_heap.Init(Alloc_Stdlib, bytes);
        }

        void Shutdown()
        {
            m_heap.Reset();
            memset(this, 0, sizeof(*this));
        }

        void Clear()
        {
            m_heap.Clear();
        }

        Header* Alloc(i32 reqBytes)
        {
            const HeapItem item = m_heap.Alloc(reqBytes);
            const i32 offset = item.offset;
            const i32 size = item.size;
            if (offset == -1)
            {
                return nullptr;
            }
            Slice<u8> allocation = m_memory.Subslice(offset, size);
            Header* pNew = (Header*)allocation.begin();
            pNew->size = size;
            pNew->c = size;
            pNew->d = offset;
            return pNew;
        }

        void Free(Header* prev)
        {
            const i32 size = prev->c;
            const i32 offset = prev->d;
            ASSERT(offset >= 0);
            ASSERT(size >= prev->size);
            ASSERT((m_memory.begin() + offset) == ((u8*)prev));

            HeapItem item;
            item.offset = offset;
            item.size = size;
            m_heap.Free(item);
        }

        Header* Realloc(Header* pOld, i32 reqBytes)
        {
            Free(pOld);
            Header* pNew = Alloc(reqBytes);
            if (!pNew)
            {
                return nullptr;
            }

            if (pNew != pOld)
            {
                Slice<u8> oldSlice = pOld->AsSlice().Tail(sizeof(Header));
                Slice<u8> newSlice = pNew->AsSlice().Tail(sizeof(Header));
                const i32 cpySize = Min(newSlice.size(), oldSlice.size());
                if (Overlaps(newSlice, oldSlice))
                {
                    memmove(newSlice.begin(), oldSlice.begin(), cpySize);
                }
                else
                {
                    memcpy(newSlice.begin(), oldSlice.begin(), cpySize);
                }
            }

            return pNew;
        }

        static constexpr const VTable Table = VTable::Create<Pool>();
    };
};
