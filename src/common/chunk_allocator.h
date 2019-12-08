#pragma once

#include "containers/array.h"

template<typename T>
struct ChunkAllocator
{
    Array<T*> m_chunks;
    Array<Array<u16>> m_freelists;
    u16 m_chunkSize;

    void Init(u16 chunkSize)
    {
        Assert(chunkSize > 0);
        m_chunks.Init(Alloc_Stdlib);
        m_freelists.Init(Alloc_Stdlib);
        m_chunkSize = chunkSize;
    }

    void Reset()
    {
        const u16 chunkSize = m_chunkSize;
        for (T* ptr : m_chunks)
        {
            AllocationT<T> alloc = { ptr, chunkSize, Alloc_Stdlib };
            Allocator::FreeT(alloc);
        }
        m_chunks.Reset();
        for (Array<u16> freelist : m_freelists)
        {
            freelist.Reset();
        }
        m_freelists.Reset();
    }

    T* Allocate()
    {
        {
            const i32 chunkCt = m_freelists.Size();
            Array<u16>* freelists = m_freelists.begin();
            for (i32 i = chunkCt - 1; i >= 0; --i)
            {
                if (!freelists[i].IsEmpty())
                {
                    u16 j = freelists[i].PopValue();
                    return m_chunks[i] + j;
                }
            }
        }
        {
            const u16 chunkSize = m_chunkSize;
            Assert(chunkSize > 0);

            m_chunks.Grow() = Allocator::AllocT<T>(Alloc_Stdlib, chunkSize).begin();
            MemClear(m_chunks.back(), sizeof(T) * chunkSize);

            Array<u16> freelist = {};
            freelist.Init(Alloc_Stdlib);
            freelist.Resize(chunkSize);
            for (u16 i = 0; i < chunkSize; ++i)
            {
                freelist[i] = (chunkSize - 1u) - i;
            }
            m_freelists.Grow() = freelist;
        }
        return Allocate();
    }

    void Free(T* ptr)
    {
        if (!ptr)
        {
            return;
        }

        const i32 chunkCt = m_chunks.Size();
        const u16 chunkSize = m_chunkSize;
        Assert(chunkSize > 0);

        T** chunks = m_chunks.begin();
        for (i32 i = chunkCt - 1; i >= 0; --i)
        {
            T* chunk = chunks[i];
            T* end = chunk + chunkSize;
            if ((ptr >= chunk) && (ptr < end))
            {
                u16 j = (u16)(ptr - chunk);
                Array<u16>& freelist = m_freelists[i];

                Assert(freelist.Size() < (i32)chunkSize);
                Assert(freelist.RFind(j) == -1);
                freelist.Grow() = j;

                if (freelist.Size() == chunkSize)
                {
                    freelist.Reset();
                    AllocationT<T> alloc = { chunk, chunkSize, Alloc_Stdlib };
                    Allocator::FreeT(alloc);
                    m_chunks.Remove(i);
                    m_freelists.Remove(i);
                }
                else
                {
                    MemClear(ptr, sizeof(T));
                }

                return;
            }
        }

        Assert(false);
    }
};
