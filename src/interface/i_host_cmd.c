#include "interface/i_host_cmd.h"

#if QUAKE_IMPL

#include "interface/i_globals.h"
#include "interface/i_cvars.h"
#include "interface/i_sys.h"
#include "interface/i_common.h"
#include "interface/i_cmd.h"
#include "interface/i_console.h"
#include "interface/i_client.h"
#include "interface/i_server.h"
#include "interface/i_host.h"
#include "interface/i_menu.h"
#include "interface/i_cvar.h"
#include "interface/i_screen.h"
#include "interface/i_progs.h"
#include "interface/i_world.h"
#include "interface/i_zone.h"
#include "interface/i_model.h"

#include "common/stringutil.h"

/*
==================
Host_Quit_f
==================
*/

void Host_Quit_f(void)
{
    if (g_key_dest != key_console && g_cls.state != ca_dedicated)
    {
        M_Menu_Quit_f();
        return;
    }
    CL_Disconnect();
    Host_ShutdownServer(false);

    Sys_Quit();
}


/*
==================
Host_Status_f
==================
*/
void Host_Status_f(void)
{
    client_t *client;
    int   seconds;
    int   minutes;
    int   hours = 0;
    int   j;
    void(*print) (const char *fmt, ...);

    if (g_cmd_source == src_command)
    {
        if (!g_sv.active)
        {
            Cmd_ForwardToServer();
            return;
        }
        print = Con_Printf;
    }
    else
    {
        print = SV_ClientPrintf;
    }

    print("host:    %s\n", Cvar_VariableString("hostname"));
    print("version: %4.2f\n", VERSION);
    if (g_tcpipAvailable)
        print("tcp/ip:  %s\n", g_my_tcpip_address);
    print("map:     %s\n", g_sv.name);
    print("players: %i active (%i max)\n\n", g_net_activeconnections, g_svs.maxclients);
    for (j = 0, client = g_svs.clients; j < g_svs.maxclients; j++, client++)
    {
        if (!client->active)
            continue;
        seconds = (int)(g_net_time - client->netconnection->connecttime);
        minutes = seconds / 60;
        if (minutes)
        {
            seconds -= (minutes * 60);
            hours = minutes / 60;
            if (hours)
                minutes -= (hours * 60);
        }
        else
        {
            hours = 0;
        }
        print("#%-2u %-16.16s  %3i  %2i:%02i:%02i\n", j + 1, client->name, (int)client->edict->v.frags, hours, minutes, seconds);
        print("   %s\n", client->netconnection->address);
    }
}


/*
==================
Host_God_f

Sets client to godmode
==================
*/
void Host_God_f(void)
{
    if (g_cmd_source == src_command)
    {
        Cmd_ForwardToServer();
        return;
    }

    if (g_pr_global_struct->deathmatch && !g_host_client->privileged)
        return;

    i32 flags = (i32)g_sv_player->v.flags;
    flags ^= FL_GODMODE;
    g_sv_player->v.flags = (float)flags;
    if (!(flags & FL_GODMODE))
        SV_ClientPrintf("godmode OFF\n");
    else
        SV_ClientPrintf("godmode ON\n");
}

void Host_Notarget_f(void)
{
    if (g_cmd_source == src_command)
    {
        Cmd_ForwardToServer();
        return;
    }

    if (g_pr_global_struct->deathmatch && !g_host_client->privileged)
        return;

    i32 flags = (i32)g_sv_player->v.flags;
    flags ^= FL_NOTARGET;
    g_sv_player->v.flags = (float)flags;
    if (!(flags & FL_NOTARGET))
        SV_ClientPrintf("notarget OFF\n");
    else
        SV_ClientPrintf("notarget ON\n");
}

void Host_Noclip_f(void)
{
    if (g_cmd_source == src_command)
    {
        Cmd_ForwardToServer();
        return;
    }

    if (g_pr_global_struct->deathmatch && !g_host_client->privileged)
        return;

    if (g_sv_player->v.movetype != MOVETYPE_NOCLIP)
    {
        g_noclip_anglehack = true;
        g_sv_player->v.movetype = MOVETYPE_NOCLIP;
        SV_ClientPrintf("noclip ON\n");
    }
    else
    {
        g_noclip_anglehack = false;
        g_sv_player->v.movetype = MOVETYPE_WALK;
        SV_ClientPrintf("noclip OFF\n");
    }
}

/*
==================
Host_Fly_f

Sets client to flymode
==================
*/
void Host_Fly_f(void)
{
    if (g_cmd_source == src_command)
    {
        Cmd_ForwardToServer();
        return;
    }

    if (g_pr_global_struct->deathmatch && !g_host_client->privileged)
        return;

    if (g_sv_player->v.movetype != MOVETYPE_FLY)
    {
        g_sv_player->v.movetype = MOVETYPE_FLY;
        SV_ClientPrintf("flymode ON\n");
    }
    else
    {
        g_sv_player->v.movetype = MOVETYPE_WALK;
        SV_ClientPrintf("flymode OFF\n");
    }
}


