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
// input.h -- external (non-keyboard) input devices

#include "interface/i_types.h"

typedef struct I_Input_s
{
    void(*_IN_Init)(void);
    void(*_IN_Shutdown)(void);
    // oportunity for devices to stick commands on the script buffer
    void(*_IN_Commands)(void);
    // add additional movement on top of the keyboard move cmd
    void(*_IN_Move)(usercmd_t *cmd);
    // restores all button and position states to defaults
    void(*_IN_ClearStates)(void);
} I_Input_t;
extern I_Input_t I_Input;

#define IN_Init(...) I_Input._IN_Init(__VA_ARGS__)
#define IN_Shutdown(...) I_Input._IN_Shutdown(__VA_ARGS__)
#define IN_Commands(...) I_Input._IN_Commands(__VA_ARGS__)
#define IN_Move(...) I_Input._IN_Move(__VA_ARGS__)
#define IN_ClearStates(...) I_Input._IN_ClearStates(__VA_ARGS__)
