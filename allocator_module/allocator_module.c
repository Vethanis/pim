#include "allocator_module.h"

#include <stdlib.h>
#include <pthread.h>
#include "tlsf/tlsf.h"

typedef struct
{
    tlsf_t tlsf;
    pool_t pool;
    int32_t capacity;
} allocator_t;

static int32_t ms_sizes[EAlloc_Count];
static int32_t ms_tempIndex;

static pthread_mutex_t ms_perm_mtx;
static allocator_t ms_perm;
static allocator_t ms_temp[2];
static PIM_TLS allocator_t ms_local;

static allocator_t Create(int32_t capacity)
{
    void* pControl = malloc(tlsf_size());
    ASSERT(pControl);

    tlsf_t tlsf = tlsf_create(pControl);
    ASSERT(tlsf);

    void* pPool = malloc(capacity);
    ASSERT(pPool);

    pool_t pool = tlsf_add_pool(tlsf, pPool, capacity);
    ASSERT(tlsf);

    return (allocator_t)
    {
        tlsf, pool, capacity
    };
}

static void Destroy(allocator_t* alloc)
{
    ASSERT(alloc);
    free(alloc->tlsf);
    alloc->tlsf = 0;
    free(alloc->pool);
    alloc->pool = 0;
    alloc->capacity = 0;
}

static void Clear(allocator_t* alloc)
{
    tlsf_remove_pool(alloc->tlsf, alloc->pool);
    tlsf_add_pool(alloc->tlsf, alloc->pool, alloc->capacity);
}

static void PIM_CDECL Init(const int32_t* sizes, int32_t count)
{
    ASSERT(count == EAlloc_Count);
    for (int32_t i = 0; i < EAlloc_Count; ++i)
    {
        ASSERT(sizes[i] >= 0);
        ms_sizes[i] = sizes[i];
    }

    pthread_mutex_init(&ms_perm_mtx, NULL);

    ms_tempIndex = 0;
    ms_perm = Create(ms_sizes[EAlloc_Perm]);
    ms_temp[0] = Create(ms_sizes[EAlloc_Temp]);
    ms_temp[1] = Create(ms_sizes[EAlloc_Temp]);
}

static void PIM_CDECL Update(void)
{
    ms_tempIndex = (ms_tempIndex + 1) & 1;
    Clear(ms_temp + ms_tempIndex);
}

static void PIM_CDECL Shutdown(void)
{
    pthread_mutex_destroy(&ms_perm_mtx);
    Destroy(&ms_perm);
    Destroy(&ms_local);
    Destroy(ms_temp + 0);
    Destroy(ms_temp + 1);
}

static void* PIM_CDECL Alloc(EAllocator type, int32_t bytes)
{
    if (bytes <= 0)
    {
        return 0;
    }

    bytes = ((bytes + 16) + 15) & ~15;

    void* ptr = 0;
    switch (type)
    {
    default:
        ASSERT(0);
        break;
    case EAlloc_Init:
        ptr = malloc(bytes);
        break;
    case EAlloc_Perm:
        pthread_mutex_lock(&ms_perm_mtx);
        ptr = tlsf_memalign(ms_perm.tlsf, 16, bytes);
        pthread_mutex_unlock(&ms_perm_mtx);
        break;
    case EAlloc_Temp:
        ptr = tlsf_memalign(ms_temp[ms_tempIndex].tlsf, 16, bytes);
        break;
    case EAlloc_Thread:
        if (!ms_local.tlsf)
        {
            ms_local = Create(ms_sizes[EAlloc_Thread]);
        }
        ptr = tlsf_memalign(ms_local.tlsf, 16, bytes);
        break;
    }

    if (ptr)
    {
        int32_t* header = (int32_t*)ptr;
        header[0] = type;
        return header + 4;
    }

    return 0;
}

static void PIM_CDECL Free(void* ptr)
{
    if (ptr)
    {
        int32_t* header = (int32_t*)ptr - 4;
        EAllocator type = header[0];

        switch (type)
        {
        default:
            ASSERT(0);
            break;
        case EAlloc_Init:
            free(header);
            break;
        case EAlloc_Perm:
            tlsf_free(ms_perm.tlsf, header);
            break;
        case EAlloc_Temp:
            tlsf_free(ms_temp[ms_tempIndex].tlsf, header);
            break;
        case EAlloc_Thread:
            ASSERT(ms_local.tlsf);
            tlsf_free(ms_local.tlsf, header);
            break;
        }
    }
}

static const allocator_module_t kModule =
{
    .Init = Init,
    .Update = Update,
    .Shutdown = Shutdown,
    .Alloc = Alloc,
    .Free = Free,
};

ALLOC_API const allocator_module_t* PIM_CDECL allocator_module_export(void)
{
    return &kModule;
}
