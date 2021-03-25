/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "interface/i_zone.h"

#include "interface/i_cmd.h"
#include "interface/i_common.h"
#include "interface/i_console.h"
#include "interface/i_sys.h"

#include "interface/i_globals.h"

#include "allocator/allocator.h"
#include "containers/text.h"
#include "containers/idalloc.h"
#include "common/time.h"
#include "common/stringutil.h"

#include <string.h>
#include <stdlib.h>

#if QUAKE_IMPL

// ----------------------------------------------------------------------------

void Z_Free(void *ptr)
{
    Mem_Free(ptr);
}

static void* Z_Malloc(i32 size)
{
    return Z_TagMalloc(size, 1);
}

static void* Z_TagMalloc(i32 size, i32 tag)
{
    return Perm_Calloc(size);
}

// ----------------------------------------------------------------------------

typedef struct HunkAllocator_s
{
    u8* pbase;
    i32 size;
    i32 lowUsed;
    i32 highUsed;
} HunkAllocator;
static HunkAllocator ms_hunk;

static i32 Hunk_Capacity(void)
{
    return ms_hunk.size - ms_hunk.lowUsed - ms_hunk.highUsed;
}

void Hunk_Init(i32 size)
{
    ASSERT(!ms_hunk.pbase);
    memset(&ms_hunk, 0, sizeof(ms_hunk));
    ASSERT(size > 0);
    size = (size + 15) & ~15;
    void* mem = malloc(size);
    ASSERT(mem);
    memset(mem, 0, size);
    ms_hunk.pbase = mem;
    ms_hunk.size = size;
}

void* Hunk_Alloc(i32 size)
{
    return Hunk_AllocName(size, "unknown");
}

void* Hunk_AllocName(i32 size, const char *name)
{
    // allocate from low stack
    ASSERT(size >= 0);
    ASSERT(name && name[0]);
    if (size <= 0)
    {
        return NULL;
    }
    size = (size + 15) & ~15;
    ASSERT(size > 0); // overflow after alignment
    if (size > Hunk_Capacity())
    {
        ASSERT(false);
        return NULL;
    }
    u8* ptr = ms_hunk.pbase + ms_hunk.lowUsed;
    ms_hunk.lowUsed += size;
    memset(ptr, 0, size);
    return ptr;
}

void* Hunk_HighAllocName(i32 size, const char *name)
{
    // allocate from high stack
    ASSERT(size >= 0);
    ASSERT(name && name[0]);
    if (size <= 0)
    {
        return NULL;
    }
    size = (size + 15) & ~15;
    ASSERT(size > 0); // overflow after alignment
    if (size > Hunk_Capacity())
    {
        ASSERT(false);
        return NULL;
    }
    ms_hunk.highUsed += size;
    u8* ptr = ms_hunk.pbase + ms_hunk.size - ms_hunk.highUsed;
    memset(ptr, 0, size);
    return ptr;
}

i32 Hunk_LowMark(void)
{
    return ms_hunk.lowUsed;
}

void Hunk_FreeToLowMark(i32 mark)
{
    i32 size = ms_hunk.lowUsed - mark;
    ASSERT(mark >= 0);
    ASSERT(size >= 0);
    ASSERT((mark + size) <= ms_hunk.size);
    memset(ms_hunk.pbase + mark, 0xcd, size);
    ms_hunk.lowUsed = mark;
}

i32 Hunk_HighMark(void)
{
    return ms_hunk.highUsed;
}

void Hunk_FreeToHighMark(i32 mark)
{
    ASSERT(mark >= 0);
    i32 offset = ms_hunk.size - ms_hunk.highUsed;
    i32 size = ms_hunk.highUsed - mark;
    ASSERT(offset >= 0);
    ASSERT(size >= 0);
    ASSERT((offset + size) <= ms_hunk.size);
    memset(ms_hunk.pbase + offset, 0xcd, size);
    ms_hunk.highUsed = mark;
}

void* Hunk_TempAlloc(i32 size)
{
    return Temp_Calloc(size);
}

void Hunk_Check(void)
{
    // dead memory gets filled with 0xcd.
    // use a debugger.
}

// ----------------------------------------------------------------------------

typedef struct CacheItem_s
{
    void* allocation;
    i32 size;
} CacheItem;

typedef struct CacheAllocator_s
{
    IdAlloc ids;
    CacheItem* pim_noalias items;
    u64* pim_noalias ticks;
    Text16* pim_noalias names;
    i32 bytesAllocated;
    u32 lastCollectFrame;
} CacheAllocator;
static CacheAllocator ms_cache;

