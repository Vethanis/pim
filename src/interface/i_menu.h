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
typedef struct I_Menu_s
{
    void(*_M_Init)(void);
    void(*_M_Keydown)(i32 key);
    void(*_M_Draw)(void);
    void(*_M_ToggleMenu_f)(void);
} I_Menu_t;
extern I_Menu_t I_Menu;

#define M_Init(...) I_Menu._M_Init(__VA_ARGS__)
#define M_Keydown(...) I_Menu._M_Keydown(__VA_ARGS__)
#define M_Draw(...) I_Menu._M_Draw(__VA_ARGS__)
#define M_ToggleMenu_f(...) I_Menu._M_ToggleMenu_f(__VA_ARGS__)
