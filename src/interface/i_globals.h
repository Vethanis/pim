#pragma once

#include "interface/i_types.h"

//=====================================
// cvars

// client
extern cvar_t cl_name;
extern cvar_t cl_color;
extern cvar_t cl_upspeed;
extern cvar_t cl_forwardspeed;
extern cvar_t cl_backspeed;
extern cvar_t cl_sidespeed;
extern cvar_t cl_movespeedkey;
extern cvar_t cl_yawspeed;
extern cvar_t cl_pitchspeed;
extern cvar_t cl_anglespeedkey;
extern cvar_t cl_autofire;
extern cvar_t cl_shownet;
extern cvar_t cl_nolerp;
extern cvar_t cl_pitchdriftspeed;
extern cvar_t lookspring;
extern cvar_t lookstrafe;
extern cvar_t sensitivity;
extern cvar_t m_pitch;
extern cvar_t m_yaw;
extern cvar_t m_forward;
extern cvar_t m_side;
// common
extern cvar_t registered;
// screen
extern cvar_t scr_viewsize;
// net
extern cvar_t hostname;
// sound
extern cvar_t loadas8bit;
extern cvar_t bgmvolume;
extern cvar_t volume;

//=====================================
// common

extern char com_gamedir[1024];
extern char com_token[1024];
extern char g_vabuf[1024];
extern qboolean com_eof;
extern i32 com_argc;
extern char **com_argv;
extern i32 com_filesize;
extern qboolean standard_quake;
extern qboolean rogue;
extern qboolean hipnotic;

//=====================================
// mathlib

extern vec3_t vec3_origin;

//=====================================
// wad

extern i32 wad_numlumps;
extern lumpinfo_t *wad_lumps;
extern u8 *wad_base;

//=====================================
// draw

extern qpic_t *draw_disc; // also used on sbar

//=====================================
// cvar

extern cvar_t *cvar_vars;

//=====================================
// client

extern client_static_t cls;
extern client_state_t cl;

// FIXME: allocate dynamically
extern efrag_t cl_efrags[MAX_EFRAGS];
extern entity_t cl_entities[MAX_EDICTS];
extern entity_t cl_static_entities[MAX_STATIC_ENTITIES];
extern lightstyle_t cl_lightstyle[MAX_LIGHTSTYLES];
extern dlight_t cl_dlights[MAX_DLIGHTS];
extern entity_t cl_temp_entities[MAX_TEMP_ENTITIES];
extern beam_t cl_beams[MAX_BEAMS];

extern i32 cl_numvisedicts;
extern entity_t *cl_visedicts[MAX_VISEDICTS];

//=====================================
// input

extern kbutton_t in_mlook;
extern kbutton_t in_klook;
extern kbutton_t in_strafe;
extern kbutton_t in_speed;

//=====================================
// vid

extern void(*vid_menudrawfn)(void);
extern void(*vid_menukeyfn)(i32 key);

// global video state
extern viddef_t vid;
extern u16 d_8to16table[256];
extern u32 d_8to24table[256];

//=====================================
// screen

extern float scr_con_current;
extern float scr_conlines;  // lines of console to display
extern i32 scr_fullupdate; // set to 0 to force full redraw
extern i32 sb_lines;
extern i32 clearnotify; // set to 0 whenever notify text is drawn
// only the refresh window will be updated unless these variables are flagged 
extern i32 scr_copytop;
extern i32 scr_copyeverything;
extern qboolean scr_disabled_for_loading;
extern qboolean scr_skipupdate;
extern qboolean block_drawing;

//=====================================
// net

extern qsocket_t *net_activeSockets;
extern qsocket_t *net_freeSockets;
extern i32 net_numsockets;

extern i32 net_numlandrivers;
extern net_landriver_t net_landrivers[MAX_NET_DRIVERS];

extern i32 net_numdrivers;
extern net_driver_t net_drivers[MAX_NET_DRIVERS];

extern i32 DEFAULTnet_hostport;
extern i32 net_hostport;
extern i32 net_driverlevel;

extern i32 messagesSent;
extern i32 messagesReceived;
extern i32 unreliableMessagesSent;
extern i32 unreliableMessagesReceived;

extern i32 hostCacheCount;
extern hostcache_t hostcache[HOSTCACHESIZE];

extern double net_time;
extern sizebuf_t net_message;
extern i32 net_activeconnections;

extern char my_tcpip_address[NET_NAMELEN];

extern qboolean tcpipAvailable;
extern qboolean slistInProgress;
extern qboolean slistSilent;
extern qboolean slistLocal;

//=====================================
// cmd

extern cmd_source_t cmd_source;

//=====================================
// sbar

extern i32 sb_lines; // scan lines to draw

//=====================================
// sound

// 0 to MAX_DYNAMIC_CHANNELS-1 = normal entity sounds
// MAX_DYNAMIC_CHANNELS to MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS - 1 = water, etc
// MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS to total_channels = static sounds
extern i32 total_channels;
extern channel_t channels[MAX_CHANNELS];

// Fake dma is a synchronous faking of the DMA progress used for
// isolating performance in the renderer.  The fakedma_updates is
// number of times S_Update() is called per second.
extern qboolean fakedma;
extern i32 fakedma_updates;

extern volatile dma_t* shm;
extern volatile dma_t sn;

extern i32 paintedtime;
extern i32 snd_blocked;
extern qboolean snd_initialized;

extern vec3_t listener_origin;
extern vec3_t listener_forward;
extern vec3_t listener_right;
extern vec3_t listener_up;
extern vec_t sound_nominal_clip_dist;

//=====================================
// render

extern refdef_t r_refdef;
extern vec3_t r_origin, vpn, vright, vup;
extern texture_t* r_notexture_mip;
// if 1, surface cache is currently empty and
extern i32 reinit_surfcache;
// set if thrashing the surface cache
extern qboolean r_cache_thrash;

//=====================================
// progs

const i32 type_size[ev_COUNT] =
{
    sizeof(i32) / 4,        // ev_void
    sizeof(string_t) / 4,   // ev_string
    sizeof(float) / 4,      // ev_float
    sizeof(vec3_t) / 4,     // ev_vector
    sizeof(i32) / 4,        // ev_entity
    sizeof(i32) / 4,        // ev_field
    sizeof(i32) / 4,        // ev_function
    sizeof(isize) / 4,      // ev_pointer
};

extern dprograms_t *progs;
extern dfunction_t *pr_functions;
extern char *pr_strings;
extern ddef_t *pr_globaldefs;
extern ddef_t *pr_fielddefs;
extern dstatement_t *pr_statements;
extern globalvars_t *pr_global_struct;
extern float *pr_globals; // same as pr_global_struct
extern i32 pr_edict_size; // in bytes

typedef void(*builtin_t) (void);
extern builtin_t* pr_builtins;
extern i32 pr_numbuiltins;
extern i32 pr_argc;
extern qboolean pr_trace;
extern dfunction_t* pr_xfunction;
extern i32 pr_xstatement;
extern u16 pr_crc;

//=====================================
// 
