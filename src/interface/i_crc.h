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
/* crc.h */

#include "interface/i_types.h"

typedef struct I_Crc_s
{
    void(*_CRC_Init)(u16 *crcvalue);
    void(*_CRC_ProcessByte)(u16 *crcvalue, u8 data);
    u16(*_CRC_Value)(u16 crcvalue);
} I_Crc_t;
extern I_Crc_t I_Crc;

#define CRC_Init(...) I_Crc._CRC_Init(__VA_ARGS__)
#define CRC_ProcessByte(...) I_Crc._CRC_ProcessByte(__VA_ARGS__)
#define CRC_Value(...) I_Crc._CRC_Value(__VA_ARGS__)
