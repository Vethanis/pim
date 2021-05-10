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

void MSG_WriteChar(sizebuf_t *sb, i32 x);
void MSG_WriteByte(sizebuf_t *sb, i32 x);
void MSG_WriteShort(sizebuf_t *sb, i32 x);
void MSG_WriteLong(sizebuf_t *sb, i32 x);
void MSG_WriteFloat(sizebuf_t *sb, float x);
void MSG_WriteString(sizebuf_t *sb, const char *x);
void MSG_WriteCoord(sizebuf_t *sb, float x);
void MSG_WriteAngle(sizebuf_t *sb, float x);
void MSG_BeginReading(void);
i32 MSG_ReadChar(void);
i32 MSG_ReadByte(void);
i32 MSG_ReadShort(void);
i32 MSG_ReadLong(void);
float MSG_ReadFloat(void);
const char* MSG_ReadString(void);
float MSG_ReadCoord(void);
float MSG_ReadAngle(void);