/*
==================
Host_Ping_f

==================
*/
void Host_Ping_f(void)
{
    if (g_cmd_source == src_command)
    {
        Cmd_ForwardToServer();
        return;
    }

    SV_ClientPrintf("Client ping times:\n");
    const i32 maxclients = g_svs.maxclients;
    for (i32 i = 0; i < maxclients; ++i)
    {
        client_t* client = client = &g_svs.clients[i];
        if (!client->active)
        {
            continue;
        }
        float total = 0.0f;
        for (i32 j = 0; j < NELEM(client->ping_times); j++)
        {
            total += client->ping_times[j];
        }
        total /= NUM_PING_TIMES;
        SV_ClientPrintf("%4d %s\n", (i32)(total * 1000), client->name);
    }
}

/*
===============================================================================

SERVER TRANSITIONS

===============================================================================
*/


/*
======================
Host_Map_f

handle a
map <servername>
command from the console.  Active clients are kicked off.
======================
*/
void Host_Map_f(void)
{
    int  i;
    char name[MAX_QPATH];

    if (g_cmd_source != src_command)
        return;

    g_cls.demonum = -1;  // stop demo loop in case this fails

    CL_Disconnect();
    Host_ShutdownServer(false);

    g_key_dest = key_game;   // remove console or menu
    SCR_BeginLoadingPlaque();

    g_cls.mapstring[0] = 0;
    for (i = 0; i < Cmd_Argc(); i++)
    {
        strcat(g_cls.mapstring, Cmd_Argv(i));
        strcat(g_cls.mapstring, " ");
    }
    strcat(g_cls.mapstring, "\n");

    g_svs.serverflags = 0;   // haven't completed an episode yet
    strcpy(name, Cmd_Argv(1));
    SV_SpawnServer(name);
    if (!g_sv.active)
        return;

    if (g_cls.state != ca_dedicated)
    {
        strcpy(g_cls.spawnparms, "");

        for (i = 2; i < Cmd_Argc(); i++)
        {
            strcat(g_cls.spawnparms, Cmd_Argv(i));
            strcat(g_cls.spawnparms, " ");
        }

        Cmd_ExecuteString("connect local", src_command);
    }
}

/*
==================
Host_Changelevel_f

Goes to a new map, taking all clients along
==================
*/
void Host_Changelevel_f(void)
{
    char level[MAX_QPATH];

    if (Cmd_Argc() != 2)
    {
        Con_Printf("changelevel <levelname> : continue game on a new level\n");
        return;
    }
    if (!g_sv.active || g_cls.demoplayback)
    {
        Con_Printf("Only the server may changelevel\n");
        return;
    }
    SV_SaveSpawnparms();
    strcpy(level, Cmd_Argv(1));
    SV_SpawnServer(level);
}

/*
==================
Host_Restart_f

Restarts the current server for a dead player
==================
*/
void Host_Restart_f(void)
{
    char mapname[MAX_QPATH];

    if (g_cls.demoplayback || !g_sv.active)
        return;

    if (g_cmd_source != src_command)
        return;
    strcpy(mapname, g_sv.name); // must copy out, because it gets cleared
                                // in sv_spawnserver
    SV_SpawnServer(mapname);
}

/*
==================
Host_Reconnect_f

This command causes the client to wait for the signon messages again.
This is sent just before a server changes levels
==================
*/
void Host_Reconnect_f(void)
{
    SCR_BeginLoadingPlaque();
    g_cls.signon = 0;  // need new connection messages
}

/*
=====================
Host_Connect_f

User command to connect to server
=====================
*/
void Host_Connect_f(void)
{
    char name[MAX_QPATH];

    g_cls.demonum = -1;  // stop demo loop in case this fails
    if (g_cls.demoplayback)
    {
        CL_StopPlayback();
        CL_Disconnect();
    }
    strcpy(name, Cmd_Argv(1));
    CL_EstablishConnection(name);
    Host_Reconnect_f();
}


/*
===============================================================================

LOAD / SAVE GAME

===============================================================================
*/

/*
===============
Host_SavegameComment

Writes a SAVEGAME_COMMENT_LENGTH character comment describing the current
===============
*/
void Host_SavegameComment(char *text, i32 size)
{
    StrCpy(text, size, g_cl.levelname);
    StrCatf(text, size, " kills:%3i/%3i", g_cl.stats[STAT_MONSTERS], g_cl.stats[STAT_TOTALMONSTERS]);
}


