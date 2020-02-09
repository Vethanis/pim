#pragma once

#include "allocator/iallocator.h"
#include "allocator/header.h"
#include "os/thread.h"
#include "containers/heap.h"
#include <stdlib.h>

struct PoolAllocator final : IAllocator
{
public:
    PoolAllocator(i32 bytes) : IAllocator(bytes)
    {
        ASSERT(bytes > 0);
        void* memory = malloc(bytes);
        ASSERT(memory);
        m_mutex.Open();
        m_memory = { (u8*)memory, bytes };
        m_heap.Init(Alloc_Stdlib, bytes);
    }

    ~PoolAllocator()
    {
        m_mutex.Lock();
        m_heap.Reset();
        void* ptr = m_memory.begin();
        free(ptr);
        m_mutex.Close();
        memset(this, 0, sizeof(*this));
    }

    void Clear() final
    {
        OS::LockGuard guard(m_mutex);
        m_heap.Clear();
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
    Heap m_heap;

    static HeapItem ToHeap(void* ptr)
    {
        using namespace Allocator;

        Header* hdr = ToHeader(ptr, Alloc_Pool);
        HeapItem item;
        item.offset = hdr->c;
        item.size = hdr->d;
        return item;
    }

    static void* ToPtr(HeapItem item, Slice<u8> memory)
    {
        using namespace Allocator;

        if (item.offset == -1)
        {
            return nullptr;
        }
        Slice<u8> region = memory.Subslice(item.offset, item.size);
        return MakePtr(region.begin(), Alloc_Pool, item.size, item.offset, item.size);
    }

    void* _Alloc(i32 bytes)
    {
        using namespace Allocator;

        bytes = AlignBytes(bytes);
        HeapItem item = m_heap.Alloc(bytes);
        return ToPtr(item, m_memory);
    }

    void _Free(void* ptr)
    {
        using namespace Allocator;

        HeapItem item = ToHeap(ptr);
        m_heap.Free(item);
    }

    void* _Realloc(void* pOld, i32 bytes)
    {
        using namespace Allocator;

        _Free(pOld);
        void* pNew = _Alloc(bytes);
        if (!pNew)
        {
            return nullptr;
        }

        if (pNew != pOld)
        {
            Copy(ToHeader(pNew, Alloc_Pool), ToHeader(pOld, Alloc_Pool));
        }

        return pNew;
    }
};
