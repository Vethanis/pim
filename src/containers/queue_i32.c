#include "containers/queue_i32.h"
#include "common/nextpow2.h"
#include "allocator/allocator.h"
#include <string.h>

void queue_i32_new(queue_i32_t* q)
{
    memset(q, 0, sizeof(*q));
}

void queue_i32_del(queue_i32_t* q)
{
    Mem_Free(q->ptr);
    memset(q, 0, sizeof(*q));
}

void queue_i32_clear(queue_i32_t* q)
{
    q->iRead = 0;
    q->iWrite = 0;
}

u32 queue_i32_size(const queue_i32_t* q)
{
    return q->iWrite - q->iRead;
}

u32 queue_i32_capacity(const queue_i32_t* q)
{
    return q->width;
}

void queue_i32_reserve(queue_i32_t* q, u32 capacity)
{
    capacity = capacity > 16 ? capacity : 16;
    const u32 newWidth = NextPow2(capacity);
    const u32 oldWidth = q->width;
    if (newWidth > oldWidth)
    {
        i32 *const pim_noalias oldPtr = q->ptr;
        i32 *const pim_noalias newPtr = Perm_Calloc(sizeof(*newPtr) * newWidth);
        const u32 iRead = q->iRead;
        const u32 len = q->iWrite - iRead;
        const u32 mask = oldWidth - 1u;
        for (u32 i = 0; i < len; ++i)
        {
            u32 j = (iRead + i) & mask;
            newPtr[i] = oldPtr[j];
        }
        Mem_Free(oldPtr);
        q->ptr = newPtr;
        q->width = newWidth;
        q->iRead = 0;
        q->iWrite = len;
    }
}

void queue_i32_push(queue_i32_t* q, i32 value)
{
    queue_i32_reserve(q, queue_i32_size(q) + 1);
    u32 mask = q->width - 1;
    u32 dst = q->iWrite++;
    q->ptr[dst & mask] = value;
}

bool queue_i32_trypop(queue_i32_t* q, i32* pim_noalias valueOut)
{
    ASSERT(valueOut);
    if (queue_i32_size(q))
    {
        u32 mask = q->width - 1;
        u32 src = q->iRead++;
        *valueOut = q->ptr[src & mask];
        return true;
    }
    return false;
}