/*
===============
Host_Savegame_f
===============
*/
void Host_Savegame_f(void)
{
    if (g_cmd_source != src_command)
        return;

    if (!g_sv.active)
    {
        Con_Printf("Not playing a local game.\n");
        return;
    }

    if (g_cl.intermission)
    {
        Con_Printf("Can't save in intermission.\n");
        return;
    }

    if (g_svs.maxclients != 1)
    {
        Con_Printf("Can't save multiplayer games.\n");
        return;
    }

    if (Cmd_Argc() != 2)
    {
        Con_Printf("save <savename> : save a game\n");
        return;
    }

    if (strstr(Cmd_Argv(1), ".."))
    {
        Con_Printf("Relative pathnames are not allowed.\n");
        return;
    }

    for (i32 i = 0; i < g_svs.maxclients; i++)
    {
        const client_t* client = &g_svs.clients[i];
        if (client->active && (client->edict->v.health <= 0.0f))
        {
            Con_Printf("Can't savegame with a dead player\n");
            return;
        }
    }

    char name[PIM_PATH] = { 0 };
    SPrintf(ARGS(name), "%s/%s", g_com_gamedir, Cmd_Argv(1));
    COM_DefaultExtension(ARGS(name), ".sav");

    Con_Printf("Saving game to %s...\n", name);
    filehdl_t f;
    if (Sys_FileOpenWrite(name, &f))
    {
        Sys_FilePrintf(f, "%d\n", SAVEGAME_VERSION);

        char comment[SAVEGAME_COMMENT_LENGTH + 1] = { 0 };
        Host_SavegameComment(ARGS(comment));
        Sys_FilePrintf(f, "%s\n", comment);

        for (i32 i = 0; i < NUM_SPAWN_PARMS; i++)
        {
            Sys_FilePrintf(f, "%f\n", g_svs.clients->spawn_parms[i]);
        }

        Sys_FilePrintf(f, "%d\n", g_current_skill);
        Sys_FilePrintf(f, "%s\n", g_sv.name);
        Sys_FilePrintf(f, "%f\n", g_sv.time);

        // write the light styles
        for (i32 i = 0; i < MAX_LIGHTSTYLES; i++)
        {
            if (g_sv.lightstyles[i])
                Sys_FilePrintf(f, "%s\n", g_sv.lightstyles[i]);
            else
                Sys_FilePrintf(f, "m\n");
        }

        ED_WriteGlobals(f);
        for (i32 i = 0; i < g_sv.num_edicts; i++)
        {
            ED_Write(f, EDICT_NUM(i));
        }

        Sys_FileClose(f);
        Con_Printf("done.\n");
    }
    else
    {
        Con_Printf("ERROR: couldn't open %s.\n", name);
    }
}


/*
===============
Host_Loadgame_f
===============
*/
void Host_Loadgame_f(void)
{
    if (g_cmd_source != src_command)
        return;

    if (Cmd_Argc() != 2)
    {
        Con_Printf("load <savename> : load a game\n");
        return;
    }

    g_cls.demonum = -1; // stop demo loop in case this fails

    char name[PIM_PATH];
    SPrintf(ARGS(name), "%s/%s", g_com_gamedir, Cmd_Argv(1));
    COM_DefaultExtension(ARGS(name), ".sav");

    // we can't call SCR_BeginLoadingPlaque, because too much stack space has
    // been used.  The menu calls it before stuffing loadgame command
    // SCR_BeginLoadingPlaque ();

    Con_Printf("Loading game from %s...\n", name);
    filehdl_t f;
    if (!Sys_FileOpenRead(name, &f))
    {
        Con_Printf("ERROR: couldn't open %s.\n", name);
    }

    i32 version = 0;
    Sys_FileScanf(f, "%i\n", &version);
    if (version != SAVEGAME_VERSION)
    {
        Sys_FileClose(f);
        Con_Printf("Savegame is version %i, not %i\n", version, SAVEGAME_VERSION);
        return;
    }

    char str[32768]; // savegame comment
    Sys_FileScanf(f, "%s\n", str);

    float spawn_parms[NUM_SPAWN_PARMS];
    for (i32 i = 0; i < NUM_SPAWN_PARMS; i++)
    {
        Sys_FileScanf(f, "%f\n", &spawn_parms[i]);
    }

    // this silliness is so we can load 1.06 save files, which have float skill values
    float tfloat = 0.0f;
    Sys_FileScanf(f, "%f\n", &tfloat);
    g_current_skill = (int)(tfloat + 0.1);
    Cvar_SetValue("skill", (float)g_current_skill);

    char mapname[PIM_PATH];
    Sys_FileScanf(f, "%s\n", mapname);

    float time = 0.0f;
    Sys_FileScanf(f, "%f\n", &time);

    CL_Disconnect_f();

    SV_SpawnServer(mapname);
    if (!g_sv.active)
    {
        Con_Printf("Couldn't load map\n");
        return;
    }
    g_sv.paused = true; // pause until all clients connect
    g_sv.loadgame = true;

    // load the light styles
    for (i32 i = 0; i < MAX_LIGHTSTYLES; i++)
    {
        str[0] = 0;
        Sys_FileScanf(f, "%s\n", str);
        i32 styleLen = StrNLen(ARGS(str));
        ASSERT(styleLen <= 256);
        char* style = Hunk_Alloc(styleLen + 1);
        memcpy(style, str, styleLen);
        style[styleLen] = 0;
        g_sv.lightstyles[i] = style;
    }

    // load the edicts out of the savegame file
    i32 entnum = -1; // -1 is the globals
    while (!Sys_FileAtEnd(f))
    {
        str[0] = 0;
        i32 i = 0;
        for (; i < (sizeof(str) - 1); ++i)
        {
            i32 r = Sys_FileGetc(f);
            if (r > 0 && r < 128)
            {
                str[i] = (char)r;
                if (r == '}')
                {
                    ++i;
                    break;
                }
            }
            else
            {
                break;
            }
        }
        if (i == (sizeof(str) - 1))
        {
            Sys_Error("Loadgame buffer overflow");
        }
        str[i] = 0;

        const char* start = COM_Parse(str);
        if (!g_com_token[0])
        {
            break; // end of file
        }
        if (g_com_token[0] != '{')
        {
            Sys_Error("First token isn't a brace");
        }

        if (entnum == -1)
        {
            // parse the global vars
            ED_ParseGlobals(start);
        }
        else
        {
            // parse an edict
            edict_t* ent = EDICT_NUM(entnum);
            memset(&ent->v, 0, g_progs->entityfields * 4);
            ent->free = false;
            ED_ParseEdict(start, ent);

            // link it into the bsp tree
            if (!ent->free)
            {
                SV_LinkEdict(ent, false);
            }
        }

        ++entnum;
    }

    g_sv.num_edicts = entnum;
    g_sv.time = time;

    Sys_FileClose(f);

    for (i32 i = 0; i < NUM_SPAWN_PARMS; i++)
    {
        g_svs.clients->spawn_parms[i] = spawn_parms[i];
    }

    if (g_cls.state != ca_dedicated)
    {
        CL_EstablishConnection("local");
        Host_Reconnect_f();
    }
}

