#include "interface/i_host.h"

#if QUAKE_IMPL

#include "interface/i_globals.h"
#include "interface/i_cvars.h"
#include "interface/i_common.h"
#include "interface/i_zone.h"
#include "interface/i_console.h"
#include "interface/i_model.h"
#include "interface/i_render.h"
#include <setjmp.h>

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

}

void Host_Init(quakeparms_t *parms)
{

}

void Host_Shutdown(void)
{

}

void Host_Error(const char *error, ...)
{

}

void Host_EndGame(const char *message, ...)
{

}

void Host_Frame(float time)
{

}

void Host_Quit_f(void)
{

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
