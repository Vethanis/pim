#include "containers/queue.h"
#include "common/nextpow2.h"
#include <string.h>

void queue_create(queue_t* q, uint32_t itemSize, EAlloc allocator)
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
    q->ptr = NULL;
    q->width = 0;
    q->iRead = 0;
    q->iWrite = 0;
}

uint32_t queue_size(const queue_t* q)
{
    ASSERT(q);
    return q->iWrite - q->iRead;
}

uint32_t queue_capacity(const queue_t* q)
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

void queue_reserve(queue_t* q, uint32_t capacity)
{
    ASSERT(q);
    capacity = capacity > 16 ? capacity : 16;
    const uint32_t newWidth = NextPow2(capacity);
    const uint32_t oldWidth = q->width;
    if (newWidth > oldWidth)
    {
        const uint32_t stride = q->stride;
        ASSERT(stride > 0);
        uint8_t* oldPtr = q->ptr;
        uint8_t* newPtr = pim_malloc(q->allocator, stride * newWidth);
        const uint32_t iRead = q->iRead;
        const uint32_t len = q->iWrite - iRead;
        if (len)
        {
            const uint32_t mask = oldWidth - 1u;
            const uint32_t twist = (iRead & mask) * stride;
            const uint32_t untwist = (oldWidth * stride) - twist;
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

void queue_push(queue_t* q, void* src, uint32_t itemSize)
{
    ASSERT(q);
    ASSERT(src);
    const uint32_t stride = q->stride;
    ASSERT(itemSize == stride);
    queue_reserve(q, queue_size(q) + 1);
    uint8_t* dst = q->ptr;
    const uint32_t mask = q->width - 1;
    const uint32_t iWrite = q->iWrite++ & mask;
    memcpy(dst + iWrite * stride, src, stride);
}

bool queue_trypop(queue_t* q, void* dst, uint32_t itemSize)
{
    ASSERT(q);
    ASSERT(dst);
    const uint32_t stride = q->stride;
    ASSERT(itemSize == stride);
    if (queue_size(q))
    {
        const uint8_t* src = q->ptr;
        const uint32_t mask = q->width - 1;
        const uint32_t iRead = q->iRead++ & mask;
        memcpy(dst, src + iRead * stride, stride);
        return true;
    }
    return false;
}

void queue_get(queue_t* q, uint32_t i, void* dst, uint32_t itemSize)
{
    ASSERT(i < queue_size(q));
    ASSERT(dst);
    const uint32_t stride = q->stride;
    ASSERT(itemSize == stride);
    const uint8_t* src = q->ptr;
    const uint32_t mask = q->width - 1;
    const uint32_t iRead = q->iRead;
    const uint32_t j = (iRead + i) & mask;
    memcpy(dst, src + j * stride, stride);
}
