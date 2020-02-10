#include "common/chunk_allocator.h"

#include "common/macro.h"
#include "allocator/allocator.h"
#include "os/atomics.h"
#include <string.h>

void ChunkAllocator::Init(AllocType allocator, i32 itemSize)
{
    ASSERT(itemSize > 0);
    m_head = 0;
    m_allocator = allocator;
    m_itemSize = itemSize;
}

void ChunkAllocator::Reset()
{
    Chunk* pChunk = LoadPtr(m_head, MO_Relaxed);
    if (pChunk && CmpExStrongPtr(m_head, pChunk, (Chunk*)0, MO_Acquire))
    {
        while (pChunk)
        {
            Chunk* pNext = LoadPtr(pChunk->pNext, MO_Relaxed);
            Allocator::Free(pChunk);
            pChunk = pNext;
        }
    }
}

void* ChunkAllocator::Allocate()
{
    const i32 itemSize = m_itemSize;
    ASSERT(itemSize);

tryalloc:
    Chunk* pChunk = LoadPtr(m_head, MO_Relaxed);
    while (pChunk)
    {
        u8* inUse = pChunk->inUse;
        for (i32 i = 0; i < kChunkSize; ++i)
        {
            u8 used = Load(inUse[i], MO_Relaxed);
            if (!used && CmpExStrong(inUse[i], used, 1, MO_Acquire))
            {
                u8* ptr = pChunk->GetItem(i, itemSize);
                memset(ptr, 0, itemSize);
                return ptr;
            }
        }
        pChunk = LoadPtr(pChunk->pNext);
    }

    Chunk* pNew = (Chunk*)Allocator::Calloc(m_allocator, sizeof(Chunk) + (itemSize * kChunkSize));

tryinsert:
    Chunk* pHead = LoadPtr(m_head);
    pNew->pNext = pHead;
    if (!CmpExStrongPtr(m_head, pHead, pNew))
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
        Chunk* pChunk = LoadPtr(m_head, MO_Relaxed);
        while (pChunk)
        {
            const isize i = pChunk->IndexOf(ptr, itemSize);
            if ((i >= 0) && (i < kChunkSize))
            {
                u8 used = Load(pChunk->inUse[i], MO_Relaxed);
                ASSERT(used);
                bool released = CmpExStrong(pChunk->inUse[i], used, 0, MO_Release);
                ASSERT(released);
            }
            pChunk = LoadPtr(pChunk->pNext, MO_Relaxed);
        }
    }
}
