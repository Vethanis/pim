#pragma once

#include "common/int_types.h"

struct ChunkAllocator
{
    static constexpr i32 kChunkSize = 256;

    struct Chunk
    {
        Chunk* pNext;
        isize _pad;
        u8 inUse[kChunkSize];

        u8* GetItems()
        {
            u8* ptr = (u8*)this;
            return ptr + sizeof(Chunk);
        }

        u8* GetItem(i32 i, i32 itemSize)
        {
            return GetItems() + i * itemSize;
        }

        isize IndexOf(u8* ptr, i32 itemSize)
        {
            u8* items = GetItems();
            return (ptr - items) / itemSize;
        }
    };

    Chunk* m_head;
    AllocType m_allocator;
    i32 m_itemSize;

    void Init(AllocType allocator, i32 itemSize);
    void Reset();
    void* Allocate();
    void Free(void* pVoid);
};

template<typename T>
struct TPool
{
    ChunkAllocator m_allocator;

    TPool()
    {
        m_allocator.Init(Alloc_Perm, sizeof(T));
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
