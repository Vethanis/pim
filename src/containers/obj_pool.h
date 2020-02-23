#pragma once

#include "common/chunk_allocator.h"
#include <new>

template<typename T>
struct ObjPool
{
    ChunkAllocator m_chunks;

    void Init()
    {
        m_chunks.Init(Alloc_Perm, sizeof(T));
    }
    void Reset()
    {
        m_chunks.Reset();
    }

    T* New() { return new (m_chunks.Allocate()) T(); }
    void Delete(T* ptr) { if (ptr) { ptr->~T(); m_chunks.Free(ptr); } }
};

template<typename T, typename Args>
struct ObjPoolEx
{
    ChunkAllocator m_chunks;

    void Init()
    {
        m_chunks.Init(Alloc_Perm, sizeof(T));
    }
    void Reset()
    {
        m_chunks.Reset();
    }

    T* New(Args args) { return new (m_chunks.Allocate()) T(args); }
    void Delete(T* ptr) { if (ptr) { ptr->~T(); m_chunks.Free(ptr); } }
};
