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

#include "interface/i_host.h"

#if QUAKE_IMPL

#include "interface/i_globals.h"
#include "interface/i_cvars.h"
#include "interface/i_cvar.h"
#include "interface/i_host_cmd.h"
#include "interface/i_sys.h"
#include "interface/i_common.h"
#include "interface/i_zone.h"
#include "interface/i_console.h"
#include "interface/i_model.h"
#include "interface/i_render.h"
#include "interface/i_server.h"
#include "interface/i_client.h"
#include "interface/i_cmd.h"
#include "interface/i_sound.h"
#include "interface/i_model.h"
#include "interface/i_screen.h"
#include "interface/i_input.h"
#include "interface/i_net.h"
#include "interface/i_cdaudio.h"
#include "interface/i_vid.h"
#include "interface/i_view.h"
#include "interface/i_wad.h"
#include "interface/i_keys.h"
#include "interface/i_chase.h"
#include "interface/i_render.h"
#include "interface/i_draw.h"
#include "interface/i_sbar.h"
#include "interface/i_menu.h"
#include "interface/i_progs.h"
#include "interface/i_msg.h"
#include "interface/i_sizebuf.h"

#include "common/stringutil.h"

#include <setjmp.h>
#include <stdarg.h>

static double s_oldrealtime;
static i32 s_host_hunklevel;
static jmp_buf s_host_abortserver;

void Host_ClearMemory(void)
{
    Con_DPrintf("Clearing memory\n");
    D_FlushCaches();
    Mod_ClearAll();
    if (s_host_hunklevel)
    {
        Hunk_FreeToLowMark(s_host_hunklevel);
    }

    g_cls.signon = 0;
    memset(&g_sv, 0, sizeof(g_sv));
    memset(&g_cl, 0, sizeof(g_cl));
}

void Host_InitCommands(void)
{
    Cmd_AddCommand("status", Host_Status_f);
    Cmd_AddCommand("quit", Host_Quit_f);
    Cmd_AddCommand("god", Host_God_f);
    Cmd_AddCommand("notarget", Host_Notarget_f);
    Cmd_AddCommand("fly", Host_Fly_f);
    Cmd_AddCommand("map", Host_Map_f);
    Cmd_AddCommand("restart", Host_Restart_f);
    Cmd_AddCommand("changelevel", Host_Changelevel_f);
    Cmd_AddCommand("connect", Host_Connect_f);
    Cmd_AddCommand("reconnect", Host_Reconnect_f);
    Cmd_AddCommand("name", Host_Name_f);
    Cmd_AddCommand("noclip", Host_Noclip_f);
    Cmd_AddCommand("version", Host_Version_f);
    Cmd_AddCommand("say", Host_Say_f);
    Cmd_AddCommand("say_team", Host_Say_Team_f);
    Cmd_AddCommand("tell", Host_Tell_f);
    Cmd_AddCommand("color", Host_Color_f);
    Cmd_AddCommand("kill", Host_Kill_f);
    Cmd_AddCommand("pause", Host_Pause_f);
    Cmd_AddCommand("spawn", Host_Spawn_f);
    Cmd_AddCommand("begin", Host_Begin_f);
    Cmd_AddCommand("prespawn", Host_PreSpawn_f);
    Cmd_AddCommand("kick", Host_Kick_f);
    Cmd_AddCommand("ping", Host_Ping_f);
    Cmd_AddCommand("load", Host_Loadgame_f);
    Cmd_AddCommand("save", Host_Savegame_f);
    Cmd_AddCommand("give", Host_Give_f);

    Cmd_AddCommand("startdemos", Host_Startdemos_f);
    Cmd_AddCommand("demos", Host_Demos_f);
    Cmd_AddCommand("stopdemo", Host_Stopdemo_f);

    Cmd_AddCommand("viewmodel", Host_Viewmodel_f);
    Cmd_AddCommand("viewframe", Host_Viewframe_f);
    Cmd_AddCommand("viewnext", Host_Viewnext_f);
    Cmd_AddCommand("viewprev", Host_Viewprev_f);

    Cmd_AddCommand("mcache", Mod_Print);
}

void Host_InitLocal(void)
{
    Host_InitCommands();
    Host_FindMaxClients();
    g_host_time = 1.0; // so a think at time 0 won't get called
}

