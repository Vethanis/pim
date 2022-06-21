#include "allocator/arena.h"
#include "common/atomics.h"
#include "threading/intrin.h"
#include <string.h>
#include <stdlib.h>

#define kRingLen        (32)
#define kRingMask       (kRingLen - 1)
#define kArenaSize      (1 << 20)
#define kCacheLine      64

typedef struct Arena_s
{
    pim_alignas(kCacheLine) u32 head;
    u8 pad[kCacheLine - 4];
} Arena;
SASSERT(sizeof(Arena) == kCacheLine);

typedef struct ArenaSys_s
{
    pim_alignas(kCacheLine) u8* mem;
    u8 pad1[kCacheLine];

    u32 seqno;
    u8 pad2[kCacheLine];

    u32 ringseq[kRingLen];
    u8 pad3[kCacheLine];

    Arena ring[kRingLen];
    u8 pad4[kCacheLine];
} ArenaSys;
static ArenaSys g_ArenaSys;

void ArenaSys_Init(void)
{
    ArenaSys *const sys = &g_ArenaSys;
    sys->seqno = kRingLen;
    sys->mem = malloc(kArenaSize * kRingLen);
    for (u32 i = 0; i < kRingLen; ++i)
    {
        sys->ringseq[i] = kRingLen + i;
        sys->ring[i].head = 0;
    }
}

void ArenaSys_Shutdown(void)
{
    ArenaSys *const sys = &g_ArenaSys;
    sys->seqno = 0xcdcdcdcd;
    for (u32 i = 0; i < kRingLen; ++i)
    {
        sys->ringseq[i] = 0xdcdcdcdc;
        sys->ring[i].head = kArenaSize;
    }
    free(sys->mem); sys->mem = NULL;
}

bool Arena_Exists(ArenaHdl hdl)
{
    ArenaSys *const sys = &g_ArenaSys;
    const u32 slot = hdl.seqno & kRingMask;
    return load_u32(&sys->ringseq[slot], MO_Relaxed) == (hdl.seqno + 1);
}

ArenaHdl Arena_Acquire(void)
{
    ArenaSys *const sys = &g_ArenaSys;
    ArenaHdl hdl = { 0 };
    const u32 seqbase = load_u32(&sys->seqno, MO_Relaxed);
    for (u32 i = 0; i < kRingLen; ++i)
    {
        const u32 seqno = seqbase + i;
        const u32 slot = seqno & kRingMask;
        u32 expected = seqno;
        if (cmpex_u32(&sys->ringseq[slot], &expected, seqno + 1, MO_Acquire))
        {
            inc_u32(&sys->seqno, MO_Release);
            store_u32(&sys->ring[slot].head, 0, MO_Release);
            hdl.seqno = seqno;
            break;
        }
    }
    return hdl;
}

void Arena_Release(ArenaHdl hdl)
{
    ArenaSys *const sys = &g_ArenaSys;
    const u32 seqno = hdl.seqno;
    const u32 slot = seqno & kRingMask;
    u32 expected = seqno + 1;
    cmpex_u32(&sys->ringseq[slot], &expected, seqno + kRingLen, MO_Release);
}

void* Arena_Alloc(ArenaHdl hdl, u32 bytes)
{
    ArenaSys *const sys = &g_ArenaSys;
    ASSERT(sys->mem);
    if (Arena_Exists(hdl))
    {
        bytes = (bytes + 15u) & ~15u;
        if (bytes && (bytes < kArenaSize))
        {
            u32 slot = hdl.seqno & kRingMask;
            u32 head = fetch_add_u32(&sys->ring[slot].head, bytes, MO_Acquire);
            return sys->mem + slot * kArenaSize + head;
        }
    }
    return NULL;
}
