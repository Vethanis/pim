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

typedef struct I_Screen_s
{
    void(*_SCR_Init)(void);
    void(*_SCR_UpdateScreen)(void);
    void(*_SCR_SizeUp)(void);
    void(*_SCR_SizeDown)(void);
    void(*_SCR_BringDownConsole)(void);
    void(*_SCR_CenterPrint)(const char *str);
    void(*_SCR_BeginLoadingPlaque)(void);
    void(*_SCR_EndLoadingPlaque)(void);
    i32(*_SCR_ModalMessage)(const char *text);
    void(*_SCR_UpdateWholeScreen)(void);
} I_Screen_t;
extern I_Screen_t I_Screen;

#define SCR_Init(...) I_Screen._SCR_Init(__VA_ARGS__)
#define SCR_UpdateScreen(...) I_Screen._SCR_UpdateScreen(__VA_ARGS__)
#define SCR_SizeUp(...) I_Screen._SCR_SizeUp(__VA_ARGS__)
#define SCR_SizeDown(...) I_Screen._SCR_SizeDown(__VA_ARGS__)
#define SCR_BringDownConsole(...) I_Screen._SCR_BringDownConsole(__VA_ARGS__)
#define SCR_CenterPrint(...) I_Screen._SCR_CenterPrint(__VA_ARGS__)
#define SCR_BeginLoadingPlaque(...) I_Screen._SCR_BeginLoadingPlaque(__VA_ARGS__)
#define SCR_EndLoadingPlaque(...) I_Screen._SCR_EndLoadingPlaque(__VA_ARGS__)
#define SCR_ModalMessage(...) I_Screen._SCR_ModalMessage(__VA_ARGS__)
#define SCR_UpdateWholeScreen(...) I_Screen._SCR_UpdateWholeScreen(__VA_ARGS__)
