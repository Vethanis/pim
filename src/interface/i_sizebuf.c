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

#include "interface/i_sizebuf.h"

#if QUAKE_IMPL

#include "allocator/allocator.h"
#include "common/stringutil.h"
#include <string.h>

void SZ_Alloc(sizebuf_t* buf, i32 startsize)
{
    ASSERT(!buf->data);
    ASSERT(startsize >= 0);
    startsize = pim_max(startsize, 256);
    startsize += 4;
    buf->data = Perm_Realloc(buf->data, startsize);
    memset(buf->data, 0, startsize);
    buf->cursize = 0;
}

void SZ_Free(sizebuf_t* buf)
{
    Mem_Free(buf->data);
    memset(buf, 0, sizeof(*buf));
}

void SZ_Clear(sizebuf_t* buf)
{
    i32 prevsize = buf->cursize;
    if (prevsize > 0)
    {
        memset(buf->data, 0, prevsize);
    }
    buf->cursize = 0;
}

void* SZ_GetSpace(sizebuf_t* buf, i32 length)
{
    ASSERT(length >= 0);
    const i32 prevsize = buf->cursize;
    buf->cursize = prevsize + length;
    buf->data = Perm_Realloc(buf->data, buf->cursize + 16);
    void* ptr = buf->data + prevsize;
    memset(ptr, 0, length + 4);
    return ptr;
}

void SZ_Write(sizebuf_t* buf, const void* data, i32 length)
{
    memcpy(SZ_GetSpace(buf, length), data, length);
}

void SZ_Print(sizebuf_t* buf, const char* data)
{
    SZ_Write(buf, data, StrLen(data));
}

#endif // QUAKE_IMPL
