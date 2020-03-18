#include "common/chunk_allocator.h"

#include "common/macro.h"
#include "allocator/allocator.h"
#include "common/atomics.h"
#include <string.h>

void ChunkAllocator::Init(EAlloc allocator, i32 itemSize)
{
    ASSERT(itemSize > 0);
    m_head = 0;
    m_allocator = allocator;
    m_itemSize = itemSize;
}

void ChunkAllocator::Reset()
{
    Chunk* pChunk = LoadPtr(Chunk, m_head, MO_Relaxed);
    if (pChunk && CmpExPtr(Chunk, m_head, pChunk, 0, MO_Acquire))
    {
        while (pChunk)
        {
            Chunk* pNext = LoadPtr(Chunk, pChunk->pNext, MO_Relaxed);
            CAllocator.Free(pChunk);
            pChunk = pNext;
        }
    }
}

void* ChunkAllocator::Allocate()
{
    const i32 itemSize = m_itemSize;
    ASSERT(itemSize);

tryalloc:
    Chunk* pChunk = LoadPtr(Chunk, m_head, MO_Relaxed);
    while (pChunk)
    {
        u8* inUse = pChunk->inUse;
        for (i32 i = 0; i < kChunkSize; ++i)
        {
            u8 used = load_u8(inUse + i, MO_Relaxed);
            if (!used && cmpex_u8(inUse + i, &used, 1, MO_Acquire))
            {
                u8* ptr = pChunk->GetItem(i, itemSize);
                memset(ptr, 0, itemSize);
                return ptr;
            }
        }
        pChunk = LoadPtr(Chunk, pChunk->pNext, MO_Relaxed);
    }

    Chunk* pNew = (Chunk*)CAllocator.Calloc(m_allocator, sizeof(Chunk) + (itemSize * kChunkSize));

tryinsert:
    Chunk* pHead = LoadPtr(Chunk, m_head, MO_Relaxed);
    pNew->pNext = pHead;
    if (!CmpExPtr(Chunk, m_head, pHead, pNew, MO_Acquire))
    {
        goto tryinsert;
    }
    goto tryalloc;
}

void ChunkAllocator::Free(void* pVoid)
{
    const i32 itemSize = m_itemSize;
    ASSERT(itemSize);

    if (pVoid)
    {
        u8* ptr = (u8*)pVoid;
        Chunk* pChunk = LoadPtr(Chunk, m_head, MO_Relaxed);
        while (pChunk)
        {
            const isize i = pChunk->IndexOf(ptr, itemSize);
            if ((i >= 0) && (i < kChunkSize))
            {
                u8 used = load_u8(pChunk->inUse + i, MO_Relaxed);
                ASSERT(used);
                bool released = cmpex_u8(pChunk->inUse + i, &used, 0, MO_Release);
                ASSERT(released);
            }
            pChunk = LoadPtr(Chunk, pChunk->pNext, MO_Relaxed);
        }
    }
}
