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

#include "common/macro.h"

typedef u8      byte;
typedef bool    qboolean;

// Allocation strategies of Quake:
// Zone: a basic heap allocator
// HunkNamed: persistent side (low) of a two-sided stack allocator
// HunkTemp: temporary side (high) of a two-sided stack allocator
// Cache: intrusive linked list of LRU garbage collected memory between hunk low and high marks
// Stack: stack memory or HunkTemp (often switched to .bss static memory in mods)
typedef enum
{
    QAlloc_Zone = 0,
    QAlloc_HunkNamed = 1,
    QAlloc_HunkTemp = 2,
    QAlloc_Cache = 3,
    QAlloc_Stack = 4,

    QAlloc_COUNT
} QAlloc;

typedef struct hdl_s
{
    u32 version : 8;
    u32 index : 24;
} hdl_t;

typedef struct filehdl_s { hdl_t h; } filehdl_t;
typedef struct meshhdl_s { hdl_t h; } meshhdl_t;
typedef struct texhdl_s { hdl_t h; } texhdl_t;
typedef struct sockhdl_s { hdl_t h; } sockhdl_t;
typedef struct enthdl_s { hdl_t h; } enthdl_t;

typedef struct sizebuf_s
{
    // allowoverflow and overflowed removed
    // sizebuf will reallocate
    byte *data;
    i32 maxsize;
    i32 cursize;
} sizebuf_t;

typedef struct link_s
{
    struct link_s *prev, *next;
} link_t;

typedef struct cache_user_s
{
    void *data;
} cache_user_t;