//============================================================================

/*
======================
Host_Name_f
======================
*/
void Host_Name_f(void)
{
    if (Cmd_Argc() == 1)
    {
        Con_Printf("\"name\" is \"%s\"\n", cv_cl_name.stringValue);
        return;
    }

    char newName[PIM_PATH] = { 0 };
    if (Cmd_Argc() == 2)
    {
        StrCpy(ARGS(newName), Cmd_Argv(1));
    }
    else
    {
        StrCpy(ARGS(newName), Cmd_Args());
    }

    if (g_cmd_source == src_command)
    {
        if (Q_strcmp(cv_cl_name.stringValue, newName) == 0)
            return;
        Cvar_Set("_cl_name", newName);
        if (g_cls.state == ca_connected)
            Cmd_ForwardToServer();
        return;
    }

    if (g_host_client->name[0] && StrCmp(ARGS(g_host_client->name), "unconnected") != 0)
    {
        if (StrCmp(ARGS(g_host_client->name), newName) != 0)
        {
            Con_Printf("'%s' renamed to '%s'\n", g_host_client->name, newName);
        }
    }
    StrCpy(ARGS(g_host_client->name), newName);

    // TODO: FIXME This is unsafe! May have to switch progs to 64bit
    ASSERT(false);
    //g_host_client->edict->v.netname = g_host_client->name - g_pr_strings;

    // send notification to all clients
    i32 clientIndex = (i32)(g_host_client - g_svs.clients);
    ASSERT(clientIndex >= 0);
    ASSERT(clientIndex < g_svs.maxclients);
    MSG_WriteByte(&g_sv.reliable_datagram, svc_updatename);
    MSG_WriteByte(&g_sv.reliable_datagram, clientIndex);
    MSG_WriteString(&g_sv.reliable_datagram, g_host_client->name);
}


void Host_Version_f(void)
{
    Con_Printf("Version %4.2f\n", VERSION);
    Con_Printf("Exe: "__TIME__" "__DATE__"\n");
}

void Host_Say(bool teamonly)
{
    bool fromServer = false;
    if (g_cmd_source == src_command)
    {
        if (g_cls.state == ca_dedicated)
        {
            fromServer = true;
            teamonly = false;
        }
        else
        {
            Cmd_ForwardToServer();
            return;
        }
    }

    if (Cmd_Argc() < 2)
        return;

    const char* args = Cmd_Args();
    char argbuf[PIM_PATH] = { 0 };
    i32 arglen = StrCpy(ARGS(argbuf), args);
    ChrRep(ARGS(argbuf), '"', ' ');

    char text[PIM_PATH] = { 0 };
    // turn on color set 1
    if (!fromServer)
    {
        SPrintf(ARGS(text), "%c%s: ", 1, g_host_client->name);
    }
    else
    {
        SPrintf(ARGS(text), "%c<%s> ", 1, cv_hostname.stringValue);
    }

    StrCatf(ARGS(text), "%s\n", argbuf);

    client_t* save = g_host_client;
    const i32 maxclients = g_svs.maxclients;
    for (i32 i = 0; i < maxclients; i++)
    {
        client_t* client = &g_svs.clients[i];
        if (!client || !client->active || !client->spawned)
        {
            continue;
        }
        if (teamonly &&
            (cv_teamplay.value != 0.0f) &&
            (client->edict->v.team != save->edict->v.team))
        {
            continue;
        }
        g_host_client = client;
        SV_ClientPrintf("%s", text);
    }
    g_host_client = save;

    Sys_Printf("%s", &text[1]);
}

void Host_Say_f(void)
{
    Host_Say(false);
}


void Host_Say_Team_f(void)
{
    Host_Say(true);
}

