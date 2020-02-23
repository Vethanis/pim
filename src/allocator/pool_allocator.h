#pragma once

#include "allocator/iallocator.h"
#include "allocator/header.h"
#include "os/thread.h"
#include "containers/heap.h"
#include <stdlib.h>

struct PoolAllocator final : IAllocator
{
private:
    struct Item
    {
        i32 offset, size;
        bool operator<(Item rhs) const { return size < rhs.size; }
    };
    OS::Mutex m_mutex;
    Slice<u8> m_memory;
    Heap<Item> m_heap;
    AllocType m_type;

public:
    void Init(i32 bytes, AllocType type) final
    {
        ASSERT(bytes > 0);
        void* memory = malloc(bytes);
        ASSERT(memory);
        m_type = type;
        m_mutex.Open();
        m_memory = { (u8*)memory, bytes };
        m_heap.Init(Alloc_Init, 2048);
        m_heap.Insert({ 0, bytes });
    }

    void Reset() final
    {
        m_mutex.Lock();
        m_heap.Reset();
        void* ptr = m_memory.begin();
        m_memory = {};
        free(ptr);
        m_mutex.Close();
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
    void* _Alloc(i32 bytes)
    {
        using namespace Allocator;

        bytes = AlignBytes(bytes);

        Item item;
        if (m_heap.RemoveBestFit({ 0, bytes }, item))
        {
            if (item.size > bytes)
            {
                m_heap.Insert({ item.offset + bytes, item.size - bytes });
                item.size = bytes;
            }
            else if (item.size < bytes)
            {
                m_heap.Insert(item);
                return nullptr;
            }
            Slice<u8> region = m_memory.Subslice(item.offset, item.size);
            return MakePtr(region.begin(), m_type, item.size, item.offset);
        }
        return nullptr;
    }

    void _Free(void* pOld)
    {
        using namespace Allocator;

        Header* hOld = ToHeader(pOld, m_type);
        ASSERT(Dec(hOld->refcount) == 1);

        m_heap.Insert({ hOld->arg1, hOld->size });
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
            Copy(ToHeader(pNew, m_type), ToHeader(pOld, m_type));
        }

        return pNew;
    }
};
