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

typedef struct I_Console_s
{
    void(*_Con_DrawCharacter)(i32 cx, i32 line, i32 num);
    void(*_Con_CheckResize)(void);
    void(*_Con_Init)(void);
    void(*_Con_DrawConsole)(i32 lines, qboolean drawinput);
    void(*_Con_Print)(const char *txt);
    void(*_Con_Printf)(const char *fmt, ...);
    void(*_Con_DPrintf)(const char *fmt, ...);
    void(*_Con_SafePrintf)(const char *fmt, ...);
    void(*_Con_Clear_f)(void);
    void(*_Con_DrawNotify)(void);
    void(*_Con_ClearNotify)(void);
    void(*_Con_ToggleConsole_f)(void);
    // during startup for sound / cd warnings
    void(*_Con_NotifyBox)(const char *text);
} I_Console_t;
extern I_Console_t I_Console;

#define Con_DrawCharacter(...) I_Console._Con_DrawCharacter(__VA_ARGS__)
#define Con_CheckResize(...) I_Console._Con_CheckResize(__VA_ARGS__)
#define Con_Init(...) I_Console._Con_Init(__VA_ARGS__)
#define Con_DrawConsole(...) I_Console._Con_DrawConsole(__VA_ARGS__)
#define Con_Print(...) I_Console._Con_Print(__VA_ARGS__)
#define Con_Printf(...) I_Console._Con_Printf(__VA_ARGS__)
#define Con_DPrintf(...) I_Console._Con_DPrintf(__VA_ARGS__)
#define Con_SafePrintf(...) I_Console._Con_SafePrintf(__VA_ARGS__)
#define Con_Clear_f(...) I_Console._Con_Clear_f(__VA_ARGS__)
#define Con_DrawNotify(...) I_Console._Con_DrawNotify(__VA_ARGS__)
#define Con_ClearNotify(...) I_Console._Con_ClearNotify(__VA_ARGS__)
#define Con_ToggleConsole_f(...) I_Console._Con_ToggleConsole_f(__VA_ARGS__)
#define Con_NotifyBox(...) I_Console._Con_NotifyBox(__VA_ARGS__)
