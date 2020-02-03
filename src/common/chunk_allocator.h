#pragma once

#include "containers/mtqueue.h"

struct ChunkAllocator
{
    static constexpr i32 kChunkSize = 256;

    MtQueue<void*> m_free;
    MtQueue<void*> m_chunks;
    i32 m_itemSize;

    void Init(AllocType allocator, i32 itemSize)
    {
        ASSERT(itemSize > 0);
        m_chunks.Init(allocator);
        m_free.Init(allocator);
        m_chunks.Reserve(1);
        m_free.Reserve(kChunkSize);
        m_itemSize = itemSize;
    }

    void Reset()
    {
        m_free.Reset();
        void* ptr = nullptr;
        while (m_chunks.TryPop(ptr))
        {
            Allocator::Free(ptr);
        }
        m_chunks.Reset();
    }

    void PushChunk()
    {
        const i32 itemSize = m_itemSize;
        ASSERT(itemSize > 0);

        u8* ptr = (u8*)Allocator::Alloc(m_chunks.GetAllocator(), kChunkSize * itemSize);

        m_chunks.Push(ptr);

        for (i32 i = 0; i < kChunkSize; ++i)
        {
            m_free.Push(ptr);
            ptr += itemSize;
        }
    }

    void* Allocate()
    {
        void* ptr = nullptr;
        while (!m_free.TryPop(ptr))
        {
            PushChunk();
        }
        memset(ptr, 0, m_itemSize);
        return ptr;
    }

    void Free(void* pVoid)
    {
        if (pVoid)
        {
            m_free.Push(pVoid);
        }
    }
};

template<typename T>
struct TPool
{
    ChunkAllocator m_allocator;

    TPool()
    {
        m_allocator.Init(Alloc_Pool, sizeof(T));
    }
    ~TPool()
    {
        m_allocator.Reset();
    }

    T* Allocate()
    {
        return (T*)m_allocator.Allocate();
    }

    void Free(T* ptr)
    {
        m_allocator.Free(ptr);
    }
};

#define DECLARE_TPOOL(T) \
    static TPool<T> ms_pool; \
    static T* PoolAlloc() { return ms_pool.Allocate(); } \
    static void PoolFree(T* ptr) { ms_pool.Free(ptr); }

#define DEFINE_TPOOL(T) TPool<T> T::ms_pool;
