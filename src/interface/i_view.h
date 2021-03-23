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
// view.h

#include "interface/i_types.h"

typedef struct I_View_s
{
    void(*_V_Init)(void);
    void(*_V_RenderView)(void);
    float(*_V_CalcRoll)(vec3_t angles, vec3_t velocity);
    void(*_V_UpdatePalette)(void);
} I_View_t;
extern I_View_t I_View;

#define V_Init(...) I_View._V_Init(__VA_ARGS__)
#define V_RenderView(...) I_View._V_RenderView(__VA_ARGS__)
#define V_CalcRoll(...) I_View._V_CalcRoll(__VA_ARGS__)
#define V_UpdatePalette(...) I_View._V_UpdatePalette(__VA_ARGS__)
