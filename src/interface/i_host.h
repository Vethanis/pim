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

typedef struct I_Host_s
{
    void(*_Host_ClearMemory)(void);
    void(*_Host_ServerFrame)(void);
    void(*_Host_InitCommands)(void);
    void(*_Host_Init)(quakeparms_t *parms);
    void(*_Host_Shutdown)(void);
    void(*_Host_Error)(const char *error, ...);
    void(*_Host_EndGame)(const char *message, ...);
    void(*_Host_Frame)(float time);
    void(*_Host_Quit_f)(void);
    void(*_Host_ClientCommands)(const char *fmt, ...);
    void(*_Host_ShutdownServer)(qboolean crash);

    void(*_Chase_Init)(void);
    void(*_Chase_Reset)(void);
    void(*_Chase_Update)(void);
} I_Host_t;
extern I_Host_t I_Host;

#define Host_ClearMemory(...) I_Host._Host_ClearMemory(__VA_ARGS__)
#define Host_ServerFrame(...) I_Host._Host_ServerFrame(__VA_ARGS__)
#define Host_InitCommands(...) I_Host._Host_InitCommands(__VA_ARGS__)
#define Host_Init(...) I_Host._Host_Init(__VA_ARGS__)
#define Host_Shutdown(...) I_Host._Host_Shutdown(__VA_ARGS__)
#define Host_Error(...) I_Host._Host_Error(__VA_ARGS__)
#define Host_EndGame(...) I_Host._Host_EndGame(__VA_ARGS__)
#define Host_Frame(...) I_Host._Host_Frame(__VA_ARGS__)
#define Host_Quit_f(...) I_Host._Host_Quit_f(__VA_ARGS__)
#define Host_ClientCommands(...) I_Host._Host_ClientCommands(__VA_ARGS__)
#define Host_ShutdownServer(...) I_Host._Host_ShutdownServer(__VA_ARGS__)

#define Chase_Init(...) I_Host._Chase_Init(__VA_ARGS__)
#define Chase_Reset(...) I_Host._Chase_Reset(__VA_ARGS__)
#define Chase_Update(...) I_Host._Chase_Update(__VA_ARGS__)
