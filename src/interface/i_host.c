#include "interface/i_host.h"

#if QUAKE_IMPL

#include "interface/i_globals.h"
#include "interface/i_cvars.h"
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

void Host_Init(quakeparms_t *parms)
{

}

void Host_Shutdown(void)
{

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

}

void Host_ShutdownServer(qboolean crash)
{

}


void Chase_Init(void)
{

}

void Chase_Reset(void)
{

}

void Chase_Update(void)
{

}

#endif // QUAKE_IMPL
