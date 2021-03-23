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

typedef struct I_Keys_s
{
    void(*_Key_Event)(i32 key, qboolean down);
    void(*_Key_Init)(void);
    void(*_Key_WriteBindings)(filehdl_t f);
    void(*_Key_SetBinding)(i32 keynum, const char* binding);
    void(*_Key_ClearStates)(void);
} I_Keys_t;
extern I_Keys_t I_Keys;

#define Key_Event(...) I_Keys._Key_Event(__VA_ARGS__)
#define Key_Init(...) I_Keys._Key_Init(__VA_ARGS__)
#define Key_WriteBindings(...) I_Keys._Key_WriteBindings(__VA_ARGS__)
#define Key_SetBinding(...) I_Keys._Key_SetBinding(__VA_ARGS__)
#define Key_ClearStates(...) I_Keys._Key_ClearStates(__VA_ARGS__)

