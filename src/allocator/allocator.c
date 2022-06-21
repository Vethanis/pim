#include "allocator/allocator.h"

#include "common/atomics.h"
#include "threading/mutex.h"
#include "threading/task.h"
#include "threading/thread.h"
#include "tlsf/tlsf.h"

#include <string.h>
#include <stdlib.h>

#define kTempFrames         2
#define kAlign              16
#define kAlignMask          (kAlign - 1)

#define kPermCapacity       (512 << 20)
#define kTextureCapacity    (512 << 20)
#define kTempCapacity       (256 << 20)
#define kScriptCapacity     (16 << 20)

#define kMaxBytesPerTexture (2048 * 2048 * 4)

typedef struct hdr_s
{
    pim_alignas(kAlign) i32 type;
    i32 userBytes;
    i32 tid;
    i32 refCount;
} hdr_t;
SASSERT((sizeof(hdr_t)) == kAlign);

typedef struct tlsf_allocator_s
{
    Mutex mtx;
    tlsf_t tlsf;
} tlsf_allocator_t;

typedef struct linear_allocator_s
{
    u64 head;
    u64 base;
    u64 capacity;
} linear_allocator_t;

static tlsf_allocator_t ms_perm;
static tlsf_allocator_t ms_texture;
static tlsf_allocator_t ms_script;
static i32 ms_tempIndex;
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
    return (u32)type < (u32)EAlloc_COUNT;
}

static bool valid_tid(i32 tid)
{
    return (u32)tid < (u32)kMaxThreads;
}

// ----------------------------------------------------------------------------

static void tlsf_allocator_new(tlsf_allocator_t* allocator, i32 capacity)
{
    ASSERT(allocator);
    ASSERT(capacity > 0);
    memset(allocator, 0, sizeof(*allocator));

    Mutex_New(&allocator->mtx);

    void* memory = malloc(capacity);
    ASSERT(memory);

    tlsf_t tlsf = tlsf_create_with_pool(memory, capacity);
    ASSERT(tlsf);
    allocator->tlsf = tlsf;
}

static void tlsf_allocator_del(tlsf_allocator_t* allocator)
{
    if (allocator)
    {
        if (allocator->tlsf)
        {
            Mutex_Del(&allocator->mtx);
            tlsf_destroy(allocator->tlsf);
        }
        memset(allocator, 0, sizeof(*allocator));
    }
}

static void* tlsf_allocator_malloc(tlsf_allocator_t* allocator, i32 bytes)
{
    Mutex_Lock(&allocator->mtx);
    void* ptr = tlsf_memalign(allocator->tlsf, kAlign, bytes);
    Mutex_Unlock(&allocator->mtx);
    ASSERT(ptr);
    return ptr;
}

static void tlsf_allocator_free(tlsf_allocator_t* allocator, void* ptr)
{
    Mutex_Lock(&allocator->mtx);
    tlsf_free(allocator->tlsf, ptr);
    Mutex_Unlock(&allocator->mtx);
}

// ----------------------------------------------------------------------------

static void linear_allocator_new(linear_allocator_t* alloc, i32 capacity)
{
    ASSERT(alloc);
    ASSERT(capacity > 0);
    memset(alloc, 0, sizeof(*alloc));

    void* memory = malloc(capacity);
    ASSERT(memory);

    alloc->base = (u64)memory;
    alloc->capacity = capacity;
}

static void linear_allocator_del(linear_allocator_t* alloc)
{
    if (alloc)
    {
        free((void*)(alloc->base));
        memset(alloc, 0, sizeof(*alloc));
    }
}

static void* linear_allocator_malloc(linear_allocator_t* alloc, i32 bytes)
{
    const u64 head = fetch_add_u64(&(alloc->head), bytes, MO_Acquire);
    const u64 tail = head + bytes;
    const u64 addr = alloc->base + head;
    return (tail <= alloc->capacity) ? (void*)addr : NULL;
}

static void linear_allocator_clear(linear_allocator_t* alloc)
{
    store_u64(&(alloc->head), 0, MO_Release);
}

