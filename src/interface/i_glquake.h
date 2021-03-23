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
// disable data conversion warnings

#include "interface/i_types.h"

typedef struct I_GlQuake_s
{
    void(*_GL_BeginRendering)(i32 *x, i32 *y, i32 *width, i32 *height);
    void(*_GL_EndRendering)(void);
    void(*_GL_Upload32)(u32* data, i32 width, i32 height, qboolean mipmap, qboolean alpha);
    void(*_GL_Upload8)(u8* data, i32 width, i32 height, qboolean mipmap, qboolean alpha);
    i32(*_GL_LoadTexture)(const char* identifier, i32 width, i32 height, u8* data, qboolean mipmap, qboolean alpha);
    i32(*_GL_FindTexture)(const char* identifier);
    void(*_GL_Bind)(i32 texnum);
    void(*_GL_DisableMultitexture)(void);
    void(*_GL_EnableMultitexture)(void);

    void(*_R_TimeRefresh_f)(void);
    void(*_R_ReadPointFile_f)(void);
    texture_t* (*_R_TextureAnimation)(texture_t *base);
    void(*_R_TranslatePlayerSkin)(i32 playernum);
} I_GlQuake_t;
extern I_GlQuake_t I_GlQuake;

#define GL_BeginRendering(...) I_GlQuake._GL_BeginRendering(__VA_ARGS__)
#define GL_EndRendering(...) I_GlQuake._GL_EndRendering(__VA_ARGS__)
#define GL_Upload32(...) I_GlQuake._GL_Upload32(__VA_ARGS__)
#define GL_Upload8(...) I_GlQuake._GL_Upload8(__VA_ARGS__)
#define GL_LoadTexture(...) I_GlQuake._GL_LoadTexture(__VA_ARGS__)
#define GL_FindTexture(...) I_GlQuake._GL_FindTexture(__VA_ARGS__)
#define GL_Bind(...) I_GlQuake._GL_Bind(__VA_ARGS__)
#define GL_DisableMultitexture(...) I_GlQuake._GL_DisableMultitexture(__VA_ARGS__)
#define GL_EnableMultitexture(...) I_GlQuake._GL_EnableMultitexture(__VA_ARGS__)

#define R_TimeRefresh_f(...) I_GlQuake._R_TimeRefresh_f(__VA_ARGS__)
#define R_ReadPointFile_f(...) I_GlQuake._R_ReadPointFile_f(__VA_ARGS__)
#define R_TextureAnimation(...) I_GlQuake._R_TextureAnimation(__VA_ARGS__)
#define R_TranslatePlayerSkin(...) I_GlQuake._R_TranslatePlayerSkin(__VA_ARGS__)
