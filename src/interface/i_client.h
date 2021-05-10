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

void CL_Init(void);
void CL_EstablishConnection(const char *host);
void CL_Signon1(void);
void CL_Signon2(void);
void CL_Signon3(void);
void CL_Signon4(void);
void CL_SignonReply(void);
void CL_Disconnect(void);
void CL_Disconnect_f(void);
float CL_KeyState(kbutton_t *key);
void CL_InitInput(void);
void CL_SendCmd(void);
void CL_SendMove(usercmd_t *cmd);
i32 CL_ReadFromServer(void);
void CL_WriteToServer(usercmd_t *cmd);
void CL_BaseMove(usercmd_t *cmd);
void CL_ClearState(void);
void CL_StopPlayback(void);
i32 CL_GetMessage(void);
void CL_NextDemo(void);
void CL_Stop_f(void);
void CL_Record_f(void);
void CL_PlayDemo_f(void);
void CL_TimeDemo_f(void);
void CL_ParseServerMessage(void);
void CL_NewTranslation(i32 slot);
void CL_InitTEnts(void);
void CL_ParseTEnt(void);
void CL_UpdateTEnts(void);
dlight_t* CL_AllocDlight(i32 key);
void CL_DecayLights(void);
