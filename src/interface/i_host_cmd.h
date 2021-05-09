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

void Host_Quit_f(void);
void Host_Status_f(void);
void Host_God_f(void);
void Host_Notarget_f(void);
void Host_Noclip_f(void);
void Host_Fly_f(void);
void Host_Ping_f(void);
void Host_Map_f(void);
void Host_Changelevel_f(void);
void Host_Restart_f(void);
void Host_Reconnect_f(void);
void Host_Connect_f(void);
void Host_SavegameComment(char *text, i32 size);
void Host_Savegame_f(void);
void Host_Loadgame_f(void);
void Host_Name_f(void);
void Host_Version_f(void);
void Host_Say(bool teamonly);
void Host_Say_f(void);
void Host_Say_Team_f(void);
void Host_Tell_f(void);
void Host_Color_f(void);
void Host_Kill_f(void);
void Host_Pause_f(void);
void Host_PreSpawn_f(void);
void Host_Spawn_f(void);
void Host_Begin_f(void);
void Host_Kick_f(void);
void Host_Give_f(void);
void Host_Viewmodel_f(void);
void Host_Viewframe_f(void);
void Host_Viewnext_f(void);
void Host_Viewprev_f(void);
void Host_Startdemos_f(void);
void Host_Demos_f(void);
void Host_Stopdemo_f(void);
