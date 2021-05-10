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

void R_Init(void);
void R_InitTextures(void);
void R_InitEfrags(void);
// must set r_refdef first
void R_RenderView(void);
// called whenever r_refdef or vid change
void R_ViewChanged(vrect_t *prevrect, i32 lineadj, float aspect);
// called at level load
void R_InitSky(texture_t *mt);
void R_AddEfrags(entity_t *ent);
void R_RemoveEfrags(entity_t *ent);
void R_NewMap(void);
void R_ParseParticleEffect(void);
void R_RunParticleEffect(vec3_t org, vec3_t dir, i32 color, i32 count);
void R_RocketTrail(vec3_t start, vec3_t end, i32 type);
void R_EntityParticles(entity_t *ent);
void R_BlobExplosion(vec3_t org);
void R_ParticleExplosion(vec3_t org);
void R_ParticleExplosion2(vec3_t org, i32 colorStart, i32 colorLength);
void R_LavaSplash(vec3_t org);
void R_TeleportSplash(vec3_t org);
void R_PushDlights(void);
void R_SetVrect(vrect_t *pvrect, vrect_t *pvrectin, i32 lineadj);

i32 D_SurfaceCacheForRes(i32 width, i32 height);
void D_FlushCaches(void);
void D_DeleteSurfaceCache(void);
void D_InitCaches(void *buffer, i32 size);
