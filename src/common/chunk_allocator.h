#pragma once

#include "containers/array.h"
#include "containers/queue.h"
#include "os/thread.h"

struct ChunkAllocator
{
    OS::Mutex m_lock;
    Array<u8*> m_chunks;
    Queue<u8*> m_freelist;
    i32 m_chunkSize;
    i32 m_itemSize;

    static constexpr i32 InitialChunkSize = 16;

    static i32 PtrCmp(u8* const & lhs, u8* const & rhs)
    {
        // All allocators each use a contiguous range of memory that is <= 2GB.
        return (i32)(lhs - rhs);
    }

    void Init(AllocType allocator, i32 itemSize)
    {
        ASSERT(itemSize > 0);
        m_lock.Open();
        m_chunks.Init(allocator);
        m_freelist.Init(allocator);
        m_chunkSize = InitialChunkSize;
        m_itemSize = itemSize;
    }

    void Shutdown()
    {
        m_lock.Lock();
        for (u8* chunk : m_chunks)
        {
            Allocator::Free(chunk);
        }
        m_chunks.Reset();
        m_freelist.Reset();
        m_chunkSize = InitialChunkSize;
        m_lock.Unlock();

        m_lock.Close();
    }

    void* Allocate()
    {
        OS::LockGuard guard(m_lock);
        if (m_freelist.IsEmpty())
        {
            const i32 chunkSize = m_chunkSize;
            const i32 itemSize = m_itemSize;
            ASSERT(itemSize > 0);
            ASSERT(chunkSize > 0);
            u8* chunk = Allocator::AllocT<u8>(m_chunks.GetAllocator(), chunkSize * itemSize);
            m_chunks.Grow() = chunk;
            m_chunkSize = chunkSize * 2;

            Queue<u8*> freelist = m_freelist;
            for (i32 i = 0; i < chunkSize; ++i)
            {
                freelist.Push(chunk + i * itemSize);
            }
            m_freelist = freelist;
        }

        void* ptr = m_freelist.Pop();
        ASSERT(ptr);
        memset(ptr, 0, m_itemSize);
        return ptr;
    }

    void Free(void* pVoid)
    {
        if (!pVoid)
        {
            return;
        }

        OS::LockGuard guard(m_lock);
        m_freelist.Push((u8*)pVoid, { PtrCmp });
    }
};
