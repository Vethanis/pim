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

#include "interface/i_cvars.h"

#if QUAKE_IMPL

#include "interface/i_cvar.h"

cvar_t cv_chase_back =  { "chase_back", "100" };
cvar_t cv_chase_up =  { "chase_up", "16" };
cvar_t cv_chase_right =  { "chase_right", "0" };
cvar_t cv_chase_active =  { "chase_active", "0" };
cvar_t cv_cl_upspeed =  { "cl_upspeed","200" };
cvar_t cv_cl_forwardspeed =  { "cl_forwardspeed","200", true };
cvar_t cv_cl_backspeed =  { "cl_backspeed","200", true };
cvar_t cv_cl_sidespeed =  { "cl_sidespeed","350" };
cvar_t cv_cl_movespeedkey =  { "cl_movespeedkey","2.0" };
cvar_t cv_cl_yawspeed =  { "cl_yawspeed","140" };
cvar_t cv_cl_pitchspeed =  { "cl_pitchspeed","150" };
cvar_t cv_cl_anglespeedkey =  { "cl_anglespeedkey","1.5" };
cvar_t cv_cl_name =  { "_cl_name", "player", true };
cvar_t cv_cl_color =  { "_cl_color", "0", true };
cvar_t cv_cl_shownet =  { "cl_shownet","0" }; // can be 0, 1, or 2
cvar_t cv_cl_nolerp =  { "cl_nolerp","0" };
cvar_t cv_lookspring =  { "lookspring","0", true };
cvar_t cv_lookstrafe =  { "lookstrafe","0", true };
cvar_t cv_sensitivity =  { "sensitivity","3", true };
cvar_t cv_m_pitch =  { "m_pitch","0.022", true };
cvar_t cv_m_yaw =  { "m_yaw","0.022", true };
cvar_t cv_m_forward =  { "m_forward","1", true };
cvar_t cv_m_side =  { "m_side","0.8", true };
cvar_t cv_registered =  { "registered","0" };
cvar_t cv_cmdline =  { "cmdline","0", false, true };
cvar_t cv_con_notifytime =  { "con_notifytime","3" }; //seconds
cvar_t cv_d_subdiv16 =  { "d_subdiv16", "1" };
cvar_t cv_d_mipcap =  { "d_mipcap", "0" };
cvar_t cv_d_mipscale =  { "d_mipscale", "1" };
cvar_t cv_gl_nobind =  { "gl_nobind", "0" };
cvar_t cv_gl_max_size =  { "gl_max_size", "1024" };
cvar_t cv_gl_picmip =  { "gl_picmip", "0" };
cvar_t cv_gl_subdivide_size =  { "gl_subdivide_size", "128", true };
cvar_t cv_r_norefresh =  { "r_norefresh","0" };
cvar_t cv_r_speeds =  { "r_speeds","0" };
cvar_t cv_r_lightmap =  { "r_lightmap","0" };
cvar_t cv_r_shadows =  { "r_shadows","0" };
cvar_t cv_r_mirroralpha =  { "r_mirroralpha","1" };
cvar_t cv_r_wateralpha =  { "r_wateralpha","1" };
cvar_t cv_r_dynamic =  { "r_dynamic","1" };
cvar_t cv_r_novis =  { "r_novis","0" };
cvar_t cv_gl_finish =  { "gl_finish","0" };
cvar_t cv_gl_clear =  { "gl_clear","0" };
cvar_t cv_gl_cull =  { "gl_cull","1" };
cvar_t cv_gl_texsort =  { "gl_texsort","1" };
cvar_t cv_gl_smoothmodels =  { "gl_smoothmodels","1" };
cvar_t cv_gl_affinemodels =  { "gl_affinemodels","0" };
cvar_t cv_gl_polyblend =  { "gl_polyblend","1" };
cvar_t cv_gl_flashblend =  { "gl_flashblend","1" };
cvar_t cv_gl_playermip =  { "gl_playermip","0" };
cvar_t cv_gl_nocolors =  { "gl_nocolors","0" };
cvar_t cv_gl_keeptjunctions =  { "gl_keeptjunctions","0" };
cvar_t cv_gl_reporttjunctions =  { "gl_reporttjunctions","0" };
cvar_t cv_gl_doubleeyes =  { "gl_doubleeys", "1" };
cvar_t cv_gl_triplebuffer =  { "gl_triplebuffer", "1", true };
cvar_t cv_vid_redrawfull =  { "vid_redrawfull","0",false };
cvar_t cv_vid_waitforrefresh =  { "vid_waitforrefresh","0",true };
cvar_t cv_mouse_button_commands[3] =
{
    {"mouse1","+attack"},
    {"mouse2","+strafe"},
    {"mouse3","+forward"},
};
cvar_t cv_gl_ztrick =  { "gl_ztrick","1" };
cvar_t cv_host_framerate =  { "host_framerate","0" }; // set for slow motion
cvar_t cv_host_speeds =  { "host_speeds","0" };   // set for running times
cvar_t cv_sys_ticrate =  { "sys_ticrate","0.05" };
cvar_t cv_serverprofile =  { "serverprofile","0" };
cvar_t cv_fraglimit =  { "fraglimit","0",false,true };
cvar_t cv_timelimit =  { "timelimit","0",false,true };
cvar_t cv_teamplay =  { "teamplay","0",false,true };
cvar_t cv_samelevel =  { "samelevel","0" };
cvar_t cv_noexit =  { "noexit","0",false,true };
cvar_t cv_developer =  { "developer","1" };
cvar_t cv_skill =  { "skill","1" }; // 0 - 3
cvar_t cv_deathmatch =  { "deathmatch","0" }; // 0, 1, or 2
cvar_t cv_coop =  { "coop","0" }; // 0 or 1
cvar_t cv_pausable =  { "pausable","1" };
cvar_t cv_temp1 =  { "temp1","0" };
cvar_t cv_m_filter =  { "m_filter","0" };
cvar_t cv_in_joystick =  { "joystick","0", true };
cvar_t cv_joy_name =  { "joyname", "joystick" };
cvar_t cv_joy_advanced =  { "joyadvanced", "0" };
cvar_t cv_joy_advaxisx =  { "joyadvaxisx", "0" };
cvar_t cv_joy_advaxisy =  { "joyadvaxisy", "0" };
cvar_t cv_joy_advaxisz =  { "joyadvaxisz", "0" };
cvar_t cv_joy_advaxisr =  { "joyadvaxisr", "0" };
cvar_t cv_joy_advaxisu =  { "joyadvaxisu", "0" };
cvar_t cv_joy_advaxisv =  { "joyadvaxisv", "0" };
cvar_t cv_joy_forwardthreshold =  { "joyforwardthreshold", "0.15" };
cvar_t cv_joy_sidethreshold =  { "joysidethreshold", "0.15" };
cvar_t cv_joy_pitchthreshold =  { "joypitchthreshold", "0.15" };
cvar_t cv_joy_yawthreshold =  { "joyyawthreshold", "0.15" };
cvar_t cv_joy_forwardsensitivity =  { "joyforwardsensitivity", "-1.0" };
cvar_t cv_joy_sidesensitivity =  { "joysidesensitivity", "-1.0" };
cvar_t cv_joy_pitchsensitivity =  { "joypitchsensitivity", "1.0" };
cvar_t cv_joy_yawsensitivity =  { "joyyawsensitivity", "-1.0" };
cvar_t cv_joy_wwhack1 =  { "joywwhack1", "0.0" };
cvar_t cv_joy_wwhack2 =  { "joywwhack2", "0.0" };
cvar_t cv_net_messagetimeout =  { "net_messagetimeout","300" };
cvar_t cv_hostname =  { "hostname", "UNNAMED" };
cvar_t cv_config_com_port =  { "_config_com_port", "0x3f8", true };
cvar_t cv_config_com_irq =  { "_config_com_irq", "4", true };
cvar_t cv_config_com_baud =  { "_config_com_baud", "57600", true };
cvar_t cv_config_com_modem =  { "_config_com_modem", "1", true };
cvar_t cv_config_modem_dialtype =  { "_config_modem_dialtype", "T", true };
cvar_t cv_config_modem_clear =  { "_config_modem_clear", "ATZ", true };
cvar_t cv_config_modem_init =  { "_config_modem_init", "", true };
cvar_t cv_config_modem_hangup =  { "_config_modem_hangup", "AT H", true };
cvar_t cv_sv_aim =  { "sv_aim", "0.93" };
cvar_t cv_nomonsters =  { "nomonsters", "0" };
cvar_t cv_gamecfg =  { "gamecfg", "0" };
cvar_t cv_scratch1 =  { "scratch1", "0" };
cvar_t cv_scratch2 =  { "scratch2", "0" };
cvar_t cv_scratch3 =  { "scratch3", "0" };
cvar_t cv_scratch4 =  { "scratch4", "0" };
cvar_t cv_savedgamecfg =  { "savedgamecfg", "0", true };
cvar_t cv_saved1 =  { "saved1", "0", true };
cvar_t cv_saved2 =  { "saved2", "0", true };
cvar_t cv_saved3 =  { "saved3", "0", true };
cvar_t cv_saved4 =  { "saved4", "0", true };
cvar_t cv_r_draworder =  { "r_draworder","0" };
cvar_t cv_r_timegraph =  { "r_timegraph","0" };
cvar_t cv_r_graphheight =  { "r_graphheight","10" };
cvar_t cv_r_clearcolor =  { "r_clearcolor","2" };
cvar_t cv_r_waterwarp =  { "r_waterwarp","1" };
cvar_t cv_r_fullbright =  { "r_fullbright","0" };
cvar_t cv_r_drawentities =  { "r_drawentities","1" };
cvar_t cv_r_drawviewmodel =  { "r_drawviewmodel","1" };
cvar_t cv_r_aliasstats =  { "r_polymodelstats","0" };
cvar_t cv_r_dspeeds =  { "r_dspeeds","0" };
cvar_t cv_r_drawflat =  { "r_drawflat", "0" };
cvar_t cv_r_ambient =  { "r_ambient", "0" };
cvar_t cv_r_reportsurfout =  { "r_reportsurfout", "0" };
cvar_t cv_r_maxsurfs =  { "r_maxsurfs", "0" };
cvar_t cv_r_numsurfs =  { "r_numsurfs", "0" };
cvar_t cv_r_reportedgeout =  { "r_reportedgeout", "0" };
cvar_t cv_r_maxedges =  { "r_maxedges", "0" };
cvar_t cv_r_numedges =  { "r_numedges", "0" };
cvar_t cv_r_aliastransbase =  { "r_aliastransbase", "200" };
cvar_t cv_r_aliastransadj =  { "r_aliastransadj", "100" };
cvar_t cv_scr_viewsize =  { "viewsize","100", true };
cvar_t cv_scr_fov =  { "fov","90" }; // 10 - 170
cvar_t cv_scr_conspeed =  { "scr_conspeed","300" };
cvar_t cv_scr_centertime =  { "scr_centertime","2" };
cvar_t cv_scr_showram =  { "showram","1" };
cvar_t cv_scr_showturtle =  { "showturtle","0" };
cvar_t cv_scr_showpause =  { "showpause","1" };
cvar_t cv_scr_printspeed =  { "scr_printspeed","8" };
cvar_t cv_bgmvolume =  { "bgmvolume", "1", true };
cvar_t cv_volume =  { "volume", "0.7", true };
cvar_t cv_nosound =  { "nosound", "0" };
cvar_t cv_precache =  { "precache", "1" };
cvar_t cv_loadas8bit =  { "loadas8bit", "0" };
cvar_t cv_bgmbuffer =  { "bgmbuffer", "4096" };
cvar_t cv_ambient_level =  { "ambient_level", "0.3" };
cvar_t cv_ambient_fade =  { "ambient_fade", "100" };
cvar_t cv_snd_noextraupdate =  { "snd_noextraupdate", "0" };
cvar_t cv_snd_show =  { "snd_show", "0" };
cvar_t cv_snd_mixahead =  { "_snd_mixahead", "0.1", true };
cvar_t cv_sv_friction =  { "sv_friction","4",false,true };
cvar_t cv_sv_stopspeed =  { "sv_stopspeed","100" };
cvar_t cv_sv_gravity =  { "sv_gravity","800",false,true };
cvar_t cv_sv_maxvelocity =  { "sv_maxvelocity","2000" };
cvar_t cv_sv_nostep =  { "sv_nostep","0" };
cvar_t cv_sv_edgefriction =  { "edgefriction", "2" };
cvar_t cv_sv_idealpitchscale =  { "sv_idealpitchscale","0.8" };
cvar_t cv_sv_maxspeed =  { "sv_maxspeed", "320", false, true };
cvar_t cv_sv_accelerate =  { "sv_accelerate", "10" };
// Note that 0 is MODE_WINDOWED
cvar_t cv_vid_mode =  { "vid_mode","0", false };
// Note that 0 is MODE_WINDOWED
cvar_t cv_vid_default_mode =  { "_vid_default_mode","0", true };
// Note that 3 is MODE_FULLSCREEN_DEFAULT
cvar_t cv_vid_default_mode_win =  { "_vid_default_mode_win","3", true };
cvar_t cv_vid_wait =  { "vid_wait","0" };
cvar_t cv_vid_nopageflip =  { "vid_nopageflip","0", true };
cvar_t cv_vid_wait_override =  { "_vid_wait_override", "0", true };
cvar_t cv_vid_config_x =  { "vid_config_x","800", true };
cvar_t cv_vid_config_y =  { "vid_config_y","600", true };
cvar_t cv_vid_stretch_by_2 =  { "vid_stretch_by_2","1", true };
cvar_t cv_windowed_mouse =  { "_windowed_mouse","0", true };
cvar_t cv_vid_fullscreen_mode =  { "vid_fullscreen_mode","3", true };
cvar_t cv_vid_windowed_mode =  { "vid_windowed_mode","0", true };
cvar_t cv_block_switch =  { "block_switch","0", true };
cvar_t cv_vid_window_x =  { "vid_window_x", "0", true };
cvar_t cv_vid_window_y =  { "vid_window_y", "0", true };
cvar_t cv_lcd_x =  { "lcd_x","0" };
cvar_t cv_lcd_yaw =  { "lcd_yaw","0" };
cvar_t cv_scr_ofsx =  { "scr_ofsx","0", false };
cvar_t cv_scr_ofsy =  { "scr_ofsy","0", false };
cvar_t cv_scr_ofsz =  { "scr_ofsz","0", false };
cvar_t cv_cl_rollspeed =  { "cl_rollspeed", "200" };
cvar_t cv_cl_rollangle =  { "cl_rollangle", "2.0" };
cvar_t cv_cl_bob =  { "cl_bob","0.02", false };
cvar_t cv_cl_bobcycle =  { "cl_bobcycle","0.6", false };
cvar_t cv_cl_bobup =  { "cl_bobup","0.5", false };
cvar_t cv_v_kicktime =  { "v_kicktime", "0.5", false };
cvar_t cv_v_kickroll =  { "v_kickroll", "0.6", false };
cvar_t cv_v_kickpitch =  { "v_kickpitch", "0.6", false };
cvar_t cv_v_iyaw_cycle =  { "v_iyaw_cycle", "2", false };
cvar_t cv_v_iroll_cycle =  { "v_iroll_cycle", "0.5", false };
cvar_t cv_v_ipitch_cycle =  { "v_ipitch_cycle", "1", false };
cvar_t cv_v_iyaw_level =  { "v_iyaw_level", "0.3", false };
cvar_t cv_v_iroll_level =  { "v_iroll_level", "0.1", false };
cvar_t cv_v_ipitch_level =  { "v_ipitch_level", "0.3", false };
cvar_t cv_v_idlescale =  { "v_idlescale", "0", false };
cvar_t cv_crosshair =  { "crosshair", "0", true };
cvar_t cv_cl_crossx =  { "cl_crossx", "0", false };
cvar_t cv_cl_crossy =  { "cl_crossy", "0", false };
cvar_t cv_gl_cshiftpercent =  { "gl_cshiftpercent", "100", false };
cvar_t cv_v_centermove =  { "v_centermove", "0.15", false };
cvar_t cv_v_centerspeed =  { "v_centerspeed","500" };
cvar_t cv_v_gamma =  { "gamma", "1", true };

