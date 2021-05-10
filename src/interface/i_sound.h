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
// sound.h -- client sound i/o functions

#include "interface/i_types.h"

void S_Init(void);
void S_Startup(void);
void S_Shutdown(void);
void S_StartSound(i32 entnum, i32 entchannel, sfx_t *sfx, vec3_t origin, float fvol, float attenuation);
void S_StaticSound(sfx_t *sfx, vec3_t origin, float vol, float attenuation);
void S_StopSound(i32 entnum, i32 entchannel);
void S_StopAllSounds(qboolean clear);
void S_ClearBuffer(void);
void S_Update(vec3_t origin, vec3_t v_forward, vec3_t v_right, vec3_t v_up);
void S_ExtraUpdate(void);
sfx_t* S_PrecacheSound(const char *sample);
void S_TouchSound(const char *sample);
void S_ClearPrecache(void);
void S_BeginPrecaching(void);
void S_EndPrecaching(void);
void S_PaintChannels(i32 endtime);
void S_InitPaintChannels(void);
void S_LocalSound(const char* s);
sfxcache_t* S_LoadSound(sfx_t *s);
void S_AmbientOff(void);
void S_AmbientOn(void);

// picks a channel based on priorities, empty slots, number of channels
channel_t* SND_PickChannel(i32 entnum, i32 entchannel);
// spatializes a channel
void SND_Spatialize(channel_t *ch);
void SND_InitScaletable(void);

// initializes cycling through a DMA buffer and returns information on it
qboolean SNDDMA_Init(void);
// gets the current DMA position
i32 SNDDMA_GetDMAPos(void);
// shutdown the DMA xfer.
void SNDDMA_Shutdown(void);
void SNDDMA_Submit(void);

wavinfo_t GetWavinfo(const char *name, u8 *wav, i32 wavlength);
