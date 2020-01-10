#pragma once

#include "common/int_types.h"
#include "os/atomics.h"
#include <stdlib.h>

template<typename T>
struct Box
{
    T Value;
    a32 RefCount;

    inline u32 Inc() { return ::Inc(RefCount, MO_AcqRel); }
    inline u32 Dec() { return ::Dec(RefCount, MO_AcqRel); }
    inline u32 Refs() const { return ::Load(RefCount, MO_AcqRel); }
};

template<typename T>
struct Rc
{
    Box<T>* Ptr;

    inline void Inc()
    {
        Box<T>* ptr = Ptr;
        if (ptr)
        {
            u32 prevCount = ptr->Inc();
            ASSERT(prevCount > 0);
        }
    }

    inline void Dec()
    {
        Box<T>* ptr = Ptr;
        Ptr = 0;
        if (ptr)
        {
            u32 prevCount = ptr->Dec();
            ASSERT(prevCount > 0);
        }
    }

    inline T* Get()
    {
        Box<T>* ptr = Ptr;
        if (ptr)
        {
            ASSERT(ptr->Refs() > 0u);
            return &(ptr->Value);
        }
        return 0;
    }

    Rc() : Ptr(0) {}
    Rc(Rc other) : Ptr(other.Ptr)
    {
        Inc();
    }
    Rc& operator=(Rc other)
    {
        other.Inc();
        Dec();
        Ptr = other.Ptr;
    }
    ~Rc()
    {
        Dec();
    }
};

template<typename T>
struct Pool
{
    static constexpr i32 ChunkSize = 256;

    struct Chunk
    {
        Box<T> items[ChunkSize];
        aPtr pNext;
    };

    aPtr m_head;

    Rc<T> Alloc()
    {
        Rc<T> dst = {};
        Chunk* pChunk = (Chunk*)Load(m_head, MO_Relaxed);
        while (pChunk)
        {
            Box<T>* items = pChunk->items;
            for (i32 i = 0; i < ChunkSize; ++i)
            {
                a32& rc = items[i].RefCount;
                if (CmpEx(rc, 0, 1, MO_AcqRel) == 0)
                {
                    dst.Ptr = items + i;
                    return dst;
                }
            }
            pChunk = (Chunk*)Load(pChunk->pNext, MO_Relaxed);
        }

        Chunk* myHead = (Chunk*)calloc(sizeof(Chunk));
        ASSERT(myHead);

        Chunk* oldHead = (Chunk*)Load(m_head, MO_Relaxed);
        while (true)
        {
            myHead->pNext = oldHead;

            Chunk* newHead = (Chunk*)CmpEx(m_head, oldHead, myHead, MO_AcqRel);
            if (newHead == oldHead)
            {
                break;
            }
            else
            {
                oldHead = newHead;
            }
        }

        ASSERT(myHead->pNext != myHead);

        return Alloc();
    }
};
