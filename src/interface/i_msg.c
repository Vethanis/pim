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

#include "interface/i_msg.h"

#if QUAKE_IMPL

#include "interface/i_globals.h"
#include "interface/i_sizebuf.h"

#include "common/stringutil.h"
#include <string.h>

void MSG_WriteChar(sizebuf_t *sb, i32 x)
{
    ASSERT(x <= 0x7f);
    ASSERT(x >= -0x80);
    x = pim_min(x, 0x7f);
    x = pim_max(x, -0x80);
    u8 y = (u8)(x & 0xff);
    SZ_Write(sb, &y, sizeof(y));
}

void MSG_WriteByte(sizebuf_t *sb, i32 x)
{
    ASSERT(x <= 0xff);
    ASSERT(x >= 0x00);
    x = pim_min(x, 0xff);
    x = pim_max(x, 0x00);
    u8 y = (u8)(x & 0xff);
    SZ_Write(sb, &y, sizeof(y));
}

void MSG_WriteShort(sizebuf_t *sb, i32 x)
{
    ASSERT(x <= 0x7fff);
    ASSERT(x >= -0x8000);
    x = pim_min(x, 0x7fff);
    x = pim_max(x, -0x8000);
    u16 y = (u16)(x & 0xffff);
    SZ_Write(sb, &y, sizeof(y));
}

void MSG_WriteLong(sizebuf_t *sb, i32 x)
{
    SZ_Write(sb, &x, sizeof(x));
}

void MSG_WriteFloat(sizebuf_t *sb, float x)
{
    SZ_Write(sb, &x, sizeof(x));
}

void MSG_WriteString(sizebuf_t *sb, const char *x)
{
    x = x ? x : "";
    SZ_Write(sb, x, StrLen(x) + 1);
}

void MSG_WriteCoord(sizebuf_t *sb, float x)
{
    MSG_WriteShort(sb, (i32)(x * 8.0f));
}

void MSG_WriteAngle(sizebuf_t *sb, float x)
{
    i32 angle = (i32)(x * (256.0f / 360.0f));
    angle &= 0xff;
    MSG_WriteByte(sb, angle);
}

void MSG_BeginReading(void)
{
    g_msg_readcount = 0;
    g_msg_badread = false;
}

i32 MSG_ReadChar(void)
{
    i8 c;
    if ((g_msg_readcount + sizeof(c)) <= g_net_message.cursize)
    {
        const void* src = &g_net_message.data[g_msg_readcount];
        memcpy(&c, src, sizeof(c));
        g_msg_readcount += sizeof(c);
        return (i32)c;
    }
    g_msg_badread = true;
    return -1;
}

i32 MSG_ReadByte(void)
{
    u8 c;
    if ((g_msg_readcount + sizeof(c)) <= g_net_message.cursize)
    {
        const void* src = &g_net_message.data[g_msg_readcount];
        memcpy(&c, src, sizeof(c));
        g_msg_readcount += sizeof(c);
        return (i32)c;
    }
    g_msg_badread = true;
    return -1;
}

i32 MSG_ReadShort(void)
{
    i16 c;
    if ((g_msg_readcount + sizeof(c)) <= g_net_message.cursize)
    {
        const void* src = &g_net_message.data[g_msg_readcount];
        memcpy(&c, src, sizeof(c));
        g_msg_readcount += sizeof(c);
        return (i32)c;
    }
    g_msg_badread = true;
    return -1;
}

i32 MSG_ReadLong(void)
{
    i32 c;
    if ((g_msg_readcount + sizeof(c)) <= g_net_message.cursize)
    {
        const void* src = &g_net_message.data[g_msg_readcount];
        memcpy(&c, src, sizeof(c));
        g_msg_readcount += sizeof(c);
        return c;
    }
    g_msg_badread = true;
    return -1;
}

float MSG_ReadFloat(void)
{
    float c;
    if ((g_msg_readcount + sizeof(c)) <= g_net_message.cursize)
    {
        const void* src = &g_net_message.data[g_msg_readcount];
        memcpy(&c, src, sizeof(c));
        g_msg_readcount += sizeof(c);
        return c;
    }
    g_msg_badread = true;
    return -1.0f;
}

static char msg_string[2048];
const char* MSG_ReadString(void)
{
    msg_string[0] = 0;
    const i32 offset = g_msg_readcount;
    i32 size = g_net_message.cursize - offset;
    size = pim_min(size, sizeof(msg_string));
    if (size > 0)
    {
        const char* src = (char*)(g_net_message.data + offset);
        i32 len = StrCpy(msg_string, size, src);
        g_msg_readcount += len + 1;
    }
    else
    {
        g_msg_badread = true;
    }
    return msg_string;
}

float MSG_ReadCoord(void)
{
    return MSG_ReadShort() * (1.0f / 8.0f);
}

float MSG_ReadAngle(void)
{
    return MSG_ReadChar() * (360.0f / 256.0f);
}

#endif // QUAKE_IMPL
