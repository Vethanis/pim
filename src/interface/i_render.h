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

// refresh.h -- public interface to refresh functions

#include "interface/i_types.h"

typedef struct I_Render_s
{
    void(*_R_Init)(void);
    void(*_R_InitTextures)(void);
    void(*_R_InitEfrags)(void);
    // must set r_refdef first
    void(*_R_RenderView)(void);
    // called whenever r_refdef or vid change
    void(*_R_ViewChanged)(vrect_t *prevrect, i32 lineadj, float aspect);
    // called at level load
    void(*_R_InitSky)(texture_t *mt);
    void(*_R_AddEfrags)(entity_t *ent);
    void(*_R_RemoveEfrags)(entity_t *ent);
    void(*_R_NewMap)(void);
    void(*_R_ParseParticleEffect)(void);
    void(*_R_RunParticleEffect)(vec3_t org, vec3_t dir, i32 color, i32 count);
    void(*_R_RocketTrail)(vec3_t start, vec3_t end, i32 type);
    void(*_R_EntityParticles)(entity_t *ent);
    void(*_R_BlobExplosion)(vec3_t org);
    void(*_R_ParticleExplosion)(vec3_t org);
    void(*_R_ParticleExplosion2)(vec3_t org, i32 colorStart, i32 colorLength);
    void(*_R_LavaSplash)(vec3_t org);
    void(*_R_TeleportSplash)(vec3_t org);
    void(*_R_PushDlights)(void);
    void(*_R_SetVrect)(vrect_t *pvrect, vrect_t *pvrectin, i32 lineadj);

    i32(*_D_SurfaceCacheForRes)(i32 width, i32 height);
    void(*_D_FlushCaches)(void);
    void(*_D_DeleteSurfaceCache)(void);
    void(*_D_InitCaches)(void *buffer, i32 size);
} I_Render_t;
extern I_Render_t I_Render;

#define R_Init(...) I_Render._R_Init(__VA_ARGS__)
#define R_InitTextures(...) I_Render._R_InitTextures(__VA_ARGS__)
#define R_InitEfrags(...) I_Render._R_InitEfrags(__VA_ARGS__)
#define R_RenderView(...) I_Render._R_RenderView(__VA_ARGS__)
#define R_ViewChanged(...) I_Render._R_ViewChanged(__VA_ARGS__)
#define R_InitSky(...) I_Render._R_InitSky(__VA_ARGS__)
#define R_AddEfrags(...) I_Render._R_AddEfrags(__VA_ARGS__)
#define R_RemoveEfrags(...) I_Render._R_RemoveEfrags(__VA_ARGS__)
#define R_NewMap(...) I_Render._R_NewMap(__VA_ARGS__)
#define R_ParseParticleEffect(...) I_Render._R_ParseParticleEffect(__VA_ARGS__)
#define R_RunParticleEffect(...) I_Render._R_RunParticleEffect(__VA_ARGS__)
#define R_RocketTrail(...) I_Render._R_RocketTrail(__VA_ARGS__)
#define R_EntityParticles(...) I_Render._R_EntityParticles(__VA_ARGS__)
#define R_BlobExplosion(...) I_Render._R_BlobExplosion(__VA_ARGS__)
#define R_ParticleExplosion(...) I_Render._R_ParticleExplosion(__VA_ARGS__)
#define R_ParticleExplosion2(...) I_Render._R_ParticleExplosion2(__VA_ARGS__)
#define R_LavaSplash(...) I_Render._R_LavaSplash(__VA_ARGS__)
#define R_TeleportSplash(...) I_Render._R_TeleportSplash(__VA_ARGS__)
#define R_PushDlights(...) I_Render._R_PushDlights(__VA_ARGS__)
#define R_SetVrect(...) I_Render._R_SetVrect(__VA_ARGS__)

#define D_SurfaceCacheForRes(...) I_Render._D_SurfaceCacheForRes(__VA_ARGS__)
#define D_FlushCaches(...) I_Render._D_FlushCaches(__VA_ARGS__)
#define D_DeleteSurfaceCache(...) I_Render._D_DeleteSurfaceCache(__VA_ARGS__)
#define D_InitCaches(...) I_Render._D_InitCaches(__VA_ARGS__)
