#pragma once

#include "interface/i_types.h"

//=====================================
// cvars

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
extern cvar_t registered;

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
