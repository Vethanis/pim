#pragma once

#include "containers/array.h"
#include "common/find.h"
#include "common/minmax.h"

struct ChunkAllocator
{
    u8** m_chunks;
    i32** m_freelists;
    i32* m_freelens;
    i32 m_length;
    i32 m_capacity;
    AllocType m_allocator;

    i32 m_chunkSize;
    i32 m_itemSize;

    void Init(AllocType allocator, i32 itemSize)
    {
        memset(this, 0, sizeof(*this));
        ASSERT(itemSize > 0);
        m_chunkSize = Max(1, 4096 / itemSize);
        m_itemSize = itemSize;
        m_allocator = allocator;
    }

    void Reset()
    {
        for (i32 i = 0; i < m_length; ++i)
        {
            Allocator::Free(m_chunks[i]);
            Allocator::Free(m_freelists[i]);
        }
        Allocator::Free(m_chunks);
        Allocator::Free(m_freelists);
        Allocator::Free(m_freelens);
        m_chunks = 0;
        m_freelists = 0;
        m_freelens = 0;
        m_length = 0;
        m_capacity = 0;
    }

    void* Allocate()
    {
        {
            u8** chunks = m_chunks;
            i32** freelists = m_freelists;
            i32* freelens = m_freelens;
            const i32 length = m_length;
            for (i32 i = length - 1; i >= 0; --i)
            {
                if (freelens[i] > 0)
                {
                    i32 back = --freelens[i];
                    i32 j = freelists[i][back];
                    return chunks[i] + j * m_itemSize;
                }
            }
        }
        {
            const AllocType allocator = m_allocator;
            const i32 length = ++m_length;

            {
                const i32 oldCap = m_capacity;
                i32 newCap = oldCap;
                m_chunks = Allocator::Reserve(allocator, m_chunks, newCap, length);
                newCap = oldCap;
                m_freelists = Allocator::Reserve(allocator, m_freelists, newCap, length);
                newCap = oldCap;
                m_freelens = Allocator::Reserve(allocator, m_freelens, newCap, length);
                m_capacity = newCap;
            }

            const i32 chunkSize = m_chunkSize;
            ASSERT(chunkSize > 0);

            i32* freelist = Allocator::AllocT<i32>(allocator, chunkSize);
            for (i32 i = 0; i < chunkSize; ++i)
            {
                freelist[i] = i;
            }

            const i32 itemSize = m_itemSize;
            ASSERT(itemSize > 0);

            m_chunks[length - 1] = Allocator::CallocT<u8>(allocator, chunkSize * itemSize);
            m_freelists[length - 1] = freelist;
            m_freelens[length - 1] = chunkSize;
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
        const i32 memSize = chunkSize * itemSize;
        ASSERT(memSize > 0);

        u8** chunks = m_chunks;
        i32** freelists = m_freelists;
        i32* freelens = m_freelens;
        const i32 length = m_length;
        for (i32 i = length - 1; i >= 0; --i)
        {
            const u8* begin = chunks[i];
            const u8* end = begin + memSize;
            if ((ptr >= begin) && (ptr < end))
            {
                const i32 j = (i32)(ptr - begin) / itemSize;
                ASSERT((u32)j < (u32)chunkSize);

                const i32 back = freelens[i]++;
                ASSERT(back < chunkSize);

                ASSERT(!Contains(freelists[i], back, j));
                freelists[i][back] = j;

                if (back == (chunkSize - 1))
                {
                    Allocator::Free(chunks[i]);
                    Allocator::Free(freelists[i]);
                    chunks[i] = chunks[length - 1];
                    freelists[i] = freelists[length - 1];
                    freelens[i] = freelens[length - 1];
                    --m_length;
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
