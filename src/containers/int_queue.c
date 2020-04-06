#include "containers/int_queue.h"
#include "common/nextpow2.h"
#include "allocator/allocator.h"

void intQ_create(intQ_t* q, EAlloc allocator)
{
    ASSERT(q);
    q->ptr = NULL;
    q->width = 0;
    q->iRead = 0;
    q->iWrite = 0;
    q->allocator = allocator;
}

void intQ_destroy(intQ_t* q)
{
    ASSERT(q);
    pim_free(q->ptr);
    q->ptr = NULL;
    q->width = 0;
    q->iRead = 0;
    q->iWrite = 0;
}

void intQ_clear(intQ_t* q)
{
    ASSERT(q);
    q->iRead = 0;
    q->iWrite = 0;
}

u32 intQ_size(const intQ_t* q)
{
    ASSERT(q);
    return q->iWrite - q->iRead;
}

u32 intQ_capacity(const intQ_t* q)
{
    ASSERT(q);
    return q->width;
}

void intQ_reserve(intQ_t* q, u32 capacity)
{
    ASSERT(q);
    capacity = capacity > 16 ? capacity : 16;
    const u32 newWidth = NextPow2(capacity);
    const u32 oldWidth = q->width;
    if (newWidth > oldWidth)
    {
        i32* oldPtr = q->ptr;
        i32* newPtr = pim_malloc(q->allocator, sizeof(*newPtr) * newWidth);
        const u32 iRead = q->iRead;
        const u32 len = q->iWrite - iRead;
        const u32 mask = oldWidth - 1u;
        for (u32 i = 0; i < len; ++i)
        {
            u32 j = (iRead + i) & mask;
            newPtr[i] = oldPtr[j];
        }
        pim_free(oldPtr);
        q->ptr = newPtr;
        q->width = newWidth;
        q->iRead = 0;
        q->iWrite = len;
    }
}

void intQ_push(intQ_t* q, i32 value)
{
    ASSERT(q);
    intQ_reserve(q, intQ_size(q) + 1);
    u32 mask = q->width - 1;
    u32 dst = q->iWrite++;
    q->ptr[dst & mask] = value;
}

bool intQ_trypop(intQ_t* q, i32* valueOut)
{
    ASSERT(valueOut);
    if (intQ_size(q))
    {
        u32 mask = q->width - 1;
        u32 src = q->iRead++;
        *valueOut = q->ptr[src & mask];
        return 1;
    }
    return 0;
}