void Host_Init(quakeparms_t *parms)
{
    if (g_standard_quake)
        g_minimum_memory = MINIMUM_MEMORY;
    else
        g_minimum_memory = MINIMUM_MEMORY_LEVELPAK;

    if (COM_CheckParm("-minmemory"))
        parms->memsize = g_minimum_memory;

    g_host_parms = *parms;

    if (parms->memsize < g_minimum_memory)
        Sys_Error("Only %4.1f megs of memory available, can't execute game", parms->memsize / (float)0x100000);

    g_com_argc = parms->argc;
    g_com_argv = parms->argv;

    Memory_Init(parms->memsize);
    Cbuf_Init();
    Cmd_Init();
    V_Init();
    Chase_Init();
    COM_Init(parms->basedir);
    Host_InitLocal();
    W_LoadWadFile("gfx.wad");
    Key_Init();
    Con_Init();
    M_Init();
    PR_Init();
    Mod_Init();
    NET_Init();
    SV_Init();

    Con_Printf("Exe: "__TIME__" "__DATE__"\n");
    Con_Printf("%4.1f megabyte heap\n", parms->memsize / (1024 * 1024.0));

    R_InitTextures(); // needed even for dedicated servers

    if (g_cls.state != ca_dedicated)
    {
        buffer_t palette = COM_LoadHunkFile("gfx/palette.lmp");
        buffer_t colormap = COM_LoadHunkFile("gfx/colormap.lmp");
        g_host_basepal = palette.ptr;
        g_host_colormap = colormap.ptr;
        if (!g_host_basepal)
            Sys_Error("Couldn't load gfx/palette.lmp");
        if (!g_host_colormap)
            Sys_Error("Couldn't load gfx/colormap.lmp");

        IN_Init();
        VID_Init(g_host_basepal);
        Draw_Init();
        SCR_Init();
        R_Init();
        S_Init();
        CDAudio_Init();
        Sbar_Init();
        CL_Init();
    }

    Cbuf_InsertText("exec quake.rc\n");

    Hunk_AllocName(0, "-HOST_HUNKLEVEL-");
    s_host_hunklevel = Hunk_LowMark();

    g_host_initialized = true;

    Sys_Printf("========Quake Initialized=========\n");
}

void Host_Shutdown(void)
{
    static bool isdown;
    if (isdown)
    {
        ASSERT(false);
        return;
    }
    isdown = true;

    // keep Con_Printf from trying to update the screen
    g_scr_disabled_for_loading = true;

    Host_WriteConfiguration();

    CDAudio_Shutdown();
    NET_Shutdown();
    S_Shutdown();
    IN_Shutdown();

    if (g_cls.state != ca_dedicated)
    {
        VID_Shutdown();
    }
}

void Host_Error(const char *error, ...)
{
    static bool inerror;
    if (inerror)
    {
        Sys_Error("Host_Error: recursively entered");
    }
    inerror = true;

    SCR_EndLoadingPlaque(); // reenable screen updates

    va_list va;
    char string[1024];
    va_start(va, error);
    VSPrintf(ARGS(string), error, va);
    va_end(va);
    Con_Printf("Host_Error: %s\n", string);

    if (g_sv.active)
    {
        Host_ShutdownServer(false);
    }

    if (g_cls.state == ca_dedicated)
    {
        Sys_Error("Host_Error: %s\n", string); // dedicated servers exit
    }

    CL_Disconnect();
    g_cls.demonum = -1;

    inerror = false;

    longjmp(s_host_abortserver, 1);
}

void Host_FindMaxClients(void)
{
    g_cls.state = ca_disconnected;
    g_svs.maxclients = 1;

    if (COM_CheckParm("-dedicated"))
    {
        g_cls.state = ca_dedicated;
        g_svs.maxclients = 8;
        const char* maxclients = COM_GetParm("-dedicated", 1);
        if (maxclients)
        {
            g_svs.maxclients = ParseInt(maxclients);
        }
    }
    if (COM_CheckParm("-listen"))
    {
        if (g_cls.state == ca_dedicated)
        {
            Sys_Error("Only one of -dedicated or -listen can be specified");
        }
        g_svs.maxclients = 8;
        const char* maxclients = COM_GetParm("-listen", 1);
        if (maxclients)
        {
            g_svs.maxclients = ParseInt(maxclients);
        }
    }
    if (g_svs.maxclients < 1)
    {
        g_svs.maxclients = 8;
    }
    g_svs.maxclients = pim_min(g_svs.maxclients, MAX_SCOREBOARD);

    memset(&g_svs.clients, 0, sizeof(g_svs.clients));
    Cvar_SetValue("deathmatch", (g_svs.maxclients > 1) ? 1.0f : 0.0f);
}

void Host_EndGame(const char *message, ...)
{
    va_list va;
    char string[1024];

    va_start(va, message);
    VSPrintf(ARGS(string), message, va);
    va_end(va);
    Con_DPrintf("Host_EndGame: %s\n", string);

    if (g_sv.active)
    {
        Host_ShutdownServer(false);
    }

    if (g_cls.state == ca_dedicated)
    {
        // dedicated servers exit
        Sys_Error("Host_EndGame: %s\n", string);
    }

    if (g_cls.demonum != -1)
    {
        CL_NextDemo();
    }
    else
    {
        CL_Disconnect();
    }

    longjmp(s_host_abortserver, 1);
}

bool Host_FilterTime(float time)
{
    g_realtime += time;

    const double limit = 1.0 / 72.0;
    double dt = g_realtime - s_oldrealtime;
    if (!g_cls.timedemo && dt < limit)
    {
        return false; // framerate is too high
    }

    g_host_frametime = g_realtime - s_oldrealtime;
    s_oldrealtime = g_realtime;

    if (cv_host_framerate.value > 0.0f)
    {
        g_host_frametime = cv_host_framerate.value;
    }
    else
    {
        // don't allow really long or short frames
        g_host_frametime = pim_min(g_host_frametime, 0.1);
        g_host_frametime = pim_max(g_host_frametime, 0.001);
    }

    return true;
}

