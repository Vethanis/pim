#include "allocator/allocator.h"

#include "common/atomics.h"
#include "threading/mutex.h"
#include "threading/task.h"
#include "threading/thread.h"
#include "tlsf/tlsf.h"

#include <string.h>
#include <stdlib.h>

#define kTempFrames     4
#define kAlign          16
#define kAlignMask      (kAlign - 1)

#define kPermCapacity   (1024 << 20)
#define kTempCapacity   (256 << 20)

typedef pim_alignas(kAlign) struct hdr_s
{
    i32 type;
    i32 userBytes;
    i32 tid;
    i32 refCount;
} hdr_t;
SASSERT((sizeof(hdr_t)) == kAlign);

typedef struct linear_allocator_s
{
    u64 base;
    u64 head;
    u64 capacity;
} linear_allocator_t;

static i32 ms_tempIndex;
static mutex_t ms_perm_mtx;
static tlsf_t ms_perm;
static linear_allocator_t ms_temp[kTempFrames];

// ----------------------------------------------------------------------------

static i32 align_bytes(i32 bytes)
{
    return ((bytes + kAlign) + kAlignMask) & ~kAlignMask;
}

static bool ptr_is_aligned(const void* ptr)
{
    return ((isize)ptr & kAlignMask) == 0;
}

static bool i32_is_aligned(i32 bytes)
{
    return (bytes & kAlignMask) == 0;
}

static bool valid_type(i32 type)
{
    return (u32)type < (u32)EAlloc_Count;
}

static bool valid_tid(i32 tid)
{
    return (u32)tid < (u32)kMaxThreads;
}

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

    alloc->base = (u64)memory;
    alloc->capacity = capacity;
    alloc->head = 0;
}

static void destroy_linear(linear_allocator_t* alloc)
{
    ASSERT(alloc);
    free((void*)(alloc->base));
    alloc->base = 0;
    alloc->capacity = 0;
    alloc->head = 0;
}

static void* linear_alloc(linear_allocator_t* alloc, i32 bytes)
{
    const u64 head = fetch_add_u64(&(alloc->head), bytes, MO_Relaxed);
    const u64 tail = head + bytes;
    const u64 addr = alloc->base + head;
    return (tail <= alloc->capacity) ? (void*)addr : NULL;
}

static void linear_clear(linear_allocator_t* alloc)
{
    IF_DEBUG(memset((void*)(alloc->base), 0xcd, alloc->capacity));
    store_u64(&(alloc->head), 0, MO_Relaxed);
}

// ----------------------------------------------------------------------------

void alloc_sys_init(void)
{
    mutex_create(&ms_perm_mtx);
    ms_perm = create_tlsf(kPermCapacity);
    ms_tempIndex = 0;
    for (i32 i = 0; i < NELEM(ms_temp); ++i)
    {
        create_linear(ms_temp + i, kTempCapacity);
    }
}

void alloc_sys_update(void)
{
    const i32 i = (ms_tempIndex + 1) % kTempFrames;
    ms_tempIndex = i;
    linear_clear(ms_temp + i);
}

void alloc_sys_shutdown(void)
{
    mutex_lock(&ms_perm_mtx);
    free(ms_perm);
    ms_perm = NULL;
    mutex_unlock(&ms_perm_mtx);
    mutex_destroy(&ms_perm_mtx);

    for (i32 i = 0; i < NELEM(ms_temp); ++i)
    {
        destroy_linear(ms_temp + i);
    }
}

// ----------------------------------------------------------------------------

void* pim_malloc(EAlloc type, i32 bytes)
{
    void* ptr = NULL;
    const i32 tid = task_thread_id();
    ASSERT(valid_tid(tid));

    ASSERT(bytes >= 0);
    if (bytes > 0)
    {
        bytes = align_bytes(bytes);
        ASSERT(bytes > kAlign);

        switch (type)
        {
        default:
            ASSERT(false);
            break;
        case EAlloc_Perm:
            mutex_lock(&ms_perm_mtx);
            ptr = tlsf_memalign(ms_perm, kAlign, bytes);
            mutex_unlock(&ms_perm_mtx);
            break;
        case EAlloc_Temp:
            ptr = linear_alloc(ms_temp + ms_tempIndex, bytes);
            break;
        }

        const i32 userBytes = bytes - kAlign;

        ASSERT(ptr);
        ASSERT(ptr_is_aligned(ptr));

        hdr_t* hdr = (hdr_t*)ptr;
        hdr->type = type;
        hdr->userBytes = userBytes;
        hdr->tid = tid;
        hdr->refCount = 1;
        ptr = hdr + 1;

        ASSERT(ptr_is_aligned(ptr));
        IF_DEBUG(memset(ptr, 0xcc, userBytes));
    }

    return ptr;
}

void pim_free(void* ptr)
{
    if (ptr)
    {
        ASSERT(ptr_is_aligned(ptr));

        hdr_t* hdr = (hdr_t*)ptr - 1;
        const i32 userBytes = hdr->userBytes;
        const i32 tid = hdr->tid;

        ASSERT(userBytes > 0);
        ASSERT(i32_is_aligned(userBytes));
        ASSERT(valid_tid(tid));
        ASSERT(dec_i32(&(hdr->refCount), MO_Relaxed) == 1);
        IF_DEBUG(memset(ptr, 0xcd, userBytes));

        switch (hdr->type)
        {
        default:
            ASSERT(false);
            break;
        case EAlloc_Perm:
            mutex_lock(&ms_perm_mtx);
            tlsf_free(ms_perm, hdr);
            mutex_unlock(&ms_perm_mtx);
            break;
        case EAlloc_Temp:
            break;
        }
    }
}

void* pim_realloc(EAlloc type, void* prev, i32 bytes)
{
    ASSERT(bytes > 0);
    ASSERT(ptr_is_aligned(prev));

    i32 prevBytes = 0;
    if (prev)
    {
        const hdr_t* prevHdr = (const hdr_t*)prev - 1;
        prevBytes = prevHdr->userBytes;

        ASSERT(ptr_is_aligned(prevHdr));
        ASSERT(valid_type(prevHdr->type));
        ASSERT(prevBytes > 0);
        ASSERT(i32_is_aligned(prevBytes));
        ASSERT(valid_tid(prevHdr->tid));
        ASSERT(load_i32(&(prevHdr->refCount), MO_Relaxed) == 1);

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
    memset(ptr, 0x00, bytes);
    return ptr;
}

// ----------------------------------------------------------------------------

#define kStackCapacity      (4 << 10)
#define kFrameCount         (kStackCapacity / kAlign)

typedef pim_alignas(kAlign) struct sframe_s
{
    u8 value[kAlign];
} sframe_t;

static i32 ms_iFrame[kMaxThreads];
static sframe_t ms_stack[kMaxThreads][kFrameCount];

void* pim_pusha(i32 bytes)
{
    ASSERT((u32)bytes <= (u32)kStackCapacity);
    const i32 tid = task_thread_id();

    bytes = align_bytes(bytes);
    const i32 frames = bytes / kAlign;
    const i32 back = ms_iFrame[tid];
    const i32 front = back + frames;
    ASSERT(front <= kFrameCount);

    ms_iFrame[tid] = front;

    return ms_stack[tid][back].value;
}

void pim_popa(i32 bytes)
{
    ASSERT((u32)bytes <= (u32)kStackCapacity);
    const i32 tid = task_thread_id();

    bytes = align_bytes(bytes);
    const i32 frames = bytes / kAlign;
    ms_iFrame[tid] -= frames;

    ASSERT(ms_iFrame[tid] >= 0);
}
