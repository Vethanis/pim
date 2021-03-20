#pragma once
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

#include "interface/i_types.h"

typedef struct I_Zone_s
{
    void(*_Memory_Init)(void *buf, i32 size);

    void(*_Z_Free)(void *ptr);
    // returns 0 filled memory
    void*(*_Z_Malloc)(i32 size);
    void*(*_Z_TagMalloc)(i32 size, i32 tag);
    void(*_Z_DumpHeap)(void);
    void(*_Z_CheckHeap)(void);
    i32(*_Z_FreeMemory)(void);

    // returns 0 filled memory
    void*(*_Hunk_Alloc)(i32 size);
    void*(*_Hunk_AllocName)(i32 size, const char *name);
    void*(*_Hunk_HighAllocName)(i32 size, const char *name);
    i32(*_Hunk_LowMark)(void);
    void(*_Hunk_FreeToLowMark)(i32 mark);
    i32(*_Hunk_HighMark)(void);
    void(*_Hunk_FreeToHighMark)(i32 mark);
    void*(*_Hunk_TempAlloc)(i32 size);
    void(*_Hunk_Check)(void);

    void(*_Cache_Flush)(void);
    // returns the cached data, and moves to the head of the LRU list if present, otherwise returns NULL
    void* (*_Cache_Check)(cache_user_t *c);
    void(*_Cache_Free)(cache_user_t *c);
    // Returns NULL if all purgable data was tossed and there still wasn't enough room.
    void*(*_Cache_Alloc)(cache_user_t *c, i32 size, const char *name);
    void(*_Cache_Report)(void);
} I_Zone_t;
extern I_Zone_t I_Zone;

#define Memory_Init(...) I_Zone._Memory_Init(__VA_ARGS__)

#define Z_Free(...) I_Zone._Z_Free(__VA_ARGS__)
#define Z_Malloc(...) I_Zone._Z_Malloc(__VA_ARGS__)
#define Z_TagMalloc(...) I_Zone._Z_TagMalloc(__VA_ARGS__)
#define Z_DumpHeap(...) I_Zone._Z_DumpHeap(__VA_ARGS__)
#define Z_CheckHeap(...) I_Zone._Z_CheckHeap(__VA_ARGS__)
#define Z_FreeMemory(...) I_Zone._Z_FreeMemory(__VA_ARGS__)

#define Hunk_Alloc(...) I_Zone._Hunk_Alloc(__VA_ARGS__)
#define Hunk_AllocName(...) I_Zone._Hunk_AllocName(__VA_ARGS__)
#define Hunk_HighAllocName(...) I_Zone._Hunk_HighAllocName(__VA_ARGS__)
#define Hunk_LowMark(...) I_Zone._Hunk_LowMark(__VA_ARGS__)
#define Hunk_FreeToLowMark(...) I_Zone._Hunk_FreeToLowMark(__VA_ARGS__)
#define Hunk_HighMark(...) I_Zone._Hunk_HighMark(__VA_ARGS__)
#define Hunk_FreeToHighMark(...) I_Zone._Hunk_FreeToHighMark(__VA_ARGS__)
#define Hunk_TempAlloc(...) I_Zone._Hunk_TempAlloc(__VA_ARGS__)
#define Hunk_Check(...) I_Zone._Hunk_Check(__VA_ARGS__)

#define Cache_Flush(...) I_Zone._Cache_Flush(__VA_ARGS__)
#define Cache_Check(...) I_Zone._Cache_Check(__VA_ARGS__)
#define Cache_Free(...) I_Zone._Cache_Free(__VA_ARGS__)
#define Cache_Alloc(...) I_Zone._Cache_Alloc(__VA_ARGS__)
#define Cache_Report(...) I_Zone._Cache_Report(__VA_ARGS__)

/*
 memory allocation

H_??? The hunk manages the entire memory block given to quake.  It must be
contiguous.  Memory can be allocated from either the low or high end in a
stack fashion.  The only way memory is released is by resetting one of the
pointers.

Hunk allocations should be given a name, so the Hunk_Print () function
can display usage.

Hunk allocations are guaranteed to be 16 byte aligned.

The video buffers are allocated high to avoid leaving a hole underneath
server allocations when changing to a higher video mode.

Z_??? I_Zone memory functions used for small, dynamic allocations like text
strings from command input.  There is only about 48K for it, allocated at
the very bottom of the hunk.

Cache_??? Cache memory is for objects that can be dynamically loaded and
can usefully stay persistant between levels.  The size of the cache
fluctuates from level to level.

To allocate a cachable object

Temp_??? Temp memory is used for file loading and surface caching.  The size
of the cache memory is adjusted so that there is a minimum of 512k remaining
for temp memory.

------ Top of Memory -------

high hunk allocations

<--- high hunk reset point held by vid

video buffer

z buffer

surface cache

<--- high hunk used

cachable memory

<--- low hunk used

client and server low hunk allocations

<-- low hunk reset point held by host

startup hunk allocations

I_Zone block

----- Bottom of Memory -----
*/