void Host_Tell_f(void)
{
    if (g_cmd_source == src_command)
    {
        Cmd_ForwardToServer();
        return;
    }

    if (Cmd_Argc() < 3)
        return;

    char text[PIM_PATH] = { 0 };
    StrCpy(ARGS(text), g_host_client->name);
    StrCat(ARGS(text), ": ");

    const char* args = Cmd_Args();
    char argbuf[PIM_PATH] = { 0 };
    i32 arglen = StrCpy(ARGS(argbuf), args);
    ChrRep(ARGS(argbuf), '"', ' ');

    StrCatf(ARGS(text), "%s\n", argbuf);

    client_t* save = g_host_client;
    const i32 maxclients = g_svs.maxclients;
    const char* sendee = Cmd_Argv(1);
    for (i32 i = 0; i < maxclients; ++i)
    {
        client_t* client = &g_svs.clients[i];
        if (!client->active || !client->spawned)
        {
            continue;
        }
        if (StrICmp(ARGS(client->name), sendee) != 0)
        {
            continue;
        }
        g_host_client = client;
        SV_ClientPrintf("%s", text);
        break;
    }
    g_host_client = save;
}


/*
==================
Host_Color_f
==================
*/
void Host_Color_f(void)
{
    const i32 argc = Cmd_Argc();
    if (argc == 1)
    {
        return;
    }

    i32 top = 0;
    i32 bottom = 0;
    if (Cmd_Argc() == 2)
    {
        top = ParseInt(Cmd_Argv(1));
        bottom = top;
    }
    else if (Cmd_Argc() == 3)
    {
        top = ParseInt(Cmd_Argv(1));
        bottom = ParseInt(Cmd_Argv(2));
    }
    else
    {
        i32 value = (i32)cv_cl_color.value;
        i32 a = value >> 4;
        i32 b = value & 15;
        Con_Printf("\"color\" is \"%d %d\"\n", a, b);
        Con_Printf("color <0-13> [0-13]\n");
        return;
    }

    top = pim_min(13, top & 15);
    bottom = pim_min(13, bottom & 15);
    i32 playercolor = top * 16 + bottom;
    if (g_cmd_source == src_command)
    {
        Cvar_SetValue("_cl_color", (float)playercolor);
        if (g_cls.state == ca_connected)
        {
            Cmd_ForwardToServer();
        }
        return;
    }

    g_host_client->colors = playercolor;
    g_host_client->edict->v.team = (float)(bottom + 1);

    // send notification to all clients
    i32 clientIndex = (i32)(g_host_client - g_svs.clients);
    ASSERT(clientIndex >= 0);
    ASSERT(clientIndex < g_svs.maxclients);
    MSG_WriteByte(&g_sv.reliable_datagram, svc_updatecolors);
    MSG_WriteByte(&g_sv.reliable_datagram, clientIndex);
    MSG_WriteByte(&g_sv.reliable_datagram, g_host_client->colors);
}

/*
==================
Host_Kill_f
==================
*/
void Host_Kill_f(void)
{
    if (g_cmd_source == src_command)
    {
        Cmd_ForwardToServer();
        return;
    }

    ASSERT(g_sv_player);
    if (!g_sv_player)
    {
        return;
    }

    if (g_sv_player->v.health <= 0)
    {
        SV_ClientPrintf("Can't suicide -- allready dead!\n");
        return;
    }

    g_pr_global_struct->time = g_sv.time;
    g_pr_global_struct->self = EDICT_TO_PROG(g_sv_player);
    PR_ExecuteProgram(g_pr_global_struct->ClientKill);
}


/*
==================
Host_Pause_f
==================
*/
void Host_Pause_f(void)
{
    if (g_cmd_source == src_command)
    {
        Cmd_ForwardToServer();
        return;
    }
    if (cv_pausable.value == 0.0f)
    {
        SV_ClientPrintf("Pause not allowed.\n");
    }
    else
    {
        g_sv.paused ^= 1;

        const char* netname = &g_pr_strings[g_sv_player->v.netname];
        if (g_sv.paused)
        {
            SV_BroadcastPrintf("%s paused the game\n", netname);
        }
        else
        {
            SV_BroadcastPrintf("%s unpaused the game\n", netname);
        }

        // send notification to all clients
        MSG_WriteByte(&g_sv.reliable_datagram, svc_setpause);
        MSG_WriteByte(&g_sv.reliable_datagram, g_sv.paused);
    }
}

//===========================================================================


/*
==================
Host_PreSpawn_f
==================
*/
void Host_PreSpawn_f(void)
{
    if (g_cmd_source == src_command)
    {
        Con_Printf("prespawn is not valid from the console\n");
        return;
    }

    if (g_host_client->spawned)
    {
        Con_Printf("prespawn not valid -- allready spawned\n");
        return;
    }

    SZ_Write(&g_host_client->message, g_sv.signon.data, g_sv.signon.cursize);
    MSG_WriteByte(&g_host_client->message, svc_signonnum);
    MSG_WriteByte(&g_host_client->message, 2);
    g_host_client->sendsignon = true;
}

