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

// draw.h -- these are the only functions outside the refresh allowed
// to touch the vid buffer

#include "interface/i_types.h"

typedef struct I_Draw_s
{
    void(*_Draw_Init)(void);
    void(*_Draw_Character)(i32 x, i32 y, i32 num);
    void(*_Draw_DebugChar)(char num);
    void(*_Draw_Pic)(i32 x, i32 y, qpic_t *pic);
    void(*_Draw_TransPic)(i32 x, i32 y, qpic_t *pic);
    void(*_Draw_TransPicTranslate)(i32 x, i32 y, qpic_t *pic, byte *translation);
    void(*_Draw_ConsoleBackground)(i32 lines);
    void(*_Draw_BeginDisc)(void);
    void(*_Draw_EndDisc)(void);
    void(*_Draw_TileClear)(i32 x, i32 y, i32 w, i32 h);
    void(*_Draw_Fill)(i32 x, i32 y, i32 w, i32 h, i32 c);
    void(*_Draw_FadeScreen)(void);
    void(*_Draw_String)(i32 x, i32 y, const char *str);
    qpic_t*(*_Draw_PicFromWad)(const char *name);
    qpic_t*(*_Draw_CachePic)(const char *path);
} I_Draw_t;
extern I_Draw_t I_Draw;

#define Draw_Init(...) I_Draw._Draw_Init(__VA_ARGS__)
#define Draw_Character(...) I_Draw._Draw_Character(__VA_ARGS__)
#define Draw_DebugChar(...) I_Draw._Draw_DebugChar(__VA_ARGS__)
#define Draw_Pic(...) I_Draw._Draw_Pic(__VA_ARGS__)
#define Draw_TransPic(...) I_Draw._Draw_TransPic(__VA_ARGS__)
#define Draw_TransPicTranslate(...) I_Draw._Draw_TransPicTranslate(__VA_ARGS__)
#define Draw_ConsoleBackground(...) I_Draw._Draw_ConsoleBackground(__VA_ARGS__)
#define Draw_BeginDisc(...) I_Draw._Draw_BeginDisc(__VA_ARGS__)
#define Draw_EndDisc(...) I_Draw._Draw_EndDisc(__VA_ARGS__)
#define Draw_TileClear(...) I_Draw._Draw_TileClear(__VA_ARGS__)
#define Draw_Fill(...) I_Draw._Draw_Fill(__VA_ARGS__)
#define Draw_FadeScreen(...) I_Draw._Draw_FadeScreen(__VA_ARGS__)
#define Draw_String(...) I_Draw._Draw_String(__VA_ARGS__)
#define Draw_PicFromWad(...) I_Draw._Draw_PicFromWad(__VA_ARGS__)
#define Draw_CachePic(...) I_Draw._Draw_CachePic(__VA_ARGS__)
