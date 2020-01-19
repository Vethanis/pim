#pragma once

#include "common/chunk_allocator.h"
#include <new>

template<typename T>
struct ObjPool
{
    ChunkAllocator m_chunks;

    void Init()
    {
        m_chunks.Init(Alloc_Pool, sizeof(T));
    }
    void Shutdown()
    {
        m_chunks.Shutdown();
    }
    T* New()
    {
        void* pMem = m_chunks.Allocate();
        return new (pMem) T();
    }
    void Delete(T* ptr)
    {
        if (ptr)
        {
            ptr->~T();
            m_chunks.Free(ptr);
        }
    }
};