/*
==================
Host_Spawn_f
==================
*/
void Host_Spawn_f(void)
{
    if (g_cmd_source == src_command)
    {
        Con_Printf("spawn is not valid from the console\n");
        return;
    }

    if (g_host_client->spawned)
    {
        Con_Printf("Spawn not valid -- allready spawned\n");
        return;
    }

    // run the entrance script
    if (g_sv.loadgame)
    {
        // loaded games are fully inited allready
        // if this is the last client to be connected, unpause
        g_sv.paused = false;
    }
    else
    {
        // set up the edict
        edict_t* ent = g_host_client->edict;

        memset(&ent->v, 0, g_progs->entityfields * 4);
        ent->v.colormap = (float)NUM_FOR_EDICT(ent);
        ent->v.team = (float)(g_host_client->colors & 15) + 1.0f;
        ASSERT(false); // todo 64bit
        //ent->v.netname = g_host_client->name - g_pr_strings;

        // copy spawn parms out of the client_t
        float* parms = &g_pr_global_struct->parm1;
        for (i32 i = 0; i < NUM_SPAWN_PARMS; i++)
        {
            parms[i] = g_host_client->spawn_parms[i];
        }

        // call the spawn function
        g_pr_global_struct->time = g_sv.time;
        g_pr_global_struct->self = EDICT_TO_PROG(g_sv_player);
        PR_ExecuteProgram(g_pr_global_struct->ClientConnect);

        double now = Sys_FloatTime();
        double connectTime = g_host_client->netconnection->connecttime;
        double timeConnected = now - connectTime;
        if (timeConnected <= g_sv.time)
        {
            Sys_Printf("%s entered the game\n", g_host_client->name);
        }

        PR_ExecuteProgram(g_pr_global_struct->PutClientInServer);
    }

    // send all current names, colors, and frag counts
    sizebuf_t* msgbuf = &g_host_client->message;
    SZ_Clear(msgbuf);

    // send time of update
    MSG_WriteByte(msgbuf, svc_time);
    MSG_WriteFloat(msgbuf, (float)g_sv.time);

    for (i32 i = 0; i < g_svs.maxclients; i++)
    {
        client_t* client = &g_svs.clients[i];
        MSG_WriteByte(msgbuf, svc_updatename);
        MSG_WriteByte(msgbuf, i);
        MSG_WriteString(msgbuf, client->name);
        MSG_WriteByte(msgbuf, svc_updatefrags);
        MSG_WriteByte(msgbuf, i);
        MSG_WriteShort(msgbuf, client->old_frags);
        MSG_WriteByte(msgbuf, svc_updatecolors);
        MSG_WriteByte(msgbuf, i);
        MSG_WriteByte(msgbuf, client->colors);
    }

    // send all current light styles
    for (i32 i = 0; i < MAX_LIGHTSTYLES; i++)
    {
        MSG_WriteByte(msgbuf, svc_lightstyle);
        MSG_WriteByte(msgbuf, (char)i);
        MSG_WriteString(msgbuf, g_sv.lightstyles[i]);
    }

    //
    // send some stats
    //
    MSG_WriteByte(msgbuf, svc_updatestat);
    MSG_WriteByte(msgbuf, STAT_TOTALSECRETS);
    MSG_WriteLong(msgbuf, (i32)g_pr_global_struct->total_secrets);

    MSG_WriteByte(msgbuf, svc_updatestat);
    MSG_WriteByte(msgbuf, STAT_TOTALMONSTERS);
    MSG_WriteLong(msgbuf, (i32)g_pr_global_struct->total_monsters);

    MSG_WriteByte(msgbuf, svc_updatestat);
    MSG_WriteByte(msgbuf, STAT_SECRETS);
    MSG_WriteLong(msgbuf, (i32)g_pr_global_struct->found_secrets);

    MSG_WriteByte(msgbuf, svc_updatestat);
    MSG_WriteByte(msgbuf, STAT_MONSTERS);
    MSG_WriteLong(msgbuf, (i32)g_pr_global_struct->killed_monsters);


    // send a fixangle
    // Never send a roll angle, because savegames can catch the server
    // in a state where it is expecting the client to correct the angle
    // and it won't happen if the game was just loaded, so you wind up
    // with a permanent head tilt
    edict_t* ent = EDICT_NUM(1 + (g_host_client - g_svs.clients));
    MSG_WriteByte(msgbuf, svc_setangle);
    for (i32 i = 0; i < 2; i++)
    {
        MSG_WriteAngle(msgbuf, ent->v.angles[i]);
    }
    MSG_WriteAngle(msgbuf, 0);

    SV_WriteClientdataToMessage(g_sv_player, &g_host_client->message);

    MSG_WriteByte(msgbuf, svc_signonnum);
    MSG_WriteByte(msgbuf, 3);
    g_host_client->sendsignon = true;
}

/*
==================
Host_Begin_f
==================
*/
void Host_Begin_f(void)
{
    if (g_cmd_source == src_command)
    {
        Con_Printf("begin is not valid from the console\n");
        return;
    }

    g_host_client->spawned = true;
}

//===========================================================================


