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

//=============================================================================
// globals
// g_ prepended to avoid namespace pollution
//=============================================================================

//=====================================
// common

extern char g_com_cmdline[PIM_PATH];
extern char g_com_gamedir[PIM_PATH];
extern char g_com_token[1024];
extern i32 g_com_argc;
extern const char **g_com_argv;
extern qboolean g_standard_quake;
extern qboolean g_rogue;
extern qboolean g_hipnotic;
extern qboolean g_proghack;
extern qboolean g_msg_badread;
extern i32 g_msg_readcount;

//=====================================
// mathlib

extern vec3_t g_vec3_origin;

//=====================================
// wad

extern i32 g_wad_numlumps;
extern lumpinfo_t *g_wad_lumps;
extern u8 *g_wad_base;

//=====================================
// draw

// also used on sbar
extern qpic_t *g_draw_disc;

//=====================================
// cvar

extern cvar_t *g_cvar_vars;

//=====================================
// client

extern client_static_t g_cls;
extern client_state_t g_cl;

// FIXME: allocate dynamically
extern efrag_t g_cl_efrags[MAX_EFRAGS];
extern entity_t g_cl_entities[MAX_EDICTS];
extern entity_t g_cl_static_entities[MAX_STATIC_ENTITIES];
extern lightstyle_t g_cl_lightstyle[MAX_LIGHTSTYLES];
extern dlight_t g_cl_dlights[MAX_DLIGHTS];
extern entity_t g_cl_temp_entities[MAX_TEMP_ENTITIES];
extern beam_t g_cl_beams[MAX_BEAMS];

extern i32 g_cl_numvisedicts;
extern entity_t *g_cl_visedicts[MAX_VISEDICTS];

//=====================================
// input

extern kbutton_t g_in_mlook;
extern kbutton_t g_in_klook;
extern kbutton_t g_in_strafe;
extern kbutton_t g_in_speed;

//=====================================
// vid

// global video state
extern viddef_t g_vid;
extern u16 g_d_8to16table[256];
extern u32 g_d_8to24table[256];

//=====================================
// screen

extern float g_scr_con_current;
extern float g_scr_conlines;  // lines of console to display
extern i32 g_scr_fullupdate; // set to 0 to force full redraw
extern i32 g_sb_lines;
extern i32 g_clearnotify; // set to 0 whenever notify text is drawn
// only the refresh window will be updated unless these variables are flagged 
extern i32 g_scr_copytop;
extern i32 g_scr_copyeverything;
extern qboolean g_scr_disabled_for_loading;
extern qboolean g_scr_skipupdate;
extern qboolean g_block_drawing;

//=====================================
// net

extern qsocket_t *g_net_activeSockets;
extern qsocket_t *g_net_freeSockets;
extern i32 g_net_numsockets;

extern i32 g_net_numlandrivers;
extern net_landriver_t g_net_landrivers[MAX_NET_DRIVERS];

extern i32 g_net_numdrivers;
extern net_driver_t g_net_drivers[MAX_NET_DRIVERS];

extern i32 g_DEFAULTnet_hostport;
extern i32 g_net_hostport;
extern i32 g_net_driverlevel;

extern i32 g_messagesSent;
extern i32 g_messagesReceived;
extern i32 g_unreliableMessagesSent;
extern i32 g_unreliableMessagesReceived;

extern i32 g_hostCacheCount;
extern hostcache_t g_hostcache[HOSTCACHESIZE];

extern double g_net_time;
extern sizebuf_t g_net_message;
extern i32 g_net_activeconnections;

extern char g_my_tcpip_address[NET_NAMELEN];

extern qboolean g_tcpipAvailable;
extern qboolean g_slistInProgress;
extern qboolean g_slistSilent;
extern qboolean g_slistLocal;

//=====================================
// cmd

extern cmd_source_t g_cmd_source;

//=====================================
// sbar

extern i32 g_sb_lines; // scan lines to draw

//=====================================
// sound

// 0 to MAX_DYNAMIC_CHANNELS-1 = normal entity sounds
// MAX_DYNAMIC_CHANNELS to MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS - 1 = water, etc
// MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS to total_channels = static sounds
extern i32 g_total_channels;
extern channel_t g_channels[MAX_CHANNELS];

// Fake dma is a synchronous faking of the DMA progress used for
// isolating performance in the renderer.  The fakedma_updates is
// number of times S_Update() is called per second.
extern qboolean g_fakedma;
extern i32 g_fakedma_updates;

extern volatile dma_t* g_shm;
extern volatile dma_t g_sn;

extern i32 g_paintedtime;
extern i32 g_snd_blocked;
extern qboolean g_snd_initialized;

extern vec3_t g_listener_origin;
extern vec3_t g_listener_forward;
extern vec3_t g_listener_right;
extern vec3_t g_listener_up;
extern vec_t g_sound_nominal_clip_dist;

