#include "interface/i_globals.h"

//=====================================
// common

char g_com_cmdline[PIM_PATH];
char g_com_gamedir[PIM_PATH];
char g_com_token[1024];
i32 g_com_argc;
const char **g_com_argv;
qboolean g_standard_quake;
qboolean g_rogue;
qboolean g_hipnotic;
qboolean g_proghack;
qboolean g_msg_badread;
i32 g_msg_readcount;

//=====================================
// mathlib

vec3_t g_vec3_origin;

//=====================================
// wad

i32 g_wad_numlumps;
lumpinfo_t *g_wad_lumps;
u8 *g_wad_base;

//=====================================
// draw

qpic_t *g_draw_disc;

//=====================================
// cvar

cvar_t *g_cvar_vars;

//=====================================
// client

client_static_t g_cls;
client_state_t g_cl;

efrag_t g_cl_efrags[MAX_EFRAGS];
entity_t g_cl_entities[MAX_EDICTS];
entity_t g_cl_static_entities[MAX_STATIC_ENTITIES];
lightstyle_t g_cl_lightstyle[MAX_LIGHTSTYLES];
dlight_t g_cl_dlights[MAX_DLIGHTS];
entity_t g_cl_temp_entities[MAX_TEMP_ENTITIES];
beam_t g_cl_beams[MAX_BEAMS];

i32 g_cl_numvisedicts;
entity_t *g_cl_visedicts[MAX_VISEDICTS];

//=====================================
// input

kbutton_t g_in_mlook;
kbutton_t g_in_klook;
kbutton_t g_in_strafe;
kbutton_t g_in_speed;

//=====================================
// vid

viddef_t g_vid;
u16 g_d_8to16table[256];
u32 g_d_8to24table[256];

//=====================================
// screen

float g_scr_con_current;
float g_scr_conlines;
i32 g_scr_fullupdate;
i32 g_sb_lines;
i32 g_clearnotify;
i32 g_scr_copytop;
i32 g_scr_copyeverything;
qboolean g_scr_disabled_for_loading;
qboolean g_scr_skipupdate;
qboolean g_block_drawing;

//=====================================
// net

qsocket_t *g_net_activeSockets;
qsocket_t *g_net_freeSockets;
i32 g_net_numsockets;

i32 g_net_numlandrivers;
net_landriver_t g_net_landrivers[MAX_NET_DRIVERS];

i32 g_net_numdrivers;
net_driver_t g_net_drivers[MAX_NET_DRIVERS];

i32 g_DEFAULTnet_hostport;
i32 g_net_hostport;
i32 g_net_driverlevel;

i32 g_messagesSent;
i32 g_messagesReceived;
i32 g_unreliableMessagesSent;
i32 g_unreliableMessagesReceived;

i32 g_hostCacheCount;
hostcache_t g_hostcache[HOSTCACHESIZE];

double g_net_time;
sizebuf_t g_net_message;
i32 g_net_activeconnections;

char g_my_tcpip_address[NET_NAMELEN];

qboolean g_tcpipAvailable;
qboolean g_slistInProgress;
qboolean g_slistSilent;
qboolean g_slistLocal;

//=====================================
// cmd

cmd_source_t g_cmd_source;

//=====================================
// sbar

i32 g_sb_lines;

//=====================================
// sound

i32 g_total_channels;
channel_t g_channels[MAX_CHANNELS];

qboolean g_fakedma;
i32 g_fakedma_updates;

volatile dma_t* g_shm;
volatile dma_t g_sn;

i32 g_paintedtime;
i32 g_snd_blocked;
qboolean g_snd_initialized;

vec3_t g_listener_origin;
vec3_t g_listener_forward;
vec3_t g_listener_right;
vec3_t g_listener_up;
vec_t g_sound_nominal_clip_dist;

//=====================================
// render

i32 g_reinit_surfcache;

//=====================================
// progs

const i32 g_type_size[ev_COUNT];

dprograms_t *g_progs;
dfunction_t *g_pr_functions;
char *g_pr_strings;
ddef_t *g_pr_globaldefs;
ddef_t *g_pr_fielddefs;
dstatement_t *g_pr_statements;
globalvars_t *g_pr_global_struct;
float *g_pr_globals;
i32 g_pr_edict_size;

typedef void(*builtin_t) (void);
builtin_t* g_pr_builtins;
i32 g_pr_numbuiltins;
i32 g_pr_argc;
qboolean g_pr_trace;
dfunction_t* g_pr_xfunction;
i32 g_pr_xstatement;
u16 g_pr_crc;

//=====================================
// server

server_static_t g_svs;
server_t g_sv;
client_t *g_host_client;
double g_host_time;
edict_t *g_sv_player;

//=====================================
// model

aliashdr_t *g_pheader;
stvert_t g_stverts[MAXALIASVERTS];
mtriangle_t g_triangles[MAXALIASTRIS];
trivertx_t *g_poseverts[MAXALIASFRAMES];

//=====================================
// keys

keydest_t g_key_dest;
char* g_keybindings[256];
i32 g_key_repeats[256];
i32 g_key_count;
i32 g_key_lastpress;

//=====================================
// console

i32 g_con_totallines;
i32 g_con_backscroll;
qboolean g_con_forcedup;
qboolean g_con_initialized;
u8* g_con_chars;
i32 g_con_notifylines;

//=====================================
// view

u8 g_gammatable[256];
u8 g_ramps[3][256];
float g_v_blend[4];

//=====================================
// menu

i32 g_m_activenet;

//=====================================
// glquake

i32 g_texture_extension_number;
i32 g_texture_mode;
float g_gldepthmin, g_gldepthmax;
glvert_t g_glv;
i32 g_glx, g_gly, g_glwidth, g_glheight;
entity_t g_r_worldentity;
qboolean g_r_cache_thrash;
vec3_t g_modelorg, g_r_entorigin;
entity_t *g_currententity;
i32 g_r_visframecount;
i32 g_r_framecount;
mplane_t g_frustum[4];
i32 g_c_brush_polys, g_c_alias_polys;
vec3_t g_vup;
vec3_t g_vpn;
vec3_t g_vright;
vec3_t g_r_origin;
refdef_t g_r_refdef;
mleaf_t *g_r_viewleaf, *g_r_oldviewleaf;
texture_t *g_r_notexture_mip;
i32 g_d_lightstylevalue[256];
qboolean g_envmap;
i32 g_currenttexture;
i32 g_cnttextures[2];
i32 g_particletexture;
i32 g_playertextures;
i32 g_skytexturenum;
i32 g_gl_lightmap_format;
i32 g_gl_solid_format;
i32 g_gl_alpha_format;
i32 g_mirrortexturenum;
qboolean g_mirror;
mplane_t* g_mirror_plane;
float g_r_world_matrix[16];
const char* g_gl_vendor;
const char* g_gl_renderer;
const char* g_gl_version;
const char* g_gl_extensions;
qboolean g_gl_mtexable;

//=====================================
// host

cvar_t g_sys_ticrate;
cvar_t g_sys_nostdout;
cvar_t g_developer;
cvar_t g_chase_active;
qboolean g_noclip_anglehack;
qboolean g_host_initialized;
qboolean g_isDedicated;
qboolean g_msg_suppress_1;
quakeparms_t g_host_parms;
u8* g_host_basepal;
u8* g_host_colormap;
i32 g_host_framecount;
double g_host_frametime;
double g_realtime;
i32 g_current_skill;
i32 g_minimum_memory;

//=====================================