/*
==================
Host_Kick_f

Kicks a user off of the server
==================
*/
void Host_Kick_f(void)
{
    if (g_cmd_source == src_command)
    {
        if (!g_sv.active)
        {
            Cmd_ForwardToServer();
            return;
        }
    }
    else if (g_pr_global_struct->deathmatch && !g_host_client->privileged)
    {
        return;
    }

    client_t* self = g_host_client;
    client_t* target = NULL;
    bool byNumber = false;
    if (strcmp(Cmd_Argv(1), "#") == 0)
    {
        byNumber = true;
        i32 i = ParseInt(Cmd_Argv(2)) - 1;
        if (i < 0 || i >= g_svs.maxclients)
        {
            return;
        }
        target = &g_svs.clients[i];
        if (!target->active)
        {
            return;
        }
    }
    else
    {
        const char* name = Cmd_Argv(1);
        const i32 maxclients = g_svs.maxclients;
        for (i32 i = 0; i < maxclients; ++i)
        {
            client_t* client = &g_svs.clients[i];
            if (!client->active)
            {
                continue;
            }
            if (StrICmp(ARGS(client->name), name) == 0)
            {
                target = client;
                break;
            }
        }
    }

    const char* by = NULL;
    if (target)
    {
        by = self->name;
        if (g_cmd_source == src_command)
        {
            by = cv_cl_name.stringValue;
            if (g_cls.state == ca_dedicated)
            {
                by = "Console";
            }
        }

        // can't kick yourself!
        if (target != self)
        {
            const char* message = NULL;
            if (Cmd_Argc() > 2)
            {
                message = COM_Parse(Cmd_Args());
                if (message && message[0])
                {
                    if (byNumber)
                    {
                        message++; // skip the #
                        while (IsSpace(message[0]))
                        {
                            message++;
                        }
                        message += Q_strlen(Cmd_Argv(2)); // skip the number
                    }
                    while (IsSpace(message[0]))
                    {
                        message++;
                    }
                }
            }

            g_host_client = target;
            if (message)
            {
                SV_ClientPrintf("Kicked by %s: %s\n", by, message);
            }
            else
            {
                SV_ClientPrintf("Kicked by %s\n", by);
            }
            SV_DropClient(false);
            g_host_client = self;
        }
    }
}

/*
===============================================================================

DEBUGGING TOOLS

===============================================================================
*/

static void Float_Or(float* x, i32 bit)
{
    i32 y = (i32)(*x);
    y |= bit;
    *x = (float)y;
}

/*
==================
Host_Give_f
==================
*/
void Host_Give_f(void)
{
    if (g_cmd_source == src_command)
    {
        Cmd_ForwardToServer();
        return;
    }

    if (g_pr_global_struct->deathmatch && !g_host_client->privileged)
    {
        return;
    }

    const char* t = Cmd_Argv(1);
    const i32 v = ParseInt(Cmd_Argv(2));

    eval_t *val = NULL;
    switch (t[0])
    {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        // MED 01/04/97 added hipnotic give stuff
        if (g_hipnotic)
        {
            if (t[0] == '6')
            {
                if (t[1] == 'a')
                    Float_Or(&g_sv_player->v.items, HIT_PROXIMITY_GUN);
                else
                    Float_Or(&g_sv_player->v.items, IT_GRENADE_LAUNCHER);
            }
            else if (t[0] == '9')
                g_sv_player->v.items = (i32)g_sv_player->v.items | HIT_LASER_CANNON;
            else if (t[0] == '0')
                g_sv_player->v.items = (i32)g_sv_player->v.items | HIT_MJOLNIR;
            else if (t[0] >= '2')
                g_sv_player->v.items = (i32)g_sv_player->v.items | (IT_SHOTGUN << (t[0] - '2'));
        }
        else
        {
            if (t[0] >= '2')
                g_sv_player->v.items = (i32)g_sv_player->v.items | (IT_SHOTGUN << (t[0] - '2'));
        }
        break;

    case 's':
        if (g_rogue)
        {
            val = GetEdictFieldValue(g_sv_player, "ammo_shells1");
            if (val)
                val->_float = v;
        }

        g_sv_player->v.ammo_shells = v;
        break;
    case 'n':
        if (g_rogue)
        {
            val = GetEdictFieldValue(g_sv_player, "ammo_nails1");
            if (val)
            {
                val->_float = v;
                if (g_sv_player->v.weapon <= IT_LIGHTNING)
                    g_sv_player->v.ammo_nails = v;
            }
        }
        else
        {
            g_sv_player->v.ammo_nails = v;
        }
        break;
    case 'l':
        if (g_rogue)
        {
            val = GetEdictFieldValue(g_sv_player, "ammo_lava_nails");
            if (val)
            {
                val->_float = v;
                if (g_sv_player->v.weapon > IT_LIGHTNING)
                    g_sv_player->v.ammo_nails = v;
            }
        }
        break;
    case 'r':
        if (g_rogue)
        {
            val = GetEdictFieldValue(g_sv_player, "ammo_rockets1");
            if (val)
            {
                val->_float = v;
                if (g_sv_player->v.weapon <= IT_LIGHTNING)
                    g_sv_player->v.ammo_rockets = v;
            }
        }
        else
        {
            g_sv_player->v.ammo_rockets = v;
        }
        break;
    case 'm':
        if (g_rogue)
        {
            val = GetEdictFieldValue(g_sv_player, "ammo_multi_rockets");
            if (val)
            {
                val->_float = v;
                if (g_sv_player->v.weapon > IT_LIGHTNING)
                    g_sv_player->v.ammo_rockets = v;
            }
        }
        break;
    case 'h':
        g_sv_player->v.health = v;
        break;
    case 'c':
        if (g_rogue)
        {
            val = GetEdictFieldValue(g_sv_player, "ammo_cells1");
            if (val)
            {
                val->_float = v;
                if (g_sv_player->v.weapon <= IT_LIGHTNING)
                    g_sv_player->v.ammo_cells = v;
            }
        }
        else
        {
            g_sv_player->v.ammo_cells = v;
        }
        break;
    case 'p':
        if (g_rogue)
        {
            val = GetEdictFieldValue(g_sv_player, "ammo_plasma");
            if (val)
            {
                val->_float = v;
                if (g_sv_player->v.weapon > IT_LIGHTNING)
                    g_sv_player->v.ammo_cells = v;
            }
        }
        break;
    }
}

