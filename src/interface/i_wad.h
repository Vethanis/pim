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
// wad.h

#include "interface/i_types.h"

typedef struct I_Wad_s
{
    void (*_W_LoadWadFile)(const char *filename);
    void(*_W_CleanupName)(const char *strIn, char *strOut);
    lumpinfo_t* (*_W_GetLumpinfo)(const char *name);
    void*(*_W_GetLumpName)(const char *name);
    void*(*_W_GetLumpNum)(i32 num);
    void(*_SwapPic)(qpic_t *pic);
} I_Wad_t;
extern I_Wad_t I_Wad;

#define W_LoadWadFile(...) I_Wad._W_LoadWadFile(__VA_ARGS__)
#define W_CleanupName(...) I_Wad._W_CleanupName(__VA_ARGS__)
#define W_GetLumpinfo(...) I_Wad._W_GetLumpinfo(__VA_ARGS__)
#define W_GetLumpName(...) I_Wad._W_GetLumpName(__VA_ARGS__)
#define W_GetLumpNum(...) I_Wad._W_GetLumpNum(__VA_ARGS__)
#define SwapPic(...) I_Wad._SwapPic(__VA_ARGS__)
