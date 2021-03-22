#include "containers/ptrqueue.h"
#include "common/atomics.h"
#include "common/nextpow2.h"
#include "allocator/allocator.h"

void PtrQueue_New(PtrQueue *const pq, EAlloc allocator, u32 capacity)
{
    ASSERT(pq);
    pq->iWrite = 0;
    pq->iRead = 0;
    pq->width = 0;
    pq->ptr = NULL;
    pq->allocator = allocator;
    ASSERT(capacity);

    const u32 width = NextPow2(capacity);
    pq->width = width;
    pq->ptr = Mem_Calloc(allocator, sizeof(pq->ptr[0]) * width);
}

void PtrQueue_Del(PtrQueue *const pq)
{
    Mem_Free(pq->ptr);
    pq->ptr = NULL;
    store_u32(&(pq->width), 0, MO_Release);
    store_u32(&(pq->iWrite), 0, MO_Release);
    store_u32(&(pq->iRead), 0, MO_Release);
}

u32 PtrQueue_Capacity(PtrQueue const *const pq)
{
    ASSERT(pq);
    return load_u32(&pq->width, MO_Acquire);
}

u32 PtrQueue_Size(PtrQueue const *const pq)
{
    ASSERT(pq);
    return load_u32(&pq->iWrite, MO_Acquire) - load_u32(&pq->iRead, MO_Acquire);
}

bool PtrQueue_TryPush(PtrQueue *const pq, void *const pValue)
{
    ASSERT(pq);
    ASSERT(pValue);
    const isize iPtr = (isize)pValue;
    const u32 mask = pq->width - 1u;
    isize *const pim_noalias ptr = pq->ptr;
    for (u32 i = load_u32(&pq->iWrite, MO_Acquire); PtrQueue_Size(pq) <= mask; ++i)
    {
        i &= mask;
        isize prev = load_isize(ptr + i, MO_Relaxed);
        if (!prev && cmpex_isize(ptr + i, &prev, iPtr, MO_Acquire))
        {
            inc_u32(&pq->iWrite, MO_Release);
            return true;
        }
    }
    return false;
}

void *const PtrQueue_TryPop(PtrQueue *const pq)
{
    ASSERT(pq);
    const u32 mask = pq->width - 1u;
    isize *const pim_noalias ptr = pq->ptr;
    for (u32 i = load_u32(&pq->iRead, MO_Acquire); PtrQueue_Size(pq); ++i)
    {
        i &= mask;
        isize prev = load_isize(ptr + i, MO_Relaxed);
        if (prev && cmpex_isize(ptr + i, &prev, 0, MO_Acquire))
        {
            inc_u32(&pq->iRead, MO_Release);
            ASSERT(prev);
            return (void *const)prev;
        }
    }
    return NULL;
}