void Host_GetConsoleCommands(void)
{
    while (1)
    {
        const char* cmd = Sys_ConsoleInput();
        if (!cmd)
            break;
        Cbuf_AddText(cmd);
    }
}

void Host_ServerFrame(void)
{
    // run the world state	
    g_pr_global_struct->frametime = g_host_frametime;

    // set the time and clear the general datagram
    SV_ClearDatagram();

    // check for new clients
    SV_CheckForNewClients();

    // read client messages
    SV_RunClients();

    // move things around and think
    // always pause in single player if in console or menus
    if (!g_sv.paused && (g_svs.maxclients > 1 || g_key_dest == key_game))
    {
        SV_Physics();
    }

    // send all messages to the clients
    SV_SendClientMessages();
}

void Host_Frame(float time)
{
    if (setjmp(s_host_abortserver))
    {
        // something bad happened, or the server disconnected
        return;
    }

    // keep the random time dependent
    //rand();

    // decide the simulation time
    if (!Host_FilterTime(time))
    {
        return; // don't run too fast, or packets will flood out
    }

    // get new key events
    Sys_SendKeyEvents();

    // allow mice or other external controllers to add commands
    IN_Commands();

    // process console commands
    Cbuf_Execute();

    NET_Poll();

    // if running the server locally, make intentions now
    if (g_sv.active)
    {
        CL_SendCmd();
    }

    //-------------------
    //
    // server operations
    //
    //-------------------

    // check for commands typed to the host
    Host_GetConsoleCommands();

    if (g_sv.active)
    {
        Host_ServerFrame();
    }

    //-------------------
    //
    // client operations
    //
    //-------------------

    // if running the server remotely, send intentions now after
    // the incoming messages have been read
    if (!g_sv.active)
    {
        CL_SendCmd();
    }

    g_host_time += g_host_frametime;

    // fetch results from server
    if (g_cls.state == ca_connected)
    {
        CL_ReadFromServer();
    }

    // update video
    SCR_UpdateScreen();

    // update audio
    if (g_cls.signon == SIGNONS)
    {
        S_Update(g_r_origin, g_vpn, g_vright, g_vup);
        CL_DecayLights();
    }
    else
    {
        S_Update(g_vec3_origin, g_vec3_origin, g_vec3_origin, g_vec3_origin);
    }

    CDAudio_Update();

    g_host_framecount++;
}

void Host_ClientCommands(const char *fmt, ...)
{
    va_list va;
    char string[1024];
    va_start(va, fmt);
    VSPrintf(ARGS(string), fmt, va);
    va_end(va);
    MSG_WriteByte(&g_host_client->message, svc_stufftext);
    MSG_WriteString(&g_host_client->message, string);
}

void Host_ShutdownServer(bool crash)
{
    if (!g_sv.active)
    {
        return;
    }
    g_sv.active = false;

    // stop all client sounds immediately
    if (g_cls.state == ca_connected)
    {
        CL_Disconnect();
    }

    // flush any pending messages - like the score!!!
    double start = Sys_FloatTime();
    i32 count = 0;
    do
    {
        count = 0;
        const i32 maxclients = g_svs.maxclients;
        for (i32 i = 0; i < maxclients; ++i)
        {
            client_t* client = &g_svs.clients[i];
            if (client->active && client->message.cursize)
            {
                if (NET_CanSendMessage(client->netconnection))
                {
                    NET_SendMessage(client->netconnection, &client->message);
                    SZ_Clear(&client->message);
                }
                else
                {
                    NET_GetMessage(client->netconnection);
                    count++;
                }
            }
        }
        if ((Sys_FloatTime() - start) > 3.0)
        {
            break;
        }
    } while (count);

    // make sure all the clients know we're disconnecting
    sizebuf_t buf = { 0 };
    MSG_WriteByte(&buf, svc_disconnect);
    count = NET_SendToAll(&buf, 5);
    if (count)
    {
        Con_Printf("Host_ShutdownServer: NET_SendToAll failed for %d clients\n", count);
    }
    SZ_Free(&buf);

    const i32 maxclients = g_svs.maxclients;
    for (i32 i = 0; i < maxclients; ++i)
    {
        client_t* client = &g_svs.clients[i];
        if (client->active)
        {
            SV_DropClient(crash, client);
        }
    }

    // clear structures
    memset(&g_sv, 0, sizeof(g_sv));
    memset(g_svs.clients, 0, sizeof(g_svs.clients));
}

void Host_WriteConfiguration(void)
{
    // dedicated servers initialize the host but don't parse and set the
    // config.cfg cvars
    if (g_host_initialized & !g_isDedicated)
    {
        filehdl_t f;
        if (!Sys_FileOpenWrite(va("%s/config.cfg", g_com_gamedir), &f))
        {
            Con_Printf("Couldn't write config.cfg.\n");
            return;
        }

        Key_WriteBindings(f);
        Cvar_WriteVariables(f);

        Sys_FileClose(f);
    }
}

#endif // QUAKE_IMPL
