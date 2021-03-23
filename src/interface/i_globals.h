#pragma once

#include "interface/i_types.h"
#include <setjmp.h>

//=============================================================================
// cvars
// cv_ prepended to avoid namespace pollution
//=============================================================================

extern cvar_t cv_chase_back;// = { "chase_back", "100" };
extern cvar_t cv_chase_up;// = { "chase_up", "16" };
extern cvar_t cv_chase_right;// = { "chase_right", "0" };
extern cvar_t cv_chase_active;// = { "chase_active", "0" };
extern cvar_t cv_cl_upspeed;// = { "cl_upspeed","200" };
extern cvar_t cv_cl_forwardspeed;// = { "cl_forwardspeed","200", true };
extern cvar_t cv_cl_backspeed;// = { "cl_backspeed","200", true };
extern cvar_t cv_cl_sidespeed;// = { "cl_sidespeed","350" };
extern cvar_t cv_cl_movespeedkey;// = { "cl_movespeedkey","2.0" };
extern cvar_t cv_cl_yawspeed;// = { "cl_yawspeed","140" };
extern cvar_t cv_cl_pitchspeed;// = { "cl_pitchspeed","150" };
extern cvar_t cv_cl_anglespeedkey;// = { "cl_anglespeedkey","1.5" };
extern cvar_t cv_cl_name;// = { "_cl_name", "player", true };
extern cvar_t cv_cl_color;// = { "_cl_color", "0", true };
extern cvar_t cv_cl_shownet;// = { "cl_shownet","0" }; // can be 0, 1, or 2
extern cvar_t cv_cl_nolerp;// = { "cl_nolerp","0" };
extern cvar_t cv_lookspring;// = { "lookspring","0", true };
extern cvar_t cv_lookstrafe;// = { "lookstrafe","0", true };
extern cvar_t cv_sensitivity;// = { "sensitivity","3", true };
extern cvar_t cv_m_pitch;// = { "m_pitch","0.022", true };
extern cvar_t cv_m_yaw;// = { "m_yaw","0.022", true };
extern cvar_t cv_m_forward;// = { "m_forward","1", true };
extern cvar_t cv_m_side;// = { "m_side","0.8", true };
extern cvar_t cv_registered;// = { "registered","0" };
extern cvar_t cv_cmdline;// = { "cmdline","0", false, true };
extern cvar_t cv_con_notifytime;// = { "con_notifytime","3" }; //seconds
extern cvar_t cv_d_subdiv16;// = { "d_subdiv16", "1" };
extern cvar_t cv_d_mipcap;// = { "d_mipcap", "0" };
extern cvar_t cv_d_mipscale;// = { "d_mipscale", "1" };
extern cvar_t cv_gl_nobind;// = { "gl_nobind", "0" };
extern cvar_t cv_gl_max_size;// = { "gl_max_size", "1024" };
extern cvar_t cv_gl_picmip;// = { "gl_picmip", "0" };
extern cvar_t cv_gl_subdivide_size;// = { "gl_subdivide_size", "128", true };
extern cvar_t cv_r_norefresh;// = { "r_norefresh","0" };
extern cvar_t cv_r_speeds;// = { "r_speeds","0" };
extern cvar_t cv_r_lightmap;// = { "r_lightmap","0" };
extern cvar_t cv_r_shadows;// = { "r_shadows","0" };
extern cvar_t cv_r_mirroralpha;// = { "r_mirroralpha","1" };
extern cvar_t cv_r_wateralpha;// = { "r_wateralpha","1" };
extern cvar_t cv_r_dynamic;// = { "r_dynamic","1" };
extern cvar_t cv_r_novis;// = { "r_novis","0" };
extern cvar_t cv_gl_finish;// = { "gl_finish","0" };
extern cvar_t cv_gl_clear;// = { "gl_clear","0" };
extern cvar_t cv_gl_cull;// = { "gl_cull","1" };
extern cvar_t cv_gl_texsort;// = { "gl_texsort","1" };
extern cvar_t cv_gl_smoothmodels;// = { "gl_smoothmodels","1" };
extern cvar_t cv_gl_affinemodels;// = { "gl_affinemodels","0" };
extern cvar_t cv_gl_polyblend;// = { "gl_polyblend","1" };
extern cvar_t cv_gl_flashblend;// = { "gl_flashblend","1" };
extern cvar_t cv_gl_playermip;// = { "gl_playermip","0" };
extern cvar_t cv_gl_nocolors;// = { "gl_nocolors","0" };
extern cvar_t cv_gl_keeptjunctions;// = { "gl_keeptjunctions","0" };
extern cvar_t cv_gl_reporttjunctions;// = { "gl_reporttjunctions","0" };
extern cvar_t cv_gl_doubleeyes;// = { "gl_doubleeys", "1" };
extern cvar_t cv_gl_triplebuffer;// = { "gl_triplebuffer", "1", true };
extern cvar_t cv_vid_redrawfull;// = { "vid_redrawfull","0",false };
extern cvar_t cv_vid_waitforrefresh;// = { "vid_waitforrefresh","0",true };
extern cvar_t cv_mouse_button_commands[3];// = { {"mouse1","+attack"}, {"mouse2","+strafe"}, {"mouse3","+forward"}, };
extern cvar_t cv_gl_ztrick;// = { "gl_ztrick","1" };
extern cvar_t cv_host_framerate;// = { "host_framerate","0" }; // set for slow motion
extern cvar_t cv_host_speeds;// = { "host_speeds","0" };   // set for running times
extern cvar_t cv_sys_ticrate;// = { "sys_ticrate","0.05" };
extern cvar_t cv_serverprofile;// = { "serverprofile","0" };
extern cvar_t cv_fraglimit;// = { "fraglimit","0",false,true };
extern cvar_t cv_timelimit;// = { "timelimit","0",false,true };
extern cvar_t cv_teamplay;// = { "teamplay","0",false,true };
extern cvar_t cv_samelevel;// = { "samelevel","0" };
extern cvar_t cv_noexit;// = { "noexit","0",false,true };
extern cvar_t cv_developer;// = { "developer","1" };
extern cvar_t cv_skill;// = { "skill","1" }; // 0 - 3
extern cvar_t cv_deathmatch;// = { "deathmatch","0" }; // 0, 1, or 2
extern cvar_t cv_coop;// = { "coop","0" }; // 0 or 1
extern cvar_t cv_pausable;// = { "pausable","1" };
extern cvar_t cv_temp1;// = { "temp1","0" };
extern cvar_t cv_m_filter;// = { "m_filter","0" };
extern cvar_t cv_in_joystick;// = { "joystick","0", true };
extern cvar_t cv_joy_name;// = { "joyname", "joystick" };
extern cvar_t cv_joy_advanced;// = { "joyadvanced", "0" };
extern cvar_t cv_joy_advaxisx;// = { "joyadvaxisx", "0" };
extern cvar_t cv_joy_advaxisy;// = { "joyadvaxisy", "0" };
extern cvar_t cv_joy_advaxisz;// = { "joyadvaxisz", "0" };
extern cvar_t cv_joy_advaxisr;// = { "joyadvaxisr", "0" };
extern cvar_t cv_joy_advaxisu;// = { "joyadvaxisu", "0" };
extern cvar_t cv_joy_advaxisv;// = { "joyadvaxisv", "0" };
extern cvar_t cv_joy_forwardthreshold;// = { "joyforwardthreshold", "0.15" };
extern cvar_t cv_joy_sidethreshold;// = { "joysidethreshold", "0.15" };
extern cvar_t cv_joy_pitchthreshold;// = { "joypitchthreshold", "0.15" };
extern cvar_t cv_joy_yawthreshold;// = { "joyyawthreshold", "0.15" };
extern cvar_t cv_joy_forwardsensitivity;// = { "joyforwardsensitivity", "-1.0" };
extern cvar_t cv_joy_sidesensitivity;// = { "joysidesensitivity", "-1.0" };
extern cvar_t cv_joy_pitchsensitivity;// = { "joypitchsensitivity", "1.0" };
extern cvar_t cv_joy_yawsensitivity;// = { "joyyawsensitivity", "-1.0" };
extern cvar_t cv_joy_wwhack1;// = { "joywwhack1", "0.0" };
extern cvar_t cv_joy_wwhack2;// = { "joywwhack2", "0.0" };
extern cvar_t cv_net_messagetimeout;// = { "net_messagetimeout","300" };
extern cvar_t cv_hostname;// = { "hostname", "UNNAMED" };
extern cvar_t cv_config_com_port;// = { "_config_com_port", "0x3f8", true };
extern cvar_t cv_config_com_irq;// = { "_config_com_irq", "4", true };
extern cvar_t cv_config_com_baud;// = { "_config_com_baud", "57600", true };
extern cvar_t cv_config_com_modem;// = { "_config_com_modem", "1", true };
extern cvar_t cv_config_modem_dialtype;// = { "_config_modem_dialtype", "T", true };
extern cvar_t cv_config_modem_clear;// = { "_config_modem_clear", "ATZ", true };
extern cvar_t cv_config_modem_init;// = { "_config_modem_init", "", true };
extern cvar_t cv_config_modem_hangup;// = { "_config_modem_hangup", "AT H", true };
extern cvar_t cv_sv_aim;// = { "sv_aim", "0.93" };
extern cvar_t cv_nomonsters;// = { "nomonsters", "0" };
extern cvar_t cv_gamecfg;// = { "gamecfg", "0" };
extern cvar_t cv_scratch1;// = { "scratch1", "0" };
extern cvar_t cv_scratch2;// = { "scratch2", "0" };
extern cvar_t cv_scratch3;// = { "scratch3", "0" };
extern cvar_t cv_scratch4;// = { "scratch4", "0" };
extern cvar_t cv_savedgamecfg;// = { "savedgamecfg", "0", true };
extern cvar_t cv_saved1;// = { "saved1", "0", true };
extern cvar_t cv_saved2;// = { "saved2", "0", true };
extern cvar_t cv_saved3;// = { "saved3", "0", true };
extern cvar_t cv_saved4;// = { "saved4", "0", true };
extern cvar_t cv_r_draworder;// = { "r_draworder","0" };
extern cvar_t cv_r_timegraph;// = { "r_timegraph","0" };
extern cvar_t cv_r_graphheight;// = { "r_graphheight","10" };
extern cvar_t cv_r_clearcolor;// = { "r_clearcolor","2" };
extern cvar_t cv_r_waterwarp;// = { "r_waterwarp","1" };
extern cvar_t cv_r_fullbright;// = { "r_fullbright","0" };
extern cvar_t cv_r_drawentities;// = { "r_drawentities","1" };
extern cvar_t cv_r_drawviewmodel;// = { "r_drawviewmodel","1" };
extern cvar_t cv_r_aliasstats;// = { "r_polymodelstats","0" };
extern cvar_t cv_r_dspeeds;// = { "r_dspeeds","0" };
extern cvar_t cv_r_drawflat;// = { "r_drawflat", "0" };
extern cvar_t cv_r_ambient;// = { "r_ambient", "0" };
extern cvar_t cv_r_reportsurfout;// = { "r_reportsurfout", "0" };
extern cvar_t cv_r_maxsurfs;// = { "r_maxsurfs", "0" };
extern cvar_t cv_r_numsurfs;// = { "r_numsurfs", "0" };
extern cvar_t cv_r_reportedgeout;// = { "r_reportedgeout", "0" };
extern cvar_t cv_r_maxedges;// = { "r_maxedges", "0" };
extern cvar_t cv_r_numedges;// = { "r_numedges", "0" };
extern cvar_t cv_r_aliastransbase;// = { "r_aliastransbase", "200" };
extern cvar_t cv_r_aliastransadj;// = { "r_aliastransadj", "100" };
extern cvar_t cv_scr_viewsize;// = { "viewsize","100", true };
extern cvar_t cv_scr_fov;// = { "fov","90" }; // 10 - 170
extern cvar_t cv_scr_conspeed;// = { "scr_conspeed","300" };
extern cvar_t cv_scr_centertime;// = { "scr_centertime","2" };
extern cvar_t cv_scr_showram;// = { "showram","1" };
extern cvar_t cv_scr_showturtle;// = { "showturtle","0" };
extern cvar_t cv_scr_showpause;// = { "showpause","1" };
extern cvar_t cv_scr_printspeed;// = { "scr_printspeed","8" };
extern cvar_t cv_bgmvolume;// = { "bgmvolume", "1", true };
extern cvar_t cv_volume;// = { "volume", "0.7", true };
extern cvar_t cv_nosound;// = { "nosound", "0" };
extern cvar_t cv_precache;// = { "precache", "1" };
extern cvar_t cv_loadas8bit;// = { "loadas8bit", "0" };
extern cvar_t cv_bgmbuffer;// = { "bgmbuffer", "4096" };
extern cvar_t cv_ambient_level;// = { "ambient_level", "0.3" };
extern cvar_t cv_ambient_fade;// = { "ambient_fade", "100" };
extern cvar_t cv_snd_noextraupdate;// = { "snd_noextraupdate", "0" };
extern cvar_t cv_snd_show;// = { "snd_show", "0" };
extern cvar_t cv_snd_mixahead;// = { "_snd_mixahead", "0.1", true };
extern cvar_t cv_sv_friction;// = { "sv_friction","4",false,true };
extern cvar_t cv_sv_stopspeed;// = { "sv_stopspeed","100" };
extern cvar_t cv_sv_gravity;// = { "sv_gravity","800",false,true };
extern cvar_t cv_sv_maxvelocity;// = { "sv_maxvelocity","2000" };
extern cvar_t cv_sv_nostep;// = { "sv_nostep","0" };
extern cvar_t cv_sv_edgefriction;// = { "edgefriction", "2" };
extern cvar_t cv_sv_idealpitchscale;// = { "sv_idealpitchscale","0.8" };
extern cvar_t cv_sv_maxspeed;// = { "sv_maxspeed", "320", false, true };
extern cvar_t cv_sv_accelerate;// = { "sv_accelerate", "10" };
// Note that 0 is MODE_WINDOWED
extern cvar_t cv_vid_mode;// = { "vid_mode","0", false };
// Note that 0 is MODE_WINDOWED
extern cvar_t cv_vid_default_mode;// = { "_vid_default_mode","0", true };
// Note that 3 is MODE_FULLSCREEN_DEFAULT
extern cvar_t cv_vid_default_mode_win;// = { "_vid_default_mode_win","3", true };
extern cvar_t cv_vid_wait;// = { "vid_wait","0" };
extern cvar_t cv_vid_nopageflip;// = { "vid_nopageflip","0", true };
extern cvar_t cv_vid_wait_override;// = { "_vid_wait_override", "0", true };
extern cvar_t cv_vid_config_x;// = { "vid_config_x","800", true };
extern cvar_t cv_vid_config_y;// = { "vid_config_y","600", true };
extern cvar_t cv_vid_stretch_by_2;// = { "vid_stretch_by_2","1", true };
extern cvar_t cv_windowed_mouse;// = { "_windowed_mouse","0", true };
extern cvar_t cv_vid_fullscreen_mode;// = { "vid_fullscreen_mode","3", true };
extern cvar_t cv_vid_windowed_mode;// = { "vid_windowed_mode","0", true };
extern cvar_t cv_block_switch;// = { "block_switch","0", true };
extern cvar_t cv_vid_window_x;// = { "vid_window_x", "0", true };
extern cvar_t cv_vid_window_y;// = { "vid_window_y", "0", true };
extern cvar_t cv_lcd_x;// = { "lcd_x","0" };
extern cvar_t cv_lcd_yaw;// = { "lcd_yaw","0" };
extern cvar_t cv_scr_ofsx;// = { "scr_ofsx","0", false };
extern cvar_t cv_scr_ofsy;// = { "scr_ofsy","0", false };
extern cvar_t cv_scr_ofsz;// = { "scr_ofsz","0", false };
extern cvar_t cv_cl_rollspeed;// = { "cl_rollspeed", "200" };
extern cvar_t cv_cl_rollangle;// = { "cl_rollangle", "2.0" };
extern cvar_t cv_cl_bob;// = { "cl_bob","0.02", false };
extern cvar_t cv_cl_bobcycle;// = { "cl_bobcycle","0.6", false };
extern cvar_t cv_cl_bobup;// = { "cl_bobup","0.5", false };
extern cvar_t cv_v_kicktime;// = { "v_kicktime", "0.5", false };
extern cvar_t cv_v_kickroll;// = { "v_kickroll", "0.6", false };
extern cvar_t cv_v_kickpitch;// = { "v_kickpitch", "0.6", false };
extern cvar_t cv_v_iyaw_cycle;// = { "v_iyaw_cycle", "2", false };
extern cvar_t cv_v_iroll_cycle;// = { "v_iroll_cycle", "0.5", false };
extern cvar_t cv_v_ipitch_cycle;// = { "v_ipitch_cycle", "1", false };
extern cvar_t cv_v_iyaw_level;// = { "v_iyaw_level", "0.3", false };
extern cvar_t cv_v_iroll_level;// = { "v_iroll_level", "0.1", false };
extern cvar_t cv_v_ipitch_level;// = { "v_ipitch_level", "0.3", false };
extern cvar_t cv_v_idlescale;// = { "v_idlescale", "0", false };
extern cvar_t cv_crosshair;// = { "crosshair", "0", true };
extern cvar_t cv_cl_crossx;// = { "cl_crossx", "0", false };
extern cvar_t cv_cl_crossy;// = { "cl_crossy", "0", false };
extern cvar_t cv_gl_cshiftpercent;// = { "gl_cshiftpercent", "100", false };
extern cvar_t cv_v_centermove;// = { "v_centermove", "0.15", false };
extern cvar_t cv_v_centerspeed;// = { "v_centerspeed","500" };
extern cvar_t cv_v_gamma;// = { "gamma", "1", true };

//=============================================================================
// globals
// g_ prepended to avoid namespace pollution
//=============================================================================

//=====================================
// common

extern char g_com_gamedir[1024];
extern char g_com_token[1024];
extern char g_vabuf[1024];
extern qboolean g_com_eof;
extern i32 g_com_argc;
extern char **g_com_argv;
extern i32 g_com_filesize;
extern qboolean g_standard_quake;
extern qboolean g_rogue;
extern qboolean g_hipnotic;

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

const i32 g_type_size[ev_COUNT] =
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
extern jmp_buf g_host_abortserver;
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
