#include "containers/int_queue.h"
#include "allocator/allocator.h"

void intQ_create(intQ_t* q)
{
    ASSERT(q);
    q->ptr = 0;
    q->width = 0;
    q->iRead = 0;
    q->iWrite = 0;
}

void intQ_destroy(intQ_t* q)
{
    ASSERT(q);
    pim_free(q->ptr);
    q->ptr = 0;
    q->width = 0;
    q->iRead = 0;
    q->iWrite = 0;
}

void intQ_clear(intQ_t* q)
{
    ASSERT(q);
    q->iRead = 0;
    q->iRead = 0;
}

uint32_t intQ_size(const intQ_t* q)
{
    ASSERT(q);
    return q->iWrite - q->iRead;
}

uint32_t intQ_capacity(const intQ_t* q)
{
    ASSERT(q);
    return q->width;
}

static uint32_t NextPow2(uint32_t x)
{
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    ++x;
    return x;
}

void intQ_reserve(intQ_t* q, uint32_t capacity)
{
    ASSERT(q);
    capacity = capacity > 16 ? capacity : 16;
    const uint32_t newWidth = NextPow2(capacity);
    const uint32_t oldWidth = q->width;
    if (newWidth > oldWidth)
    {
        int32_t* oldPtr = q->ptr;
        int32_t* newPtr = pim_malloc(EAlloc_Perm, sizeof(*newPtr) * newWidth);
        const uint32_t iRead = q->iRead;
        const uint32_t len = q->iWrite - iRead;
        const uint32_t mask = oldWidth - 1u;
        for (uint32_t i = 0; i < len; ++i)
        {
            uint32_t j = (iRead + i) & mask;
            newPtr[i] = oldPtr[j];
        }
        pim_free(oldPtr);
        q->ptr = newPtr;
        q->width = newWidth;
        q->iRead = 0;
        q->iWrite = len;
    }
}

void intQ_push(intQ_t* q, int32_t value)
{
    ASSERT(q);
    intQ_reserve(q, intQ_size(q) + 1);
    uint32_t mask = q->width - 1;
    uint32_t dst = q->iWrite++;
    q->ptr[dst & mask] = value;
}

int32_t intQ_trypop(intQ_t* q, int32_t* valueOut)
{
    ASSERT(valueOut);
    if (intQ_size(q))
    {
        uint32_t mask = q->width - 1;
        uint32_t src = q->iRead++;
        *valueOut = q->ptr[src & mask];
        return 1;
    }
    return 0;
}
