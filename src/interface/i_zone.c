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
#include "interface/i_sys.h"

#include "allocator/allocator.h"
#include "containers/dict.h"
#include "containers/text.h"
#include "containers/lookup.h"
#include "containers/idalloc.h"
#include "common/time.h"
#include "common/stringutil.h"

#include <string.h>

static void _Memory_Init(void *buf, i32 size);

static void _Z_Free(void *ptr);
static void* _Z_Malloc(i32 size);
static void* _Z_TagMalloc(i32 size, i32 tag);
static void _Z_DumpHeap(void);
static void _Z_CheckHeap(void);
static i32 _Z_FreeMemory(void);

static void* _Hunk_Alloc(i32 size);
static void* _Hunk_AllocName(i32 size, const char *name);
static void* _Hunk_HighAllocName(i32 size, const char *name);
static i32 _Hunk_LowMark(void);
static void _Hunk_FreeToLowMark(i32 mark);
static i32 _Hunk_HighMark(void);
static void _Hunk_FreeToHighMark(i32 mark);
static void* _Hunk_TempAlloc(i32 size);
static void _Hunk_Check(void);

static void _Cache_Flush(void);
static void* _Cache_Check(cache_user_t *c);
static void _Cache_Free(cache_user_t *c);
static void* _Cache_Alloc(cache_user_t *c, i32 size, const char *name);
static void _Cache_Report(void);

const I_Zone_t I_Zone = 
{
    ._Memory_Init = _Memory_Init,

    ._Z_Free = _Z_Free,
    ._Z_Malloc = _Z_Malloc,
    ._Z_TagMalloc = _Z_TagMalloc,
    ._Z_DumpHeap = _Z_DumpHeap,
    ._Z_CheckHeap = _Z_CheckHeap,
    ._Z_FreeMemory = _Z_FreeMemory,

    ._Hunk_Alloc = _Hunk_Alloc,
    ._Hunk_AllocName = _Hunk_AllocName,
    ._Hunk_HighAllocName = _Hunk_HighAllocName,
    ._Hunk_LowMark = _Hunk_LowMark,
    ._Hunk_FreeToLowMark = _Hunk_FreeToLowMark,
    ._Hunk_HighMark = _Hunk_HighMark,
    ._Hunk_FreeToHighMark = _Hunk_FreeToHighMark,
    ._Hunk_TempAlloc = _Hunk_TempAlloc,
    ._Hunk_Check = _Hunk_Check,

    ._Cache_Flush = _Cache_Flush,
    ._Cache_Check = _Cache_Check,
    ._Cache_Free= _Cache_Free,
    ._Cache_Alloc = _Cache_Alloc,
    ._Cache_Report = _Cache_Report,
};

// ----------------------------------------------------------------------------

static bool ms_once;

static void _Hunk_Init(void* buf, i32 size);
static void _Cache_Init(void);
static void _Z_Init(void);

static void _Memory_Init(void *buf, i32 size)
{
    ASSERT(!ms_once);
    ms_once = true;
    _Hunk_Init(buf, size);
    _Cache_Init();
    _Z_Init();
}

// ----------------------------------------------------------------------------

static void _Z_Init(void)
{

}

static void _Z_Free(void *ptr)
{
    Mem_Free(ptr);
}

static void* _Z_Malloc(i32 size)
{
    return NULL;
}

static void* _Z_TagMalloc(i32 size, i32 tag)
{
    return NULL;
}

static void _Z_DumpHeap(void)
{

}

static void _Z_CheckHeap(void)
{

}

static i32 _Z_FreeMemory(void)
{
    return 0;
}

// ----------------------------------------------------------------------------

typedef struct HunkAllocator_s
{
    u8* pbase;
    i32 size;
    i32 lowUsed;
    i32 highUsed;
    i32 tempMark;
    bool tempActive;
} HunkAllocator;
static HunkAllocator ms_hunk;

static void _Hunk_Init(void* buf, i32 size)
{

}

static void* _Hunk_Alloc(i32 size)
{
    return NULL;
}

static void* _Hunk_AllocName(i32 size, const char *name)
{
    return NULL;
}

static void* _Hunk_HighAllocName(i32 size, const char *name)
{
    return NULL;
}

static i32 _Hunk_LowMark(void)
{
    return 0;
}

static void _Hunk_FreeToLowMark(i32 mark)
{

}

static i32 _Hunk_HighMark(void)
{
    return 0;
}

static void _Hunk_FreeToHighMark(i32 mark)
{

}

static void* _Hunk_TempAlloc(i32 size)
{
    return NULL;
}

static void _Hunk_Check(void)
{

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
} CacheAllocator;
static CacheAllocator ms_cache;

static void _Cache_Init(void)
{
    ASSERT(!ms_cache.bytesAllocated);
    memset(&ms_cache, 0, sizeof(ms_cache));
}

static void _Cache_Flush(void)
{
    CacheItem* pim_noalias items = ms_cache.items;
    const i32 len = ms_cache.ids.length;
    for (i32 i = 0; i < len; ++i)
    {
        void* allocation = items[i].allocation;
        i32 size = items[i].size;
        if (allocation)
        {
            memset(allocation, 0, size);
            Mem_Free(allocation);
        }
    }
    memset(items, 0, sizeof(items[0]) * len);
    memset(ms_cache.ticks, 0, sizeof(ms_cache.ticks[0]) * len);
    memset(ms_cache.names, 0, sizeof(ms_cache.names[0]) * len);
    IdAlloc_Clear(&ms_cache.ids);
}

static void* _Cache_Check(cache_user_t *c)
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

static void _Cache_Free(cache_user_t *c)
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
            memset(allocation, 0, size);
            Mem_Free(allocation);
            ms_cache.bytesAllocated -= size;
        }
        memset(item, 0, sizeof(*item));
        ms_cache.ticks[slot] = 0;
        memset(&ms_cache.names[slot], 0, sizeof(ms_cache.names[slot]));
    }
}

static void* _Cache_Alloc(cache_user_t *c, i32 size, const char *name)
{
    ASSERT(!IdAlloc_Exists(&ms_cache.ids, c->hdl.h));
    ASSERT(size >= 0);
    ASSERT(name && name[0]);
    if (size <= 0)
    {
        return NULL;
    }

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

    if (ms_cache.bytesAllocated >= (1 << 20))
    {
        const u8* pim_noalias versions = ms_cache.ids.versions;
        const u64* pim_noalias ticks = ms_cache.ticks;
        u64 now = Time_Now();
        for (i32 i = 0; i < len; ++i)
        {
            if (versions[i] & 1)
            {
                if (Time_Sec(now - ticks[i]) > (60.0))
                {
                    cache_user_t user = { 0 };
                    user.hdl.h.index = i;
                    user.hdl.h.version = versions[i];
                    _Cache_Free(&user);
                }
            }
        }
    }

    return allocation;
}

static void _Cache_Report(void)
{

}

