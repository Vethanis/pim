#include "allocator/allocator.h"

#include "common/atomics.h"
#include "threading/mutex.h"
#include "threading/task.h"
#include "tlsf/tlsf.h"

#include <stdlib.h>
#include <string.h>

#define kMaxThreads 32

static const int32_t kInitCapacity = 0;
static const int32_t kPermCapacity = 1 << 30;
static const int32_t kTempCapacity = 8 << 20;
static const int32_t kTlsCapacity = 1 << 20;

typedef struct linear_allocator_s
{
    int64_t base;
    int64_t head;
    int64_t capacity;
} linear_allocator_t;

static int32_t ms_tempIndex;
static mutex_t ms_perm_mtx;
static tlsf_t ms_perm;
static linear_allocator_t ms_temp[2];
static tlsf_t ms_local[kMaxThreads];

// ----------------------------------------------------------------------------

static tlsf_t create_tlsf(int32_t capacity)
{
    ASSERT(capacity > 0);

    void* memory = malloc(capacity);
    ASSERT(memory);

    tlsf_t tlsf = tlsf_create_with_pool(memory, capacity);
    ASSERT(tlsf);

    return tlsf;
}

static void create_linear(linear_allocator_t* alloc, int32_t capacity)
{
    ASSERT(alloc);
    ASSERT(capacity > 0);

    void* memory = malloc(capacity);
    ASSERT(memory);

    alloc->base = (int64_t)memory;
    alloc->capacity = capacity;
    alloc->head = 0;
}

static void destroy_linear(linear_allocator_t* alloc)
{
    ASSERT(alloc);
    free((void*)(alloc->base));
    memset(alloc, 0, sizeof(linear_allocator_t));
}

static void* linear_alloc(linear_allocator_t* alloc, int32_t bytes)
{
    int64_t head = fetch_add_i64(&(alloc->head), bytes, MO_Acquire);
    int64_t tail = head + bytes;
    int64_t addr = alloc->base + head;
    return (tail < alloc->capacity) ? (void*)addr : 0;
}

static void linear_free(linear_allocator_t* alloc, void* ptr, int32_t bytes)
{
    int64_t addr = (int64_t)ptr;
    int64_t base = alloc->base;
    int64_t head = addr - base;
    int64_t tail = head + bytes;
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
    ms_tempIndex = 0;
    ms_perm = create_tlsf(kPermCapacity);
    create_linear(ms_temp + 0, kTempCapacity);
    create_linear(ms_temp + 1, kTempCapacity);
}

void alloc_sys_update(void)
{
    ms_tempIndex = (ms_tempIndex + 1) & 1;
    linear_clear(ms_temp + ms_tempIndex);
}

void alloc_sys_shutdown(void)
{
    mutex_destroy(&ms_perm_mtx);

    free(ms_perm);
    ms_perm = 0;

    for (int32_t i = 0; i < kMaxThreads; ++i)
    {
        if (ms_local[i])
        {
            free(ms_local[i]);
            ms_local[i] = 0;
        }
    }

    destroy_linear(ms_temp + 0);
    destroy_linear(ms_temp + 1);
}

// ----------------------------------------------------------------------------

void* pim_malloc(EAlloc type, int32_t bytes)
{
    void* ptr = 0;
    int32_t tid = task_thread_id();
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
        int32_t* header = (int32_t*)ptr;
        header[0] = type;
        header[1] = bytes - 16;
        header[2] = tid;
        header[3] = 1;
        ptr = header + 4;

        ASSERT(((intptr_t)ptr & 15) == 0);
    }

    return ptr;
}

void pim_free(void* ptr)
{
    if (ptr)
    {
        ASSERT(((intptr_t)ptr & 15) == 0);
        int32_t* header = (int32_t*)ptr - 4;
        EAlloc type = header[0];
        int32_t bytes = header[1];
        int32_t tid = header[2];
        int32_t rc = dec_i32(header + 3, MO_Relaxed);
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

void* pim_realloc(EAlloc type, void* prev, int32_t bytes)
{
    if (!prev)
    {
        return pim_malloc(type, bytes);
    }
    if (bytes <= 0)
    {
        ASSERT(bytes == 0);
        pim_free(prev);
        return 0;
    }

    int32_t* prevHdr = (int32_t*)prev - 4;
    const int32_t prevBytes = prevHdr[1];
    ASSERT(prevBytes > 0);
    ASSERT(load_i32(prevHdr + 3, MO_Relaxed) == 1);
    if (bytes <= prevBytes)
    {
        return prev;
    }

    bytes = (bytes > (prevBytes * 2)) ? bytes : (prevBytes * 2);
    ASSERT(bytes > 0);

    void* next = pim_malloc(type, bytes);
    memcpy(next, prev, prevBytes);
    pim_free(prev);

    return next;
}

void* pim_calloc(EAlloc type, int32_t bytes)
{
    void* ptr = pim_malloc(type, bytes);
    if (ptr)
    {
        memset(ptr, 0, bytes);
    }
    return ptr;
}