// ----------------------------------------------------------------------------

void MemSys_Init(void)
{
    tlsf_allocator_new(&ms_perm, kPermCapacity);
    tlsf_allocator_new(&ms_texture, kTextureCapacity);
    tlsf_allocator_new(&ms_script, kScriptCapacity);
    ms_tempIndex = 0;
    for (i32 i = 0; i < kTempFrames; ++i)
    {
        linear_allocator_new(&ms_temp[i], kTempCapacity);
    }
}

void MemSys_Update(void)
{
    const i32 i = (ms_tempIndex + 1) % kTempFrames;
    ms_tempIndex = i;
    linear_allocator_clear(&ms_temp[i]);
}

void MemSys_Shutdown(void)
{
    tlsf_allocator_del(&ms_perm);
    tlsf_allocator_del(&ms_texture);
    tlsf_allocator_del(&ms_script);
    for (i32 i = 0; i < kTempFrames; ++i)
    {
        linear_allocator_del(&ms_temp[i]);
    }
}

// ----------------------------------------------------------------------------

void* Mem_Alloc(EAlloc type, i32 bytes)
{
    void* ptr = NULL;
    const i32 tid = Task_ThreadId();
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
            ptr = tlsf_allocator_malloc(&ms_perm, bytes);
            break;
        case EAlloc_Texture:
        {
            if (bytes < kMaxBytesPerTexture)
            {
                ptr = tlsf_allocator_malloc(&ms_texture, bytes);
            }
            else
            {
                // tlsf has trouble with very large allocations
                ptr = malloc(bytes);
            }
        }
        break;
        case EAlloc_Script:
            ptr = tlsf_allocator_malloc(&ms_script, bytes);
            break;
        case EAlloc_Temp:
            ptr = linear_allocator_malloc(&ms_temp[ms_tempIndex], bytes);
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

void Mem_Free(void* ptr)
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
            tlsf_allocator_free(&ms_perm, hdr);
            break;
        case EAlloc_Texture:
        {
            if ((userBytes + kAlign) < kMaxBytesPerTexture)
            {
                tlsf_allocator_free(&ms_texture, hdr);
            }
            else
            {
                free(hdr);
            }
        }
        break;
        case EAlloc_Script:
            tlsf_allocator_free(&ms_script, hdr);
            break;
        case EAlloc_Temp:
            break;
        }
    }
}

void* Mem_Realloc(EAlloc type, void* prev, i32 bytes)
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

    void* next = Mem_Alloc(type, nextBytes);
    memcpy(next, prev, prevBytes);
    Mem_Free(prev);

    return next;
}

void* Mem_Calloc(EAlloc type, i32 bytes)
{
    void* ptr = Mem_Alloc(type, bytes);
    memset(ptr, 0, bytes);
    return ptr;
}

// ----------------------------------------------------------------------------

#define kStackCapacity      (4 << 10)
#define kFrameCount         (kStackCapacity / kAlign)

typedef struct StackFrame_s
{
    pim_alignas(kAlign) u8 value[kAlign];
} StackFrame;

static i32 ms_iFrame[kMaxThreads];
static StackFrame ms_stack[kMaxThreads][kFrameCount];

void* Mem_Push(i32 bytes)
{
    ASSERT((u32)bytes <= (u32)kStackCapacity);
    i32 tid = Task_ThreadId();

    i32 frames = (bytes + sizeof(StackFrame) - 1) / sizeof(StackFrame);
    i32 back = ms_iFrame[tid];
    i32 front = back + frames;
    ASSERT(front <= kFrameCount);

    ms_iFrame[tid] = front;

    return ms_stack[tid][back].value;
}

void Mem_Pop(i32 bytes)
{
    ASSERT((u32)bytes <= (u32)kStackCapacity);
    i32 tid = Task_ThreadId();

    i32 frames = (bytes + sizeof(StackFrame) - 1) / sizeof(StackFrame);
    ms_iFrame[tid] -= frames;

    ASSERT(ms_iFrame[tid] >= 0);
}
