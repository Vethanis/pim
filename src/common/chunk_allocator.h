#pragma once

#include "containers/array.h"
#include "containers/queue.h"

struct ChunkAllocator
{
    Array<u8*> m_chunks;
    Queue<u8*> m_freelist;
    i32 m_chunkSize;
    i32 m_itemSize;

    void Init(AllocType allocator, i32 itemSize)
    {
        ASSERT(itemSize > 0);
        m_chunks.Init(allocator);
        m_freelist.Init(allocator);
        m_chunkSize = 64;
        m_itemSize = itemSize;
    }

    void Reset()
    {
        for (u8* chunk : m_chunks)
        {
            Allocator::Free(chunk);
        }
        m_chunks.Reset();
        m_freelist.Reset();
        m_chunkSize = 64;
    }

    void* Allocate()
    {
        if (m_freelist.IsEmpty())
        {
            const i32 chunkSize = m_chunkSize;
            const i32 itemSize = m_itemSize;
            const AllocType allocator = m_chunks.GetAllocType();
            ASSERT(itemSize > 0);
            ASSERT(chunkSize > 0);
            u8* chunk = Allocator::AllocT<u8>(allocator, chunkSize * itemSize);
            m_chunks.Grow() = chunk;
            m_chunkSize = chunkSize * 2;

            Queue<u8*> freelist = m_freelist;
            for (i32 i = 0; i < chunkSize; ++i)
            {
                freelist.Push(chunk + i * itemSize);
            }
            m_freelist = freelist;
        }
        return m_freelist.Pop();
    }

    void Free(void* pVoid)
    {
        if (!pVoid)
        {
            return;
        }

        m_freelist.Push((u8*)pVoid, OpComparable<u8*>());
    }
};
