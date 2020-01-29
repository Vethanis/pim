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
    void Reset()
    {
        m_chunks.Reset();
    }

    T* New() { return new (m_chunks.Allocate()) T(); }
    void Delete(T* ptr) { if (ptr) { ptr->~T(); m_chunks.Free(ptr); } }
};
