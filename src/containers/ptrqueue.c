#include "containers/ptrqueue.h"
#include "common/atomics.h"
#include "common/nextpow2.h"
#include <string.h>

void ptrqueue_create(ptrqueue_t* pq, EAlloc allocator, uint32_t capacity)
{
    ASSERT(pq);
    memset(pq, 0, sizeof(ptrqueue_t));
    rwlock_create(&(pq->lock));
    pq->allocator = allocator;
    if (capacity > 0)
    {
        ptrqueue_reserve(pq, capacity);
    }
}

void ptrqueue_destroy(ptrqueue_t* pq)
{
    rwlock_t* lock = &(pq->lock);
    rwlock_lock_write(lock);
    pim_free(pq->ptr);
    pq->ptr = 0;
    store_u32(&(pq->width), 0, MO_Release);
    store_u32(&(pq->iWrite), 0, MO_Release);
    store_u32(&(pq->iRead), 0, MO_Release);
    rwlock_unlock_write(lock);
    rwlock_destroy(lock);
}

uint32_t ptrqueue_capacity(const ptrqueue_t* pq)
{
    ASSERT(pq);
    return load_u32(&(pq->width), MO_Acquire);
}

uint32_t ptrqueue_size(const ptrqueue_t* pq)
{
    ASSERT(pq);
    return load_u32(&(pq->iWrite), MO_Acquire) - load_u32(&(pq->iRead), MO_Acquire);
}

void ptrqueue_clear(ptrqueue_t* pq)
{
    ASSERT(pq);
    rwlock_lock_read(&(pq->lock));
    {
        store_u32(&(pq->iWrite), 0, MO_Relaxed);
        store_u32(&(pq->iRead), 0, MO_Relaxed);
        const uint32_t width = load_u32(&(pq->width), MO_Relaxed);
        intptr_t* ptr = pq->ptr;
        for (uint32_t i = 0; i < width; ++i)
        {
            store_ptr(ptr + i, 0, MO_Relaxed);
        }
    }
    rwlock_unlock_read(&(pq->lock));
}

void ptrqueue_reserve(ptrqueue_t* pq, uint32_t capacity)
{
    ASSERT(pq);
    capacity = (capacity > 16) ? capacity : 16;
    const uint32_t newWidth = NextPow2(capacity);
    if (newWidth > ptrqueue_capacity(pq))
    {
        intptr_t* newPtr = pim_tcalloc(intptr_t, pq->allocator, newWidth);
        ASSERT(newPtr);

        rwlock_lock_write(&(pq->lock));

        intptr_t* oldPtr = pq->ptr;
        const uint32_t oldWidth = ptrqueue_capacity(pq);
        const int32_t grew = oldWidth < newWidth;

        if (grew)
        {
            const uint32_t oldMask = oldWidth - 1u;
            const uint32_t oldTail = load_u32(&(pq->iRead), MO_Acquire);
            const uint32_t oldHead = load_u32(&(pq->iWrite), MO_Acquire);
            const uint32_t len = oldHead - oldTail;

            for (uint32_t i = 0; i < len; ++i)
            {
                const uint32_t iOld = (oldTail + i) & oldMask;
                newPtr[i] = oldPtr[iOld];
            }

            pq->ptr = newPtr;
            store_u32(&(pq->width), newWidth, MO_Release);
            store_u32(&(pq->iRead), 0, MO_Release);
            store_u32(&(pq->iWrite), len, MO_Release);
        }

        rwlock_unlock_write(&(pq->lock));

        if (grew)
        {
            pim_free(oldPtr);
        }
        else
        {
            pim_free(newPtr);
        }
    }
}

void ptrqueue_push(ptrqueue_t* pq, void* pValue)
{
    ASSERT(pq);
    ASSERT(pValue);
    const intptr_t iPtr = (intptr_t)pValue;
    while (1)
    {
        ptrqueue_reserve(pq, ptrqueue_size(pq) + 1u);

        rwlock_lock_read(&(pq->lock));
        const uint32_t mask = ptrqueue_capacity(pq) - 1u;
        intptr_t* ptr = pq->ptr;
        for (uint32_t i = load_u32(&(pq->iWrite), MO_Acquire); ptrqueue_size(pq) <= mask; ++i)
        {
            i &= mask;
            intptr_t prev = load_ptr(ptr + i, MO_Relaxed);
            if (!prev && cmpex_ptr(ptr + i, &prev, iPtr, MO_Acquire))
            {
                inc_u32(&(pq->iWrite), MO_Release);
                rwlock_unlock_read(&(pq->lock));
                return;
            }
        }
        rwlock_unlock_read(&(pq->lock));
    }
}

void* ptrqueue_trypop(ptrqueue_t* pq)
{
    ASSERT(pq);
    if (!ptrqueue_size(pq))
    {
        return 0;
    }

    rwlock_lock_read(&(pq->lock));
    const uint32_t mask = ptrqueue_capacity(pq) - 1u;
    intptr_t* ptr = pq->ptr;
    for (uint32_t i = load_u32(&(pq->iRead), MO_Acquire); ptrqueue_size(pq) != 0u; ++i)
    {
        i &= mask;
        intptr_t prev = load_ptr(ptr + i, MO_Relaxed);
        if (prev && cmpex_ptr(ptr + i, &prev, 0, MO_Acquire))
        {
            inc_u32(&(pq->iRead), MO_Release);
            ASSERT(prev);
            rwlock_unlock_read(&(pq->lock));
            return (void*)prev;
        }
    }
    rwlock_unlock_read(&(pq->lock));

    return 0;
}
