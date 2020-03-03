#pragma once

#include "allocator/iallocator.h"
#include "allocator/header.h"
#include "containers/array.h"
#include "os/thread.h"
#include "os/atomics.h"
#include <stdlib.h>

struct StackAllocator final : IAllocator
{
private:
    Slice<u8> m_memory;
    Slice<u8> m_stack;
    Array<Allocator::Header*> m_allocations;
    AllocType m_type;

public:
    void Init(i32 capacity, AllocType type) final
    {
        ASSERT(capacity > 0);
        void* memory = malloc(capacity);
        ASSERT(memory);
        m_type = type;
        m_memory = { (u8*)memory, capacity };
        m_stack = m_memory;
        m_allocations.Init(Alloc_Init);
    }

    void Reset() final
    {
        m_allocations.Reset();
        void* ptr = m_memory.begin();
        m_memory = {};
        free(ptr);
    }

    void Clear() final
    {
        m_stack = m_memory;
        m_allocations.Clear();
    }

    void* Alloc(i32 bytes) final
    {
        return _Alloc(bytes);
    }

    void Free(void* ptr) final
    {
        _Free(ptr);
    }

    void* Realloc(void* ptr, i32 bytes) final
    {
        return _Realloc(ptr, bytes);
    }

private:
    void* _Alloc(i32 userBytes)
    {
        using namespace Allocator;

        const i32 rawBytes = AlignBytes(userBytes);

        Slice<u8> allocation = m_stack.Head(rawBytes);
        m_stack = m_stack.Tail(rawBytes);

        void* pRaw = allocation.begin();
        Header* pHeader = RawToHeader(pRaw, m_type, rawBytes);

        m_allocations.PushBack(pHeader);

        return HeaderToUser(pHeader);
    }

    void _Free(void* pUser)
    {
        using namespace Allocator;

        Header* pHeader = UserToHeader(pUser, m_type);
        pHeader->arg1 = 1;

        while (m_allocations.size())
        {
            Header* pBackHeader = m_allocations.back();
            if (pBackHeader->arg1)
            {
                m_allocations.Pop();
                void* pRaw = HeaderToRaw(pBackHeader);
                m_stack = Combine(m_stack, { (u8*)pRaw, pBackHeader->size });
            }
            else
            {
                break;
            }
        }
    }

    void* _Realloc(void* pOldUser, i32 bytes)
    {
        using namespace Allocator;

        _Free(pOldUser);
        void* pNewUser = _Alloc(bytes);

        if (pNewUser != pOldUser)
        {
            Copy(UserToHeader(pNewUser, m_type), UserToHeader(pOldUser, m_type));
        }

        return pNewUser;
    }
};