static edict_t *FindViewthing(void)
{
    const i32 num_edicts = g_sv.num_edicts;
    for (i32 i = 0; i < num_edicts; i++)
    {
        edict_t* e = EDICT_NUM(i);
        i32 classname = e->v.classname;
        ASSERT(classname >= 0);
        const char* str = &g_pr_strings[classname];
        if (strcmp(str, "viewthing") == 0)
        {
            return e;
        }
    }
    Con_Printf("No viewthing on map\n");
    return NULL;
}

/*
==================
Host_Viewmodel_f
==================
*/
void Host_Viewmodel_f(void)
{
    edict_t* e = FindViewthing();
    if (!e)
        return;

    e->v.frame = 0;

    model_t* m = Mod_ForName(Cmd_Argv(1), false);
    if (!m)
    {
        Con_Printf("Can't load %s\n", Cmd_Argv(1));
        return;
    }

    i32 modelIndex = (i32)e->v.modelindex;
    if ((modelIndex >= 0) &&
        (modelIndex < NELEM(g_cl.model_precache)))
    {
        g_cl.model_precache[modelIndex] = m;
    }
    else
    {
        ASSERT(false);
    }
}

/*
==================
Host_Viewframe_f
==================
*/
void Host_Viewframe_f(void)
{
    edict_t* e = FindViewthing();
    if (!e)
        return;

    i32 modelIndex = (i32)e->v.modelindex;
    if ((modelIndex >= 0) &&
        (modelIndex < NELEM(g_cl.model_precache)))
    {
        const model_t* m = g_cl.model_precache[modelIndex];
        ASSERT(m);
        if (!m)
            return;

        i32 f = ParseInt(Cmd_Argv(1));
        f = pim_max(f, 0);
        f = pim_min(f, m->numframes);
        e->v.frame = (float)f;
    }
    else
    {
        ASSERT(false);
    }
}

static void PrintFrameName(model_t *m, int frame)
{
    aliashdr_t    *hdr;
    maliasframedesc_t *pframedesc;

    hdr = (aliashdr_t *)Mod_Extradata(m);
    if (!hdr)
        return;
    pframedesc = &hdr->frames[frame];

    Con_Printf("frame %i: %s\n", frame, pframedesc->name);
}

/*
==================
Host_Viewnext_f
==================
*/
void Host_Viewnext_f(void)
{
    edict_t *e;
    model_t *m;

    e = FindViewthing();
    if (!e)
        return;
    m = g_cl.model_precache[(int)e->v.modelindex];

    e->v.frame = e->v.frame + 1;
    if (e->v.frame >= m->numframes)
        e->v.frame = m->numframes - 1;

    PrintFrameName(m, e->v.frame);
}

/*
==================
Host_Viewprev_f
==================
*/
void Host_Viewprev_f(void)
{
    edict_t *e;
    model_t *m;

    e = FindViewthing();
    if (!e)
        return;

    m = g_cl.model_precache[(int)e->v.modelindex];

    e->v.frame = e->v.frame - 1;
    if (e->v.frame < 0)
        e->v.frame = 0;

    PrintFrameName(m, e->v.frame);
}

/*
===============================================================================

DEMO LOOP CONTROL

===============================================================================
*/


/*
==================
Host_Startdemos_f
==================
*/
void Host_Startdemos_f(void)
{
    if (g_cls.state == ca_dedicated)
    {
        if (!g_sv.active)
            Cbuf_AddText("map start\n");
        return;
    }

    i32 c = Cmd_Argc() - 1;
    if (c > MAX_DEMOS)
    {
        Con_Printf("Max %d demos in demoloop\n", MAX_DEMOS);
        c = MAX_DEMOS;
    }
    Con_Printf("%d demo(s) in loop\n", c);

    for (i32 i = 1; i < c + 1; i++)
    {
        StrCpy(ARGS(g_cls.demos[i - 1]), Cmd_Argv(i));
    }

    if (!g_sv.active && (g_cls.demonum != -1) && !g_cls.demoplayback)
    {
        g_cls.demonum = 0;
        CL_NextDemo();
    }
    else
    {
        g_cls.demonum = -1;
    }
}


/*
==================
Host_Demos_f

Return to looping demos
==================
*/
void Host_Demos_f(void)
{
    if (g_cls.state == ca_dedicated)
        return;
    if (g_cls.demonum == -1)
    {
        g_cls.demonum = 1;
    }
    CL_Disconnect_f();
    CL_NextDemo();
}

/*
==================
Host_Stopdemo_f

Return to looping demos
==================
*/
void Host_Stopdemo_f(void)
{
    if (g_cls.state == ca_dedicated)
        return;
    if (!g_cls.demoplayback)
        return;
    CL_StopPlayback();
    CL_Disconnect();
}

#endif // QUAKE_IMPL