void Cvars_RegisterAll(void)
{
    Cvar_RegisterVariable(&cv_chase_back);
    Cvar_RegisterVariable(&cv_chase_up);
    Cvar_RegisterVariable(&cv_chase_right);
    Cvar_RegisterVariable(&cv_chase_active);
    Cvar_RegisterVariable(&cv_cl_upspeed);
    Cvar_RegisterVariable(&cv_cl_forwardspeed);
    Cvar_RegisterVariable(&cv_cl_backspeed);
    Cvar_RegisterVariable(&cv_cl_sidespeed);
    Cvar_RegisterVariable(&cv_cl_movespeedkey);
    Cvar_RegisterVariable(&cv_cl_yawspeed);
    Cvar_RegisterVariable(&cv_cl_pitchspeed);
    Cvar_RegisterVariable(&cv_cl_anglespeedkey);
    Cvar_RegisterVariable(&cv_cl_name);
    Cvar_RegisterVariable(&cv_cl_color);
    Cvar_RegisterVariable(&cv_cl_shownet);
    Cvar_RegisterVariable(&cv_cl_nolerp);
    Cvar_RegisterVariable(&cv_lookspring);
    Cvar_RegisterVariable(&cv_lookstrafe);
    Cvar_RegisterVariable(&cv_sensitivity);
    Cvar_RegisterVariable(&cv_m_pitch);
    Cvar_RegisterVariable(&cv_m_yaw);
    Cvar_RegisterVariable(&cv_m_forward);
    Cvar_RegisterVariable(&cv_m_side);
    Cvar_RegisterVariable(&cv_registered);
    Cvar_RegisterVariable(&cv_cmdline);
    Cvar_RegisterVariable(&cv_con_notifytime);
    Cvar_RegisterVariable(&cv_d_subdiv16);
    Cvar_RegisterVariable(&cv_d_mipcap);
    Cvar_RegisterVariable(&cv_d_mipscale);
    Cvar_RegisterVariable(&cv_gl_nobind);
    Cvar_RegisterVariable(&cv_gl_max_size);
    Cvar_RegisterVariable(&cv_gl_picmip);
    Cvar_RegisterVariable(&cv_gl_subdivide_size);
    Cvar_RegisterVariable(&cv_r_norefresh);
    Cvar_RegisterVariable(&cv_r_speeds);
    Cvar_RegisterVariable(&cv_r_lightmap);
    Cvar_RegisterVariable(&cv_r_shadows);
    Cvar_RegisterVariable(&cv_r_mirroralpha);
    Cvar_RegisterVariable(&cv_r_wateralpha);
    Cvar_RegisterVariable(&cv_r_dynamic);
    Cvar_RegisterVariable(&cv_r_novis);
    Cvar_RegisterVariable(&cv_gl_finish);
    Cvar_RegisterVariable(&cv_gl_clear);
    Cvar_RegisterVariable(&cv_gl_cull);
    Cvar_RegisterVariable(&cv_gl_texsort);
    Cvar_RegisterVariable(&cv_gl_smoothmodels);
    Cvar_RegisterVariable(&cv_gl_affinemodels);
    Cvar_RegisterVariable(&cv_gl_polyblend);
    Cvar_RegisterVariable(&cv_gl_flashblend);
    Cvar_RegisterVariable(&cv_gl_playermip);
    Cvar_RegisterVariable(&cv_gl_nocolors);
    Cvar_RegisterVariable(&cv_gl_keeptjunctions);
    Cvar_RegisterVariable(&cv_gl_reporttjunctions);
    Cvar_RegisterVariable(&cv_gl_doubleeyes);
    Cvar_RegisterVariable(&cv_gl_triplebuffer);
    Cvar_RegisterVariable(&cv_vid_redrawfull);
    Cvar_RegisterVariable(&cv_vid_waitforrefresh);
    Cvar_RegisterVariable(&cv_mouse_button_commands[0]);
    Cvar_RegisterVariable(&cv_mouse_button_commands[1]);
    Cvar_RegisterVariable(&cv_mouse_button_commands[2]);
    Cvar_RegisterVariable(&cv_gl_ztrick);
    Cvar_RegisterVariable(&cv_host_framerate);
    Cvar_RegisterVariable(&cv_host_speeds);
    Cvar_RegisterVariable(&cv_sys_ticrate);
    Cvar_RegisterVariable(&cv_serverprofile);
    Cvar_RegisterVariable(&cv_fraglimit);
    Cvar_RegisterVariable(&cv_timelimit);
    Cvar_RegisterVariable(&cv_teamplay);
    Cvar_RegisterVariable(&cv_samelevel);
    Cvar_RegisterVariable(&cv_noexit);
    Cvar_RegisterVariable(&cv_developer);
    Cvar_RegisterVariable(&cv_skill);
    Cvar_RegisterVariable(&cv_deathmatch);
    Cvar_RegisterVariable(&cv_coop);
    Cvar_RegisterVariable(&cv_pausable);
    Cvar_RegisterVariable(&cv_temp1);
    Cvar_RegisterVariable(&cv_m_filter);
    Cvar_RegisterVariable(&cv_in_joystick);
    Cvar_RegisterVariable(&cv_joy_name);
    Cvar_RegisterVariable(&cv_joy_advanced);
    Cvar_RegisterVariable(&cv_joy_advaxisx);
    Cvar_RegisterVariable(&cv_joy_advaxisy);
    Cvar_RegisterVariable(&cv_joy_advaxisz);
    Cvar_RegisterVariable(&cv_joy_advaxisr);
    Cvar_RegisterVariable(&cv_joy_advaxisu);
    Cvar_RegisterVariable(&cv_joy_advaxisv);
    Cvar_RegisterVariable(&cv_joy_forwardthreshold);
    Cvar_RegisterVariable(&cv_joy_sidethreshold);
    Cvar_RegisterVariable(&cv_joy_pitchthreshold);
    Cvar_RegisterVariable(&cv_joy_yawthreshold);
    Cvar_RegisterVariable(&cv_joy_forwardsensitivity);
    Cvar_RegisterVariable(&cv_joy_sidesensitivity);
    Cvar_RegisterVariable(&cv_joy_pitchsensitivity);
    Cvar_RegisterVariable(&cv_joy_yawsensitivity);
    Cvar_RegisterVariable(&cv_joy_wwhack1);
    Cvar_RegisterVariable(&cv_joy_wwhack2);
    Cvar_RegisterVariable(&cv_net_messagetimeout);
    Cvar_RegisterVariable(&cv_hostname);
    Cvar_RegisterVariable(&cv_config_com_port);
    Cvar_RegisterVariable(&cv_config_com_irq);
    Cvar_RegisterVariable(&cv_config_com_baud);
    Cvar_RegisterVariable(&cv_config_com_modem);
    Cvar_RegisterVariable(&cv_config_modem_dialtype);
    Cvar_RegisterVariable(&cv_config_modem_clear);
    Cvar_RegisterVariable(&cv_config_modem_init);
    Cvar_RegisterVariable(&cv_config_modem_hangup);
    Cvar_RegisterVariable(&cv_sv_aim);
    Cvar_RegisterVariable(&cv_nomonsters);
    Cvar_RegisterVariable(&cv_gamecfg);
    Cvar_RegisterVariable(&cv_scratch1);
    Cvar_RegisterVariable(&cv_scratch2);
    Cvar_RegisterVariable(&cv_scratch3);
    Cvar_RegisterVariable(&cv_scratch4);
    Cvar_RegisterVariable(&cv_savedgamecfg);
    Cvar_RegisterVariable(&cv_saved1);
    Cvar_RegisterVariable(&cv_saved2);
    Cvar_RegisterVariable(&cv_saved3);
    Cvar_RegisterVariable(&cv_saved4);
    Cvar_RegisterVariable(&cv_r_draworder);
    Cvar_RegisterVariable(&cv_r_timegraph);
    Cvar_RegisterVariable(&cv_r_graphheight);
    Cvar_RegisterVariable(&cv_r_clearcolor);
    Cvar_RegisterVariable(&cv_r_waterwarp);
    Cvar_RegisterVariable(&cv_r_fullbright);
    Cvar_RegisterVariable(&cv_r_drawentities);
    Cvar_RegisterVariable(&cv_r_drawviewmodel);
    Cvar_RegisterVariable(&cv_r_aliasstats);
    Cvar_RegisterVariable(&cv_r_dspeeds);
    Cvar_RegisterVariable(&cv_r_drawflat);
    Cvar_RegisterVariable(&cv_r_ambient);
    Cvar_RegisterVariable(&cv_r_reportsurfout);
    Cvar_RegisterVariable(&cv_r_maxsurfs);
    Cvar_RegisterVariable(&cv_r_numsurfs);
    Cvar_RegisterVariable(&cv_r_reportedgeout);
    Cvar_RegisterVariable(&cv_r_maxedges);
    Cvar_RegisterVariable(&cv_r_numedges);
    Cvar_RegisterVariable(&cv_r_aliastransbase);
    Cvar_RegisterVariable(&cv_r_aliastransadj);
    Cvar_RegisterVariable(&cv_scr_viewsize);
    Cvar_RegisterVariable(&cv_scr_fov);
    Cvar_RegisterVariable(&cv_scr_conspeed);
    Cvar_RegisterVariable(&cv_scr_centertime);
    Cvar_RegisterVariable(&cv_scr_showram);
    Cvar_RegisterVariable(&cv_scr_showturtle);
    Cvar_RegisterVariable(&cv_scr_showpause);
    Cvar_RegisterVariable(&cv_scr_printspeed);
    Cvar_RegisterVariable(&cv_bgmvolume);
    Cvar_RegisterVariable(&cv_volume);
    Cvar_RegisterVariable(&cv_nosound);
    Cvar_RegisterVariable(&cv_precache);
    Cvar_RegisterVariable(&cv_loadas8bit);
    Cvar_RegisterVariable(&cv_bgmbuffer);
    Cvar_RegisterVariable(&cv_ambient_level);
    Cvar_RegisterVariable(&cv_ambient_fade);
    Cvar_RegisterVariable(&cv_snd_noextraupdate);
    Cvar_RegisterVariable(&cv_snd_show);
    Cvar_RegisterVariable(&cv_snd_mixahead);
    Cvar_RegisterVariable(&cv_sv_friction);
    Cvar_RegisterVariable(&cv_sv_stopspeed);
    Cvar_RegisterVariable(&cv_sv_gravity);
    Cvar_RegisterVariable(&cv_sv_maxvelocity);
    Cvar_RegisterVariable(&cv_sv_nostep);
    Cvar_RegisterVariable(&cv_sv_edgefriction);
    Cvar_RegisterVariable(&cv_sv_idealpitchscale);
    Cvar_RegisterVariable(&cv_sv_maxspeed);
    Cvar_RegisterVariable(&cv_sv_accelerate);
    Cvar_RegisterVariable(&cv_vid_mode);
    Cvar_RegisterVariable(&cv_vid_default_mode);
    Cvar_RegisterVariable(&cv_vid_default_mode_win);
    Cvar_RegisterVariable(&cv_vid_wait);
    Cvar_RegisterVariable(&cv_vid_nopageflip);
    Cvar_RegisterVariable(&cv_vid_wait_override);
    Cvar_RegisterVariable(&cv_vid_config_x);
    Cvar_RegisterVariable(&cv_vid_config_y);
    Cvar_RegisterVariable(&cv_vid_stretch_by_2);
    Cvar_RegisterVariable(&cv_windowed_mouse);
    Cvar_RegisterVariable(&cv_vid_fullscreen_mode);
    Cvar_RegisterVariable(&cv_vid_windowed_mode);
    Cvar_RegisterVariable(&cv_block_switch);
    Cvar_RegisterVariable(&cv_vid_window_x);
    Cvar_RegisterVariable(&cv_vid_window_y);
    Cvar_RegisterVariable(&cv_lcd_x);
    Cvar_RegisterVariable(&cv_lcd_yaw);
    Cvar_RegisterVariable(&cv_scr_ofsx);
    Cvar_RegisterVariable(&cv_scr_ofsy);
    Cvar_RegisterVariable(&cv_scr_ofsz);
    Cvar_RegisterVariable(&cv_cl_rollspeed);
    Cvar_RegisterVariable(&cv_cl_rollangle);
    Cvar_RegisterVariable(&cv_cl_bob);
    Cvar_RegisterVariable(&cv_cl_bobcycle);
    Cvar_RegisterVariable(&cv_cl_bobup);
    Cvar_RegisterVariable(&cv_v_kicktime);
    Cvar_RegisterVariable(&cv_v_kickroll);
    Cvar_RegisterVariable(&cv_v_kickpitch);
    Cvar_RegisterVariable(&cv_v_iyaw_cycle);
    Cvar_RegisterVariable(&cv_v_iroll_cycle);
    Cvar_RegisterVariable(&cv_v_ipitch_cycle);
    Cvar_RegisterVariable(&cv_v_iyaw_level);
    Cvar_RegisterVariable(&cv_v_iroll_level);
    Cvar_RegisterVariable(&cv_v_ipitch_level);
    Cvar_RegisterVariable(&cv_v_idlescale);
    Cvar_RegisterVariable(&cv_crosshair);
    Cvar_RegisterVariable(&cv_cl_crossx);
    Cvar_RegisterVariable(&cv_cl_crossy);
    Cvar_RegisterVariable(&cv_gl_cshiftpercent);
    Cvar_RegisterVariable(&cv_v_centermove);
    Cvar_RegisterVariable(&cv_v_centerspeed);
    Cvar_RegisterVariable(&cv_v_gamma);
}

#endif QUAKE_IMPL
