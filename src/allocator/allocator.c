#include "allocator/allocator.h"

#include "common/atomics.h"
#include "threading/mutex.h"
#include "threading/task.h"
#include "tlsf/tlsf.h"

#include <stdlib.h>
#include <string.h>

#define kMaxThreads     32
#define kTempFrames     4
#define kTempMask       3

#define kInitCapacity   0
#define kPermCapacity   (1 << 30)
#define kTempCapacity   (8 << 20)
#define kTlsCapacity    (1 << 20)

typedef struct linear_allocator_s
{
    i64 base;
    i64 head;
    i64 capacity;
} linear_allocator_t;

static i32 ms_tempIndex;
static mutex_t ms_perm_mtx;
static tlsf_t ms_perm;
static linear_allocator_t ms_temp[kTempFrames];
static tlsf_t ms_local[kMaxThreads];

// ----------------------------------------------------------------------------

static tlsf_t create_tlsf(i32 capacity)
{
    ASSERT(capacity > 0);

    void* memory = malloc(capacity);
    ASSERT(memory);

    tlsf_t tlsf = tlsf_create_with_pool(memory, capacity);
    ASSERT(tlsf);

    return tlsf;
}

static void create_linear(linear_allocator_t* alloc, i32 capacity)
{
    ASSERT(alloc);
    ASSERT(capacity > 0);

    void* memory = malloc(capacity);
    ASSERT(memory);

    alloc->base = (i64)memory;
    alloc->capacity = capacity;
    alloc->head = 0;
}

static void destroy_linear(linear_allocator_t* alloc)
{
    ASSERT(alloc);
    free((void*)(alloc->base));
    memset(alloc, 0, sizeof(linear_allocator_t));
}

static void* linear_alloc(linear_allocator_t* alloc, i32 bytes)
{
    const i64 head = fetch_add_i64(&(alloc->head), bytes, MO_Acquire);
    const i64 tail = head + bytes;
    const i64 addr = alloc->base + head;
    return (tail < alloc->capacity) ? (void*)addr : NULL;
}

static void linear_free(linear_allocator_t* alloc, void* ptr, i32 bytes)
{
    const i64 addr = (i64)ptr;
    const i64 base = alloc->base;
    const i64 head = addr - base;
    i64 tail = head + bytes;
    cmpex_i64(&(alloc->head), &tail, head, MO_Release);
}

static void linear_clear(linear_allocator_t* alloc)
{
    store_i64(&(alloc->head), 0, MO_Release);
}

// ----------------------------------------------------------------------------

void alloc_sys_init(void)
{
    mutex_create(&ms_perm_mtx);
    ms_perm = create_tlsf(kPermCapacity);
    ms_tempIndex = 0;
    for (i32 i = 0; i < kTempFrames; ++i)
    {
        create_linear(ms_temp + i, kTempCapacity);
    }
}

void alloc_sys_update(void)
{
    const i32 i = (ms_tempIndex + 1) & kTempMask;
    ms_tempIndex = i;
    linear_clear(ms_temp + i);
}

void alloc_sys_shutdown(void)
{
    mutex_lock(&ms_perm_mtx);

    free(ms_perm);
    ms_perm = 0;

    for (i32 i = 0; i < kMaxThreads; ++i)
    {
        free(ms_local[i]);
        ms_local[i] = NULL;
    }

    for (i32 i = 0; i < kTempFrames; ++i)
    {
        destroy_linear(ms_temp + i);
    }

    mutex_unlock(&ms_perm_mtx);
    mutex_destroy(&ms_perm_mtx);
}

// ----------------------------------------------------------------------------

void* pim_malloc(EAlloc type, i32 bytes)
{
    void* ptr = 0;
    const i32 tid = task_thread_id();
    ASSERT(tid < kMaxThreads);

    ASSERT(bytes >= 0);
    if (bytes > 0)
    {
        bytes = ((bytes + 16) + 15) & ~15;

        switch (type)
        {
        default:
            ASSERT(0);
            break;
        case EAlloc_Init:
            ptr = malloc(bytes);
            break;
        case EAlloc_Perm:
            mutex_lock(&ms_perm_mtx);
            ptr = tlsf_memalign(ms_perm, 16, bytes);
            mutex_unlock(&ms_perm_mtx);
            break;
        case EAlloc_Temp:
            ptr = linear_alloc(ms_temp + ms_tempIndex, bytes);
            break;
        case EAlloc_TLS:
            if (!ms_local[tid])
            {
                ms_local[tid] = create_tlsf(kTlsCapacity);
            }
            ptr = tlsf_memalign(ms_local[tid], 16, bytes);
            break;
        }

        ASSERT(ptr);
        i32* header = (i32*)ptr;
        header[0] = type;
        header[1] = bytes - 16;
        header[2] = tid;
        header[3] = 1;
        ptr = header + 4;

        ASSERT(((isize)ptr & 15) == 0);
    }

    return ptr;
}

void pim_free(void* ptr)
{
    if (ptr)
    {
        ASSERT(((isize)ptr & 15) == 0);
        i32* header = (i32*)ptr - 4;
        const EAlloc type = header[0];
        const i32 bytes = header[1];
        const i32 tid = header[2];
        const i32 rc = dec_i32(header + 3, MO_Relaxed);
        ASSERT(bytes > 0);
        ASSERT((bytes & 15) == 0);
        ASSERT(tid < kMaxThreads);
        ASSERT(rc == 1);

        switch (type)
        {
        default:
            ASSERT(0);
            break;
        case EAlloc_Init:
            free(header);
            break;
        case EAlloc_Perm:
            mutex_lock(&ms_perm_mtx);
            tlsf_free(ms_perm, header);
            mutex_unlock(&ms_perm_mtx);
            break;
        case EAlloc_Temp:
            linear_free(ms_temp + ms_tempIndex, header, bytes + 16);
            break;
        case EAlloc_TLS:
            ASSERT(tid == task_thread_id());
            ASSERT(ms_local[tid]);
            tlsf_free(ms_local[tid], header);
            break;
        }
    }
}

void* pim_realloc(EAlloc type, void* prev, i32 bytes)
{
    ASSERT(bytes > 0);

    i32 prevBytes = 0;
    if (prev)
    {
        const i32* prevHdr = (const i32*)prev - 4;
        prevBytes = prevHdr[1];
        ASSERT(prevBytes > 0);
        ASSERT(load_i32(prevHdr + 3, MO_Relaxed) == 1);

        if (bytes <= prevBytes)
        {
            return prev;
        }
    }

    i32 nextBytes = prevBytes * 2;
    nextBytes = nextBytes > 64 ? nextBytes : 64;
    nextBytes = nextBytes > bytes ? nextBytes : bytes;

    void* next = pim_malloc(type, nextBytes);
    memcpy(next, prev, prevBytes);
    pim_free(prev);

    return next;
}

void* pim_calloc(EAlloc type, i32 bytes)
{
    void* ptr = pim_malloc(type, bytes);
    if (ptr)
    {
        memset(ptr, 0, bytes);
    }
    return ptr;
}
