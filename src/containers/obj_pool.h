#pragma once

#include "common/chunk_allocator.h"
#include <new>

template<typename T>
struct ObjPool
{
    ChunkAllocator m_chunks;

    ObjPool() { Init(); }
    ~ObjPool() { Reset(); }

    void Init()
    {
        m_chunks.Init(Alloc_Pool, sizeof(T));
    }
    void Reset()
    {
        m_chunks.Reset();
    }

    T* New() { return new (m_chunks.Allocate()) T(); }
    void Delete(T* ptr) { if (ptr) { ptr->~T(); m_chunks.Free(ptr); } }
};

#define DECLARE_POOL(T) \
    static ObjPool<T> ms_pool; \
    static T* PoolNew() { return ms_pool.New(); } \
    static void PoolDelete(T* ptr) { ms_pool.Delete(ptr); }

#define DEFINE_POOL(T) ObjPool<T> T::ms_pool;
