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

typedef struct I_CdAudio_s
{
    i32(*_CDAudio_Init)(void);
    void(*_CDAudio_Play)(u8 track, qboolean looping);
    void(*_CDAudio_Stop)(void);
    void(*_CDAudio_Pause)(void);
    void(*_CDAudio_Resume)(void);
    void(*_CDAudio_Shutdown)(void);
    void(*_CDAudio_Update)(void);
} I_CdAudio_t;
extern I_CdAudio_t I_CdAudio;

#define CDAudio_Init(...) I_CdAudio._CDAudio_Init(__VA_ARGS__)
#define CDAudio_Play(...) I_CdAudio._CDAudio_Play(__VA_ARGS__)
#define CDAudio_Stop(...) I_CdAudio._CDAudio_Stop(__VA_ARGS__)
#define CDAudio_Pause(...) I_CdAudio._CDAudio_Pause(__VA_ARGS__)
#define CDAudio_Resume(...) I_CdAudio._CDAudio_Resume(__VA_ARGS__)
#define CDAudio_Shutdown(...) I_CdAudio._CDAudio_Shutdown(__VA_ARGS__)
#define CDAudio_Update(...) I_CdAudio._CDAudio_Update(__VA_ARGS__)
