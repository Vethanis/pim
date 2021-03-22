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

typedef struct I_Sound_s
{
    void _S_Init(void);
    void _S_Startup(void);
    void _S_Shutdown(void);
    void _S_StartSound(i32 entnum, i32 entchannel, sfx_t *sfx, vec3_t origin, float fvol, float attenuation);
    void _S_StaticSound(sfx_t *sfx, vec3_t origin, float vol, float attenuation);
    void _S_StopSound(i32 entnum, i32 entchannel);
    void _S_StopAllSounds(qboolean clear);
    void _S_ClearBuffer(void);
    void _S_Update(vec3_t origin, vec3_t v_forward, vec3_t v_right, vec3_t v_up);
    void _S_ExtraUpdate(void);
    sfx_t* _S_PrecacheSound(const char *sample);
    void _S_TouchSound(const char *sample);
    void _S_ClearPrecache(void);
    void _S_BeginPrecaching(void);
    void _S_EndPrecaching(void);
    void _S_PaintChannels(i32 endtime);
    void _S_InitPaintChannels(void);
    void _S_LocalSound(const char* s);
    sfxcache_t* _S_LoadSound(sfx_t *s);
    void _S_AmbientOff(void);
    void _S_AmbientOn(void);

    // picks a channel based on priorities, empty slots, number of channels
    channel_t* _SND_PickChannel(i32 entnum, i32 entchannel);
    // spatializes a channel
    void _SND_Spatialize(channel_t *ch);
    void _SND_InitScaletable(void);

    // initializes cycling through a DMA buffer and returns information on it
    qboolean _SNDDMA_Init(void);
    // gets the current DMA position
    i32 _SNDDMA_GetDMAPos(void);
    // shutdown the DMA xfer.
    void _SNDDMA_Shutdown(void);
    void _SNDDMA_Submit(void);

    wavinfo_t _GetWavinfo(const char *name, u8 *wav, i32 wavlength);
} I_Sound_t;
extern I_Sound_t I_Sound;

#define S_Init(...) I_Sound._S_Init(__VA_ARGS__)
#define S_Startup(...) I_Sound._S_Startup(__VA_ARGS__)
#define S_Shutdown(...) I_Sound._S_Shutdown(__VA_ARGS__)
#define S_StartSound(...) I_Sound._S_StartSound(__VA_ARGS__)
#define S_StaticSound(...) I_Sound._S_StaticSound(__VA_ARGS__)
#define S_StopSound(...) I_Sound._S_StopSound(__VA_ARGS__)
#define S_StopAllSounds(...) I_Sound._S_StopAllSounds(__VA_ARGS__)
#define S_ClearBuffer(...) I_Sound._S_ClearBuffer(__VA_ARGS__)
#define S_Update(...) I_Sound._S_Update(__VA_ARGS__)
#define S_ExtraUpdate(...) I_Sound._S_ExtraUpdate(__VA_ARGS__)
#define S_PrecacheSound(...) I_Sound._S_PrecacheSound(__VA_ARGS__)
#define S_TouchSound(...) I_Sound._S_TouchSound(__VA_ARGS__)
#define S_ClearPrecache(...) I_Sound._S_ClearPrecache(__VA_ARGS__)
#define S_BeginPrecaching(...) I_Sound._S_BeginPrecaching(__VA_ARGS__)
#define S_EndPrecaching(...) I_Sound._S_EndPrecaching(__VA_ARGS__)
#define S_PaintChannels(...) I_Sound._S_PaintChannels(__VA_ARGS__)
#define S_InitPaintChannels(...) I_Sound._S_InitPaintChannels(__VA_ARGS__)
#define S_LocalSound(...) I_Sound._S_LocalSound(__VA_ARGS__)
#define S_LoadSound(...) I_Sound._S_LoadSound(__VA_ARGS__)
#define S_AmbientOff(...) I_Sound._S_AmbientOff(__VA_ARGS__)
#define S_AmbientOn(...) I_Sound._S_AmbientOn(__VA_ARGS__)

#define SND_PickChannel(...) I_Sound._SND_PickChannel(__VA_ARGS__)
#define SND_Spatialize(...) I_Sound._SND_Spatialize(__VA_ARGS__)
#define SND_InitScaletable(...) I_Sound._SND_InitScaletable(__VA_ARGS__)

#define SNDDMA_Init(...) I_Sound._SNDDMA_Init(__VA_ARGS__)
#define SNDDMA_GetDMAPos(...) I_Sound._SNDDMA_GetDMAPos(__VA_ARGS__)
#define SNDDMA_Shutdown(...) I_Sound._SNDDMA_Shutdown(__VA_ARGS__)
#define SNDDMA_Submit(...) I_Sound._SNDDMA_Submit(__VA_ARGS__)

#define GetWavinfo(...) I_Sound._GetWavinfo(__VA_ARGS__)