//=====================================
// render

// if 1, surface cache is currently empty and
extern i32 g_reinit_surfcache;

//=====================================
// progs

extern const i32 g_type_size[ev_COUNT];

extern dprograms_t *g_progs;
extern dfunction_t *g_pr_functions;
extern char *g_pr_strings;
extern ddef_t *g_pr_globaldefs;
extern ddef_t *g_pr_fielddefs;
extern dstatement_t *g_pr_statements;
extern globalvars_t *g_pr_global_struct;
extern float *g_pr_globals; // same as pr_global_struct
extern i32 g_pr_edict_size; // in bytes

typedef void(*builtin_t) (void);
extern builtin_t* g_pr_builtins;
extern i32 g_pr_numbuiltins;
extern i32 g_pr_argc;
extern qboolean g_pr_trace;
extern dfunction_t* g_pr_xfunction;
extern i32 g_pr_xstatement;
extern u16 g_pr_crc;

//=====================================
// server

// persistent server info
extern server_static_t g_svs;
// local server
extern server_t g_sv;
extern client_t *g_host_client;
extern double g_host_time;
extern edict_t *g_sv_player;

//=====================================
// model

extern aliashdr_t *g_pheader;
extern stvert_t g_stverts[MAXALIASVERTS];
extern mtriangle_t g_triangles[MAXALIASTRIS];
extern trivertx_t *g_poseverts[MAXALIASFRAMES];

//=====================================
// keys

extern keydest_t g_key_dest;
extern char* g_keybindings[256];
extern i32 g_key_repeats[256];
// incremented every key event
extern i32 g_key_count;
extern i32 g_key_lastpress;

//=====================================
// console

extern i32 g_con_totallines;
extern i32 g_con_backscroll;
// because no entities to refresh
extern qboolean g_con_forcedup;
extern qboolean g_con_initialized;
extern u8* g_con_chars;
// scan lines to clear for notify lines
extern i32 g_con_notifylines;

//=====================================
// view

// palette is sent through this
extern u8 g_gammatable[256];
extern u8 g_ramps[3][256];
extern float g_v_blend[4];

//=====================================
// menu

extern i32 g_m_activenet;

//=====================================
// glquake

extern i32 g_texture_extension_number;
extern i32 g_texture_mode;
extern float g_gldepthmin, g_gldepthmax;
extern glvert_t g_glv;
extern i32 g_glx, g_gly, g_glwidth, g_glheight;
extern entity_t g_r_worldentity;
extern qboolean g_r_cache_thrash; // compatability
extern vec3_t g_modelorg, g_r_entorigin;
extern entity_t *g_currententity;
extern i32 g_r_visframecount; // ??? what difs?
extern i32 g_r_framecount;
extern mplane_t g_frustum[4];
extern i32 g_c_brush_polys, g_c_alias_polys;
// view origin
extern vec3_t g_vup;
extern vec3_t g_vpn;
extern vec3_t g_vright;
extern vec3_t g_r_origin;
// screen size info
extern refdef_t g_r_refdef;
extern mleaf_t *g_r_viewleaf, *g_r_oldviewleaf;
extern texture_t *g_r_notexture_mip;
// 8.8 fraction of base light value
extern i32 g_d_lightstylevalue[256];
extern qboolean g_envmap;
extern i32 g_currenttexture;
extern i32 g_cnttextures[2];
extern i32 g_particletexture;
extern i32 g_playertextures;
// index in cl.loadmodel, not gl texture object
extern i32 g_skytexturenum;
extern i32 g_gl_lightmap_format;
extern i32 g_gl_solid_format;
extern i32 g_gl_alpha_format;
extern i32 g_mirrortexturenum; // quake texturenum, not gltexturenum
extern qboolean g_mirror;
extern mplane_t* g_mirror_plane;
extern float g_r_world_matrix[16];
extern const char* g_gl_vendor;
extern const char* g_gl_renderer;
extern const char* g_gl_version;
extern const char* g_gl_extensions;
extern qboolean g_gl_mtexable;

//=====================================
// host

extern cvar_t g_sys_ticrate;
extern cvar_t g_sys_nostdout;
extern cvar_t g_developer;
extern cvar_t g_chase_active;
extern qboolean g_noclip_anglehack;
// true if into command execution
extern qboolean g_host_initialized;
extern qboolean g_isDedicated;
// suppresses resolution and cache size console output
extern qboolean g_msg_suppress_1;
extern quakeparms_t g_host_parms;
extern u8* g_host_basepal;
extern u8* g_host_colormap;
// incremented every frame, never reset
extern i32 g_host_framecount;
extern double g_host_frametime;
// not bounded in any way, changed at start of every frame, never reset
extern double g_realtime;
// skill level for currently loaded level (in case the user changes the cvar while the level is running, this reflects the level actually in use)
extern i32 g_current_skill;
extern i32 g_minimum_memory;

//=====================================
