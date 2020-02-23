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
    void Init(i32 capacity, AllocType type) final
    {
        ASSERT(capacity > 0);
        void* memory = malloc(capacity);
        ASSERT(memory);
        m_type = type;
        m_mutex.Open();
        m_memory = { (u8*)memory, capacity };
        m_heap.Init(Alloc_Init, 2048);
        m_heap.Insert({ 0, capacity });
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
    void* _Alloc(i32 userBytes)
    {
        using namespace Allocator;

        const i32 rawBytes = AlignBytes(userBytes);

        Item item;
        if (m_heap.RemoveBestFit({ 0, rawBytes }, item))
        {
            if (item.size > (rawBytes + 64))
            {
                m_heap.Insert({ item.offset + rawBytes, item.size - rawBytes });
                item.size = rawBytes;
            }
            else if (item.size < rawBytes)
            {
                m_heap.Insert(item);
                return nullptr;
            }
            Slice<u8> region = m_memory.Subslice(item.offset, item.size);
            return RawToUser(region.begin(), m_type, item.size, item.offset);
        }
        return nullptr;
    }

    void _Free(void* pUser)
    {
        using namespace Allocator;

        Header* pHeader = UserToHeader(pUser, m_type);
        m_heap.Insert({ pHeader->arg1, pHeader->size });
    }

    void* _Realloc(void* pOldUser, i32 userBytes)
    {
        using namespace Allocator;

        _Free(pOldUser);
        void* pNewUser = _Alloc(userBytes);
        if (!pNewUser)
        {
            return nullptr;
        }

        if (pNewUser != pOldUser)
        {
            Copy(UserToHeader(pNewUser, m_type), UserToHeader(pOldUser, m_type));
        }

        return pNewUser;
    }
};
