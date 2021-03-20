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

typedef struct I_Client_s
{
    void(*_CL_Init)(void);
    void(*_CL_EstablishConnection)(char *host);
    void(*_CL_Signon1)(void);
    void(*_CL_Signon2)(void);
    void(*_CL_Signon3)(void);
    void(*_CL_Signon4)(void);
    void(*_CL_SignonReply)(void);
    void(*_CL_Disconnect)(void);
    void(*_CL_Disconnect_f)(void);
    float(*_CL_KeyState)(kbutton_t *key);
    void(*_CL_InitInput)(void);
    void(*_CL_SendCmd)(void);
    void(*_CL_SendMove)(usercmd_t *cmd);
    i32(*_CL_ReadFromServer)(void);
    void(*_CL_WriteToServer)(usercmd_t *cmd);
    void(*_CL_BaseMove)(usercmd_t *cmd);
    void(*_CL_ClearState)(void);
    void(*_CL_StopPlayback)(void);
    i32(*_CL_GetMessage)(void);
    void(*_CL_NextDemo)(void);
    void(*_CL_Stop_f)(void);
    void(*_CL_Record_f)(void);
    void(*_CL_PlayDemo_f)(void);
    void(*_CL_TimeDemo_f)(void);
    void(*_CL_ParseServerMessage)(void);
    void(*_CL_NewTranslation)(i32 slot);
    void(*_CL_InitTEnts)(void);
    void(*_CL_ParseTEnt)(void);
    void(*_CL_UpdateTEnts)(void);
    dlight_t*(*_CL_AllocDlight)(i32 key);
    void(*_CL_DecayLights)(void);

    void(*_V_StartPitchDrift)(void);
    void(*_V_StopPitchDrift)(void);
    void(*_V_RenderView)(void);
    void(*_V_UpdatePalette)(void);
    void(*_V_Register)(void);
    void(*_V_ParseDamage)(void);
    void(*_V_SetContentsColor)(i32 contents);

    const char*(*_Key_KeynumToString)(i32 keynum);
} I_Client_t;
extern I_Client_t I_Client;

#define CL_Init(...) I_Client._CL_Init(__VA_ARGS__)
#define CL_EstablishConnection(...) I_Client._CL_EstablishConnection(__VA_ARGS__)
#define CL_Signon1(...) I_Client._CL_Signon1(__VA_ARGS__)
#define CL_Signon2(...) I_Client._CL_Signon2(__VA_ARGS__)
#define CL_Signon3(...) I_Client._CL_Signon3(__VA_ARGS__)
#define CL_Signon4(...) I_Client._CL_Signon4(__VA_ARGS__)
#define CL_SignonReply(...) I_Client._CL_SignonReply(__VA_ARGS__)
#define CL_Disconnect(...) I_Client._CL_Disconnect(__VA_ARGS__)
#define CL_Disconnect_f(...) I_Client._CL_Disconnect_f(__VA_ARGS__)
#define CL_KeyState(...) I_Client._CL_KeyState(__VA_ARGS__)
#define CL_InitInput(...) I_Client._CL_InitInput(__VA_ARGS__)
#define CL_SendCmd(...) I_Client._CL_SendCmd(__VA_ARGS__)
#define CL_SendMove(...) I_Client._CL_SendMove(__VA_ARGS__)
#define CL_ReadFromServer(...) I_Client._CL_ReadFromServer(__VA_ARGS__)
#define CL_WriteToServer(...) I_Client._CL_WriteToServer(__VA_ARGS__)
#define CL_BaseMove(...) I_Client._CL_BaseMove(__VA_ARGS__)
#define CL_ClearState(...) I_Client._CL_ClearState(__VA_ARGS__)
#define CL_StopPlayback(...) I_Client._CL_StopPlayback(__VA_ARGS__)
#define CL_GetMessage(...) I_Client._CL_GetMessage(__VA_ARGS__)
#define CL_NextDemo(...) I_Client._CL_NextDemo(__VA_ARGS__)
#define CL_Stop_f(...) I_Client._CL_Stop_f(__VA_ARGS__)
#define CL_Record_f(...) I_Client._CL_Record_f(__VA_ARGS__)
#define CL_PlayDemo_f(...) I_Client._CL_PlayDemo_f(__VA_ARGS__)
#define CL_TimeDemo_f(...) I_Client._CL_TimeDemo_f(__VA_ARGS__)
#define CL_ParseServerMessage(...) I_Client._CL_ParseServerMessage(__VA_ARGS__)
#define CL_NewTranslation(...) I_Client._CL_NewTranslation(__VA_ARGS__)
#define CL_InitTEnts(...) I_Client._CL_InitTEnts(__VA_ARGS__)
#define CL_ParseTEnt(...) I_Client._CL_ParseTEnt(__VA_ARGS__)
#define CL_UpdateTEnts(...) I_Client._CL_UpdateTEnts(__VA_ARGS__)
#define CL_AllocDlight(...) I_Client._CL_AllocDlight(__VA_ARGS__)
#define CL_DecayLights(...) I_Client._CL_DecayLights(__VA_ARGS__)

#define V_StartPitchDrift(...) I_Client._V_StartPitchDrift(__VA_ARGS__)
#define V_StopPitchDrift(...) I_Client._V_StopPitchDrift(__VA_ARGS__)
#define V_RenderView(...) I_Client._V_RenderView(__VA_ARGS__)
#define V_UpdatePalette(...) I_Client._V_UpdatePalette(__VA_ARGS__)
#define V_Register(...) I_Client._V_Register(__VA_ARGS__)
#define V_ParseDamage(...) I_Client._V_ParseDamage(__VA_ARGS__)
#define V_SetContentsColor(...) I_Client._V_SetContentsColor(__VA_ARGS__)

#define Key_KeynumToString(...) I_Client._Key_KeynumToString(__VA_ARGS__)
