#include "containers/ptrqueue.h"
#include "common/atomics.h"
#include "common/nextpow2.h"
#include "allocator/allocator.h"

void ptrqueue_create(ptrqueue_t* pq, EAlloc allocator, u32 capacity)
{
    ASSERT(pq);
    rwlock_create(&(pq->lock));
    pq->iWrite = 0;
    pq->iRead = 0;
    pq->width = 0;
    pq->ptr = NULL;
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

u32 ptrqueue_capacity(const ptrqueue_t* pq)
{
    ASSERT(pq);
    return load_u32(&(pq->width), MO_Acquire);
}

u32 ptrqueue_size(const ptrqueue_t* pq)
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
        const u32 width = load_u32(&(pq->width), MO_Relaxed);
        isize* ptr = pq->ptr;
        for (u32 i = 0; i < width; ++i)
        {
            store_isize(ptr + i, 0, MO_Relaxed);
        }
    }
    rwlock_unlock_read(&(pq->lock));
}

void ptrqueue_reserve(ptrqueue_t* pq, u32 capacity)
{
    ASSERT(pq);
    capacity = (capacity > 16) ? capacity : 16;
    const u32 newWidth = NextPow2(capacity);
    if (newWidth > ptrqueue_capacity(pq))
    {
        isize* newPtr = pim_calloc(pq->allocator, newWidth * sizeof(isize));
        ASSERT(newPtr);

        rwlock_lock_write(&(pq->lock));

        isize* oldPtr = pq->ptr;
        const u32 oldWidth = ptrqueue_capacity(pq);
        const bool grew = oldWidth < newWidth;

        if (grew)
        {
            const u32 oldMask = oldWidth - 1u;
            const u32 oldTail = load_u32(&(pq->iRead), MO_Acquire);
            const u32 oldHead = load_u32(&(pq->iWrite), MO_Acquire);
            const u32 len = oldHead - oldTail;

            for (u32 i = 0; i < len; ++i)
            {
                const u32 iOld = (oldTail + i) & oldMask;
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
    const isize iPtr = (isize)pValue;
    while (1)
    {
        ptrqueue_reserve(pq, ptrqueue_size(pq) + 1u);

        rwlock_lock_read(&(pq->lock));
        const u32 mask = ptrqueue_capacity(pq) - 1u;
        isize* ptr = pq->ptr;
        for (u32 i = load_u32(&(pq->iWrite), MO_Acquire); ptrqueue_size(pq) <= mask; ++i)
        {
            i &= mask;
            isize prev = load_isize(ptr + i, MO_Relaxed);
            if (!prev && cmpex_isize(ptr + i, &prev, iPtr, MO_Acquire))
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
    const u32 mask = ptrqueue_capacity(pq) - 1u;
    isize* ptr = pq->ptr;
    for (u32 i = load_u32(&(pq->iRead), MO_Acquire); ptrqueue_size(pq) != 0u; ++i)
    {
        i &= mask;
        isize prev = load_isize(ptr + i, MO_Relaxed);
        if (prev && cmpex_isize(ptr + i, &prev, 0, MO_Acquire))
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
