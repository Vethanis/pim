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

typedef struct I_Server_s
{
    void(*_SV_Init)(void);
    void(*_SV_StartParticle)(vec3_t org, vec3_t dir, i32 color, i32 count);
    void(*_SV_StartSound)(edict_t *entity, i32 channel, const char *sample, i32 volume, float attenuation);
    void(*_SV_DropClient)(qboolean crash);
    void(*_SV_SendClientMessages)(void);
    void(*_SV_ClearDatagram)(void);
    i32(*_SV_ModelIndex)(const char *name);
    void(*_SV_SetIdealPitch)(void);
    void(*_SV_AddUpdates)(void);
    void(*_SV_ClientThink)(void);
    void(*_SV_AddClientToServer)(qsocket_t *ret);
    void(*_SV_ClientPrintf)(const char *fmt, ...);
    void(*_SV_BroadcastPrintf)(const char *fmt, ...);
    void(*_SV_Physics)(void);
    qboolean(*_SV_CheckBottom)(edict_t *ent);
    qboolean(*_SV_movestep)(edict_t *ent, vec3_t move, qboolean relink);
    void(*_SV_WriteClientdataToMessage)(edict_t *ent, sizebuf_t *msg);
    void(*_SV_MoveToGoal)(void);
    void(*_SV_CheckForNewClients)(void);
    void(*_SV_RunClients)(void);
    void(*_SV_SaveSpawnparms)();
    void(*_SV_SpawnServer)(const char *server);
} I_Server_t;
extern I_Server_t I_Server;

#define SV_Init(...) I_Server._SV_Init(__VA_ARGS__)
#define SV_StartParticle(...) I_Server._SV_StartParticle(__VA_ARGS__)
#define SV_StartSound(...) I_Server._SV_StartSound(__VA_ARGS__)
#define SV_DropClient(...) I_Server._SV_DropClient(__VA_ARGS__)
#define SV_SendClientMessages(...) I_Server._SV_SendClientMessages(__VA_ARGS__)
#define SV_ClearDatagram(...) I_Server._SV_ClearDatagram(__VA_ARGS__)
#define SV_ModelIndex(...) I_Server._SV_ModelIndex(__VA_ARGS__)
#define SV_SetIdealPitch(...) I_Server._SV_SetIdealPitch(__VA_ARGS__)
#define SV_AddUpdates(...) I_Server._SV_AddUpdates(__VA_ARGS__)
#define SV_ClientThink(...) I_Server._SV_ClientThink(__VA_ARGS__)
#define SV_AddClientToServer(...) I_Server._SV_AddClientToServer(__VA_ARGS__)
#define SV_ClientPrintf(...) I_Server._SV_ClientPrintf(__VA_ARGS__)
#define SV_BroadcastPrintf(...) I_Server._SV_BroadcastPrintf(__VA_ARGS__)
#define SV_Physics(...) I_Server._SV_Physics(__VA_ARGS__)
#define SV_CheckBottom(...) I_Server._SV_CheckBottom(__VA_ARGS__)
#define SV_movestep(...) I_Server._SV_movestep(__VA_ARGS__)
#define SV_WriteClientdataToMessage(...) I_Server._SV_WriteClientdataToMessage(__VA_ARGS__)
#define SV_MoveToGoal(...) I_Server._SV_MoveToGoal(__VA_ARGS__)
#define SV_CheckForNewClients(...) I_Server._SV_CheckForNewClients(__VA_ARGS__)
#define SV_RunClients(...) I_Server._SV_RunClients(__VA_ARGS__)
#define SV_SaveSpawnparms(...) I_Server._SV_SaveSpawnparms(__VA_ARGS__)
#define SV_SpawnServer(...) I_Server._SV_SpawnServer(__VA_ARGS__)
