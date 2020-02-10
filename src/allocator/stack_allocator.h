#pragma once

#include "allocator/iallocator.h"
#include "allocator/header.h"
#include "containers/array.h"
#include "os/thread.h"
#include <stdlib.h>

struct StackAllocator final : IAllocator
{
public:
    StackAllocator(i32 bytes) : IAllocator(bytes)
    {
        ASSERT(bytes > 0);
        void* memory = malloc(bytes);
        ASSERT(memory);

        memset(this, 0, sizeof(*this));
        m_mutex.Open();
        m_memory = { (u8*)memory, bytes };
        m_stack = m_memory;
        m_allocations.Init(Alloc_Stdlib);
    }

    ~StackAllocator()
    {
        m_mutex.Lock();
        m_allocations.Reset();
        void* ptr = m_memory.begin();
        free(ptr);
        m_mutex.Close();
        memset(this, 0, sizeof(*this));
    }

    void Clear() final
    {
        OS::LockGuard guard(m_mutex);
        m_stack = m_memory;
        m_allocations.Clear();
    }

    void* Alloc(i32 bytes) final
    {
        if (bytes <= 0)
        {
            ASSERT(bytes == 0);
            return nullptr;
        }
        OS::LockGuard guard(m_mutex);
        return _Alloc(bytes);
    }

    void Free(void* ptr) final
    {
        if (ptr)
        {
            OS::LockGuard guard(m_mutex);
            _Free(ptr);
        }
    }

    void* Realloc(void* ptr, i32 bytes) final
    {
        if (!ptr)
        {
            return Alloc(bytes);
        }

        if (bytes <= 0)
        {
            ASSERT(bytes == 0);
            Free(ptr);
            return nullptr;
        }

        OS::LockGuard guard(m_mutex);
        return _Realloc(ptr, bytes);
    }

private:
    OS::Mutex m_mutex;
    Slice<u8> m_memory;
    Slice<u8> m_stack;
    Array<Allocator::Header*> m_allocations;

    void* _Alloc(i32 bytes)
    {
        using namespace Allocator;

        bytes = AlignBytes(bytes);

        Slice<u8> allocation = m_stack.Head(bytes);
        m_stack = m_stack.Tail(bytes);

        Header* pNew = (Header*)allocation.begin();
        m_allocations.PushBack(pNew);
        return MakePtr(pNew, Alloc_Stack, bytes, 0);
    }

    void _Free(void* ptr)
    {
        using namespace Allocator;

        Header* hdr = ToHeader(ptr, Alloc_Stack);
        const i32 rc = Dec(hdr->refcount, MO_Relaxed);
        ASSERT(rc == 1);
        Store(hdr->arg1, 1);

        while (m_allocations.size())
        {
            Header* pBack = m_allocations.back();
            if (Load(pBack->arg1))
            {
                m_allocations.Pop();
                m_stack = Combine(m_stack, pBack->AsRaw());
            }
            else
            {
                break;
            }
        }
    }

    void* _Realloc(void* pOld, i32 bytes)
    {
        using namespace Allocator;

        _Free(pOld);
        void* pNew = _Alloc(bytes);

        if (pNew != pOld)
        {
            Copy(ToHeader(pNew, Alloc_Stack), ToHeader(pOld, Alloc_Stack));
        }

        return pNew;
    }
};
