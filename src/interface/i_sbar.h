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

typedef struct I_Sbar_s
{
    void(*_Sbar_Init)(void);
    // call whenever any of the client stats represented on the sbar changes
    void(*_Sbar_Changed)(void);
    // called every frame by screen
    void(*_Sbar_Draw)(void);
    // called each frame after the level has been completed
    void(*_Sbar_IntermissionOverlay)(void);
    void(*_Sbar_FinaleOverlay)(void);
} I_Sbar_t;
extern I_Sbar_t I_Sbar;

#define Sbar_Init(...) I_Sbar._Sbar_Init(__VA_ARGS__)
#define Sbar_Changed(...) I_Sbar._Sbar_Changed(__VA_ARGS__)
#define Sbar_Draw(...) I_Sbar._Sbar_Draw(__VA_ARGS__)
#define Sbar_IntermissionOverlay(...) I_Sbar._Sbar_IntermissionOverlay(__VA_ARGS__)
#define Sbar_FinaleOverlay(...) I_Sbar._Sbar_FinaleOverlay(__VA_ARGS__)

// the status bar is only redrawn if something has changed, but if anything
// does, the entire thing will be redrawn for the next vid.numpages frames.
