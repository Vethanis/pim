#pragma once

#include "containers/array.h"
#include "common/find.h"

struct ChunkAllocator
{
    struct Chunk
    {
        u8* memory;
        i32* freelist;
        i32 freelen;
    };

    Array<Chunk> m_chunks;
    i32 m_chunkSize;
    i32 m_itemSize;

    void Init(AllocType allocator, i32 chunkSize, i32 itemSize)
    {
        ASSERT(chunkSize > 0);
        ASSERT(itemSize > 0);
        m_chunks.Init(allocator);
        m_chunkSize = chunkSize;
        m_itemSize = itemSize;
    }

    void Reset()
    {
        for (Chunk chunk : m_chunks)
        {
            Allocator::Free(chunk.memory);
            Allocator::Free(chunk.freelist);
        }
        m_chunks.Reset();
    }

    void* Allocate()
    {
        {
            const i32 count = m_chunks.Size();
            Chunk* chunks = m_chunks.begin();
            for (i32 i = count - 1; i >= 0; --i)
            {
                if (chunks[i].freelen > 0)
                {
                    i32 j = --chunks[i].freelen;
                    return chunks[i].memory + j * m_itemSize;
                }
            }
        }
        {
            const AllocType allocator = m_chunks.m_allocType;
            const i32 chunkSize = m_chunkSize;
            const i32 itemSize = m_itemSize;
            ASSERT(chunkSize > 0);
            ASSERT(itemSize > 0);

            Chunk chunk = {};
            chunk.memory = Allocator::CallocT<u8>(
                allocator, chunkSize * itemSize);
            chunk.freelist = Allocator::AllocT<i32>(allocator, chunkSize);
            chunk.freelen = chunkSize;

            for (i32 i = 0; i < chunkSize; ++i)
            {
                chunk.freelist[i] = (chunkSize - 1) - i;
            }

            m_chunks.Grow() = chunk;
        }
        return Allocate();
    }

    void Free(void* pVoid)
    {
        if (!pVoid)
        {
            return;
        }

        u8* ptr = (u8*)pVoid;
        const i32 chunkSize = m_chunkSize;
        const i32 itemSize = m_itemSize;
        ASSERT(chunkSize > 0);
        ASSERT(itemSize > 0);

        Chunk* chunks = m_chunks.begin();
        const i32 chunkCt = m_chunks.Size();
        for (i32 i = chunkCt - 1; i >= 0; --i)
        {
            const u8* begin = chunks[i].memory;
            const u8* end = begin + chunkSize * itemSize;
            if ((ptr >= begin) && (ptr < end))
            {
                const i32 j = (i32)(ptr - begin) / itemSize;

                i32* freelist = chunks[i].freelist;
                i32 freelen = chunks[i].freelen;

                ASSERT(freelen < chunkSize);
                ASSERT(!Contains(freelist, freelen, j));

                freelist[freelen++] = j;
                chunks[i].freelen = freelen;

                if (freelen == chunkSize)
                {
                    Allocator::Free(chunks[i].memory);
                    Allocator::Free(chunks[i].freelist);
                    m_chunks.Remove(i);
                }
                else
                {
                    memset(ptr, 0, itemSize);
                }

                return;
            }
        }

        ASSERT(false);
    }
};
