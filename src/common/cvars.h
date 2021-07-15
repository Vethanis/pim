#pragma once

#include "common/cvar.h"

PIM_C_BEGIN

extern ConVar cv_basedir;
extern ConVar cv_gamedir;
extern ConVar cv_con_logpath;
extern ConVar cv_in_pitchscale;
extern ConVar cv_in_yawscale;
extern ConVar cv_in_movescale;
extern ConVar cv_r_fov;
extern ConVar cv_r_znear;
extern ConVar cv_r_zfar;
extern ConVar cv_r_whitepoint;
extern ConVar cv_r_display_nits_min;
extern ConVar cv_r_display_nits_max;
extern ConVar cv_r_ui_nits;
extern ConVar cv_r_maxdelqueue;
extern ConVar cv_r_scale;
extern ConVar cv_r_width;
extern ConVar cv_r_height;
extern ConVar cv_r_fpslimit;
extern ConVar cv_r_bumpiness;
extern ConVar cv_r_tex_custom;
extern ConVar cv_pt_dist_meters;
extern ConVar cv_pt_trace;
extern ConVar cv_pt_denoise;
extern ConVar cv_pt_normal;
extern ConVar cv_pt_albedo;
extern ConVar cv_r_refl_gen;
extern ConVar cv_r_sun_dir;
extern ConVar cv_r_sun_col;
extern ConVar cv_r_sun_lum;
extern ConVar cv_r_sun_res;
extern ConVar cv_r_sun_steps;
extern ConVar cv_r_qlights;
extern ConVar cv_ui_opacity;

extern ConVar cv_lm_upload;
extern ConVar cv_lm_gen;
extern ConVar cv_lm_density;
extern ConVar cv_lm_timeslice;
extern ConVar cv_lm_spp;

extern ConVar cv_exp_standard;
extern ConVar cv_exp_manual;
extern ConVar cv_exp_aperture;
extern ConVar cv_exp_shutter;
extern ConVar cv_exp_adaptrate;
extern ConVar cv_exp_evoffset;
extern ConVar cv_exp_evmin;
extern ConVar cv_exp_evmax;
extern ConVar cv_exp_cdfmin;
extern ConVar cv_exp_cdfmax;

extern ConVar cv_fullscreen;

void ConVars_RegisterAll(void);

PIM_C_END
