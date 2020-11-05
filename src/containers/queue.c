#include "containers/queue.h"
#include "common/nextpow2.h"
#include "allocator/allocator.h"

#include <string.h>

void queue_create(queue_t* q, u32 itemSize, EAlloc allocator)
{
    ASSERT(q);
    ASSERT(itemSize > 0);
    ASSERT(itemSize <= 256);
    memset(q, 0, sizeof(*q));
    q->stride = itemSize;
    q->allocator = allocator;
}

void queue_destroy(queue_t* q)
{
    ASSERT(q);
    pim_free(q->ptr);
    memset(q, 0, sizeof(*q));
}

u32 queue_size(const queue_t* q)
{
    ASSERT(q);
    return q->iWrite - q->iRead;
}

u32 queue_capacity(const queue_t* q)
{
    ASSERT(q);
    return q->width;
}

void queue_clear(queue_t* q)
{
    ASSERT(q);
    q->iRead = 0;
    q->iWrite = 0;
}

void queue_reserve(queue_t* q, u32 capacity)
{
    ASSERT(q);
    capacity = capacity > 16 ? capacity : 16;
    const u32 newWidth = NextPow2(capacity);
    const u32 oldWidth = q->width;
    if (newWidth > oldWidth)
    {
        const u32 stride = q->stride;
        ASSERT(stride > 0);
        u8* oldPtr = q->ptr;
        u8* newPtr = pim_malloc(q->allocator, stride * newWidth);
        const u32 iRead = q->iRead;
        const u32 len = q->iWrite - iRead;
        if (len)
        {
            const u32 mask = oldWidth - 1u;
            const u32 twist = (iRead & mask) * stride;
            const u32 untwist = (oldWidth * stride) - twist;
            memcpy(newPtr, oldPtr + twist, untwist);
            memcpy(newPtr + untwist, oldPtr, twist);
        }
        pim_free(oldPtr);
        q->ptr = newPtr;
        q->width = newWidth;
        q->iRead = 0;
        q->iWrite = len;
    }
}

void queue_push(queue_t* q, const void* src, u32 itemSize)
{
    ASSERT(q);
    ASSERT(src);
    const u32 stride = q->stride;
    ASSERT(itemSize == stride);
    queue_reserve(q, queue_size(q) + 1);
    u8* dst = q->ptr;
    const u32 mask = q->width - 1;
    const u32 iWrite = q->iWrite++ & mask;
    memcpy(dst + iWrite * stride, src, stride);
}

void queue_pushfront(queue_t* q, const void* src, u32 itemSize)
{
    ASSERT(q);
    ASSERT(src);
    const u32 stride = q->stride;
    ASSERT(itemSize == stride);

    queue_reserve(q, queue_size(q) + 1);
    q->iWrite++;

    const u32 mask = q->width - 1;
    const u32 len = queue_size(q);
    const u32 iRead = q->iRead;
    u8* ptr = q->ptr;

    for (u32 i = 1; i < len; ++i)
    {
        u32 iSrc = (iRead + i - 1) & mask;
        u32 iDst = (iRead + i) & mask;
        memcpy(ptr + iDst * stride, ptr + iSrc * stride, stride);
    }

    u32 iDst = iRead & mask;
    memcpy(ptr + iDst * stride, src, stride);
}

bool queue_trypop(queue_t* q, void* dst, u32 itemSize)
{
    ASSERT(q);
    ASSERT(dst);
    const u32 stride = q->stride;
    ASSERT(itemSize == stride);
    if (queue_size(q))
    {
        const u8* src = q->ptr;
        const u32 mask = q->width - 1;
        const u32 iRead = q->iRead++ & mask;
        memcpy(dst, src + iRead * stride, stride);
        return true;
    }
    return false;
}

void queue_get(queue_t* q, u32 i, void* dst, u32 itemSize)
{
    ASSERT(i < queue_size(q));
    ASSERT(dst);
    const u32 stride = q->stride;
    ASSERT(itemSize == stride);
    const u8* src = q->ptr;
    const u32 mask = q->width - 1;
    const u32 iRead = q->iRead;
    const u32 j = (iRead + i) & mask;
    memcpy(dst, src + j * stride, stride);
}
