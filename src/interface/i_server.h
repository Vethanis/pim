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
// server.h

#include "interface/i_types.h"

void SV_Init(void);
void SV_StartParticle(vec3_t org, vec3_t dir, i32 color, i32 count);
void SV_StartSound(edict_t *entity, i32 channel, const char *sample, i32 volume, float attenuation);
void SV_DropClient(qboolean crash, client_t* client);
void SV_SendClientMessages(void);
void SV_ClearDatagram(void);
i32 SV_ModelIndex(const char *name);
void SV_SetIdealPitch(void);
void SV_AddUpdates(void);
void SV_ClientThink(void);
void SV_AddClientToServer(qsocket_t *ret);
void SV_ClientPrintf(client_t* client, const char *fmt, ...);
void SV_BroadcastPrintf(const char *fmt, ...);
void SV_Physics(void);
qboolean SV_CheckBottom(edict_t *ent);
qboolean SV_movestep(edict_t *ent, vec3_t move, qboolean relink);
void SV_WriteClientdataToMessage(edict_t *ent, sizebuf_t *msg);
void SV_MoveToGoal(void);
void SV_CheckForNewClients(void);
void SV_RunClients(void);
void SV_SaveSpawnparms();
void SV_SpawnServer(const char *server);
