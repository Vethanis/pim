#pragma once

#include "common/macro.h"
#include "allocator/allocator_vtable.h"
#include "containers/slice.h"
#include "os/thread.h"

namespace Allocator
{
    struct Linear
    {
        OS::Mutex m_mutex;
        Slice<u8> m_memory;
        Slice<u8> m_current;

        void Init(void* memory, i32 bytes)
        {
            m_mutex.Open();
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
            m_mutex.Close();
        }

        void Clear()
        {
            OS::LockGuard guard(m_mutex);
            m_current = m_memory;
        }

        Header* _Alloc(i32 count)
        {
            Slice<u8> alloc = m_current.Head(count);
            m_current = m_current.Tail(count);
            Header* pNew = (Header*)alloc.begin();
            pNew->size = count;
            pNew->type = Alloc_Linear;
            return pNew;
        }

        void _Free(Header* prev)
        {
            Slice<u8> slice = prev->AsSlice();
            if (Adjacent(m_current, slice))
            {
                m_current = Combine(m_current, slice);
            }
        }

        Header* _Realloc(Header* pOld, i32 count)
        {
            _Free(pOld);
            Header* pNew = _Alloc(count);
            ASSERT(pNew == pOld);
            return pNew;
        }

        Header* Alloc(i32 count)
        {
            OS::LockGuard guard(m_mutex);
            return _Alloc(count);
        }

        Header* Realloc(Header* pOld, i32 count)
        {
            OS::LockGuard guard(m_mutex);
            return _Realloc(pOld, count);
        }

        void Free(Header* prev)
        {
            OS::LockGuard guard(m_mutex);
            _Free(prev);
        }

        static constexpr const VTable Table = VTable::Create<Linear>();
    };
};