void Cache_Init(void)
{
    ASSERT(!ms_cache.bytesAllocated);
    memset(&ms_cache, 0, sizeof(ms_cache));
    IdAlloc_New(&ms_cache.ids);

    Cmd_AddCommand("flush", Cache_Flush);
}

void Cache_Flush(void)
{
    CacheItem* pim_noalias items = ms_cache.items;
    const i32 len = ms_cache.ids.length;
    for (i32 i = 0; i < len; ++i)
    {
        Mem_Free(items[i].allocation);
    }
    memset(items, 0, sizeof(items[0]) * len);
    memset(ms_cache.ticks, 0, sizeof(ms_cache.ticks[0]) * len);
    memset(ms_cache.names, 0, sizeof(ms_cache.names[0]) * len);
    IdAlloc_Clear(&ms_cache.ids);
    ms_cache.bytesAllocated = 0;
    ms_cache.lastCollectFrame = Time_FrameCount();
}

void Cache_Collect(void)
{
    const u32 frameIndex = Time_FrameCount();
    if (ms_cache.lastCollectFrame != frameIndex)
    {
        ms_cache.lastCollectFrame = frameIndex;
        const u64 now = Time_Now();
        const u8* pim_noalias versions = ms_cache.ids.versions;
        const u64* pim_noalias ticks = ms_cache.ticks;
        const i32 len = ms_cache.ids.length;
        for (i32 i = 0; i < len; ++i)
        {
            u8 alive = versions[i] & 1;
            double duration = Time_Sec(now - ticks[i]) - 10.0;
            if ((alive) && (duration > 0.0))
            {
                cache_user_t user = { 0 };
                user.hdl.h.index = i;
                user.hdl.h.version = versions[i];
                Cache_Free(&user);
            }
        }
    }
}

void* Cache_Check(cache_user_t *c)
{
    GenId id = c->hdl.h;
    if (IdAlloc_Exists(&ms_cache.ids, id))
    {
        i32 slot = id.index;
        ms_cache.ticks[slot] = Time_Now();
        return ms_cache.items[slot].allocation;
    }
    return NULL;
}

void Cache_Free(cache_user_t *c)
{
    GenId id = c->hdl.h;
    if (IdAlloc_Free(&ms_cache.ids, id))
    {
        i32 slot = id.index;
        CacheItem* pim_noalias item = &ms_cache.items[slot];
        void* allocation = item->allocation;
        i32 size = item->size;
        if (allocation)
        {
            ASSERT(size > 0);
            Mem_Free(allocation);
            ms_cache.bytesAllocated -= size;
        }
        memset(item, 0, sizeof(*item));
        ms_cache.ticks[slot] = 0;
        memset(&ms_cache.names[slot], 0, sizeof(ms_cache.names[slot]));
    }
}

void* Cache_Alloc(cache_user_t *c, i32 size, const char *name)
{
    ASSERT(!IdAlloc_Exists(&ms_cache.ids, c->hdl.h));
    ASSERT(size >= 0);
    ASSERT(name && name[0]);
    if (size <= 0)
    {
        return NULL;
    }

    Cache_Collect();

    GenId id = IdAlloc_Alloc(&ms_cache.ids);
    i32 len = ms_cache.ids.length;
    ms_cache.items = Perm_Realloc(ms_cache.items, sizeof(ms_cache.items[0]) * len);
    ms_cache.ticks = Perm_Realloc(ms_cache.ticks, sizeof(ms_cache.ticks[0]) * len);
    ms_cache.names = Perm_Realloc(ms_cache.names, sizeof(ms_cache.names[0]) * len);

    i32 slot = id.index;
    void* allocation = Perm_Calloc(size);

    ms_cache.items[slot].allocation = allocation;
    ms_cache.items[slot].size = size;
    ms_cache.ticks[slot] = Time_Now();
    StrCpy(ARGS(ms_cache.names[slot].c), name);
    ms_cache.bytesAllocated += size;

    return allocation;
}

void _Cache_Report(void)
{
    float size = (float)(ms_cache.bytesAllocated) / (float)(1 << 20);
    Con_DPrintf("%4.1f megabyte data cache\n", size);
}

// ----------------------------------------------------------------------------

static bool ms_once;

void Memory_Init(i32 size)
{
    ASSERT(!ms_once);
    ms_once = true;
    Hunk_Init(size);
    Cache_Init();
}

#endif // QUAKE_IMPL
