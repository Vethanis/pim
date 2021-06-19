#include "containers/queue.h"
#include "common/nextpow2.h"
#include "allocator/allocator.h"

#include <string.h>

void Queue_New(Queue* q, u32 valueSize, EAlloc allocator)
{
    ASSERT(q);
    ASSERT(valueSize > 0);
    ASSERT(valueSize <= 256);
    memset(q, 0, sizeof(*q));
    q->valueSize = valueSize;
    q->allocator = allocator;
}

void Queue_Del(Queue* q)
{
    ASSERT(q);
    Mem_Free(q->ptr);
    memset(q, 0, sizeof(*q));
}

u32 Queue_Size(const Queue* q)
{
    ASSERT(q);
    return q->iWrite - q->iRead;
}

u32 Queue_Capacity(const Queue* q)
{
    ASSERT(q);
    return q->width;
}

void Queue_Clear(Queue* q)
{
    ASSERT(q);
    q->iRead = 0;
    q->iWrite = 0;
}

void Queue_Reserve(Queue* q, u32 capacity)
{
    ASSERT(q);
    capacity = capacity > 16 ? capacity : 16;
    const u32 newWidth = NextPow2(capacity);
    const u32 oldWidth = q->width;
    if (newWidth > oldWidth)
    {
        const u32 valueSize = q->valueSize;
        ASSERT(valueSize > 0);
        u8* oldPtr = q->ptr;
        u8* newPtr = Mem_Alloc(q->allocator, valueSize * newWidth);
        const u32 iRead = q->iRead;
        const u32 len = q->iWrite - iRead;
        if (len)
        {
            const u32 mask = oldWidth - 1u;
            const u32 twist = (iRead & mask) * valueSize;
            const u32 untwist = (oldWidth * valueSize) - twist;
            memcpy(newPtr, oldPtr + twist, untwist);
            memcpy(newPtr + untwist, oldPtr, twist);
        }
        Mem_Free(oldPtr);
        q->ptr = newPtr;
        q->width = newWidth;
        q->iRead = 0;
        q->iWrite = len;
    }
}

void Queue_Push(Queue* q, const void* src, u32 itemSize)
{
    ASSERT(q);
    ASSERT(src);
    const u32 valueSize = q->valueSize;
    ASSERT(itemSize == valueSize);
    Queue_Reserve(q, Queue_Size(q) + 1);
    u8* dst = q->ptr;
    const u32 mask = q->width - 1;
    const u32 iWrite = q->iWrite++ & mask;
    memcpy(dst + iWrite * valueSize, src, valueSize);
}

void Queue_PushFront(Queue* q, const void* src, u32 itemSize)
{
    ASSERT(q);
    ASSERT(src);
    const u32 valueSize = q->valueSize;
    ASSERT(itemSize == valueSize);

    Queue_Reserve(q, Queue_Size(q) + 1);
    q->iWrite++;

    const u32 mask = q->width - 1u;
    const u32 len = Queue_Size(q);
    const u32 iRead = q->iRead;
    u8* ptr = q->ptr;

    for (u32 i = len; i != 0; --i)
    {
        u32 iSrc = (iRead + i - 2u) & mask;
        u32 iDst = (iRead + i - 1u) & mask;
        memmove(&ptr[iDst * valueSize], &ptr[iSrc * valueSize], valueSize);
    }

    u32 iDst = iRead & mask;
    memcpy(&ptr[iDst * valueSize], src, valueSize);
}

bool Queue_TryPop(Queue* q, void* dst, u32 itemSize)
{
    ASSERT(q);
    ASSERT(dst);
    const u32 valueSize = q->valueSize;
    ASSERT(itemSize == valueSize);
    if (Queue_Size(q))
    {
        const u8* src = q->ptr;
        const u32 mask = q->width - 1;
        const u32 iRead = q->iRead++ & mask;
        memcpy(dst, src + iRead * valueSize, valueSize);
        return true;
    }
    return false;
}

void Queue_Get(Queue* q, u32 i, void* dst, u32 itemSize)
{
    ASSERT(i < Queue_Size(q));
    ASSERT(dst);
    const u32 valueSize = q->valueSize;
    ASSERT(itemSize == valueSize);
    const u8* src = q->ptr;
    const u32 mask = q->width - 1;
    const u32 iRead = q->iRead;
    const u32 j = (iRead + i) & mask;
    memcpy(dst, src + j * valueSize, valueSize);
}
