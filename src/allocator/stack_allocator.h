#pragma once

#include "common/macro.h"
#include "common/minmax.h"
#include "containers/slice.h"
#include "containers/array.h"
#include "allocator/allocator_vtable.h"
#include "os/thread.h"
#include <string.h>

namespace Allocator
{
    struct Stack
    {
        OS::Mutex m_mutex;
        Slice<u8> m_memory;
        Slice<u8> m_stack;
        Array<Header*> m_allocations;

        void Init(void* memory, i32 bytes)
        {
            memset(this, 0, sizeof(*this));
            m_mutex.Open();
            m_memory = { (u8*)memory, bytes };
            m_stack = m_memory;
            m_allocations.Init(Alloc_Stdlib);
        }

        void Shutdown()
        {
            m_allocations.Reset();
            m_mutex.Close();
            memset(this, 0, sizeof(*this));
        }

        void Clear()
        {
            OS::LockGuard guard(m_mutex);
            m_stack = m_memory;
            m_allocations.Clear();
        }

        Header* _Alloc(i32 count)
        {
            Slice<u8> allocation = m_stack.Head(count);
            m_stack = m_stack.Tail(count);

            Header* pNew = (Header*)allocation.ptr;
            pNew->size = count;
            pNew->type = Alloc_Stack;
            pNew->c = m_allocations.size();
            pNew->d = 0;

            m_allocations.Grow() = pNew;

            return pNew;
        }

        void _Free(Header* hdr)
        {
            hdr->d = 1;
            while (m_allocations.size())
            {
                Header* pBack = m_allocations.back();
                if (pBack->d)
                {
                    m_allocations.Pop();
                    m_stack = Combine(m_stack, pBack->AsSlice());
                }
                else
                {
                    break;
                }
            }
        }

        Header* _Realloc(Header* pOld, i32 count)
        {
            _Free(pOld);
            Header* pNew = _Alloc(count);
            if (pNew != pOld)
            {
                Slice<u8> oldRegion = pOld->AsSlice().Tail(sizeof(Header));
                Slice<u8> newRegion = pNew->AsSlice().Tail(sizeof(Header));
                const i32 cpySize = Min(newRegion.size(), oldRegion.size());
                if (Overlaps(newRegion, oldRegion))
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

        Header* Alloc(i32 count)
        {
            OS::LockGuard guard(m_mutex);
            return _Alloc(count);
        }

        void Free(Header* hdr)
        {
            OS::LockGuard guard(m_mutex);
            _Free(hdr);
        }

        Header* Realloc(Header* pOld, i32 count)
        {
            OS::LockGuard guard(m_mutex);
            return _Realloc(pOld, count);
        }

        static constexpr const VTable Table = VTable::Create<Stack>();
    };
};
