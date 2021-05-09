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
// screen.h

#include "interface/i_types.h"

void SCR_Init(void);
void SCR_UpdateScreen(void);
void SCR_SizeUp(void);
void SCR_SizeDown(void);
void SCR_BringDownConsole(void);
void SCR_CenterPrint(const char *str);
void SCR_BeginLoadingPlaque(void);
void SCR_EndLoadingPlaque(void);
i32 SCR_ModalMessage(const char *text);
void SCR_UpdateWholeScreen(void);
