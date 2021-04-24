#include "common/cvars.h"

ConVar cv_basedir =
{
    .type = cvart_text,
    .name = "basedir",
    .value = "data",
    .desc = "Base directory for game data",
};

ConVar cv_gamedir =
{
    .type = cvart_text,
    .name = "gamedir",
    .value = "id1",
    .desc = "Name of the active game",
};

ConVar cv_con_logpath =
{
    .type = cvart_text,
    .name = "con_logpath",
    .value = "console.log",
    .desc = "Path to the console log file",
};

ConVar cv_in_pitchscale =
{
    .type = cvart_float,
    .name = "in_pitchscale",
    .value = "2",
    .minFloat = 0.0f,
    .maxFloat = 10.0f,
    .desc = "Pitch input sensitivity",
};

ConVar cv_in_yawscale =
{
    .type = cvart_float,
    .name = "in_yawscale",
    .value = "1",
    .minFloat = 0.0f,
    .maxFloat = 10.0f,
    .desc = "Yaw input sensitivity",
};

ConVar cv_in_movescale =
{
    .type = cvart_float,
    .name = "in_movescale",
    .value = "8",
    .minFloat = 0.0f,
    .maxFloat = 100.0f,
    .desc = "Movement input sensitivity",
};

ConVar cv_r_fov =
{
    .type = cvart_float,
    .name = "r_fov",
    .value = "90",
    .minFloat = 1.0f,
    .maxFloat = 170.0f,
    .desc = "Vertical field of fiew, in degrees",
};

ConVar cv_r_znear =
{
    .type = cvart_float,
    .name = "r_znear",
    .value = "0.1",
    .minFloat = 0.01f,
    .maxFloat = 1.0f,
    .desc = "Near clipping plane, in meters",
};

ConVar cv_r_zfar =
{
    .type = cvart_float,
    .name = "r_zfar",
    .value = "500",
    .minFloat = 1.0f,
    .maxFloat = 1000.0f,
    .desc = "Far clipping plane, in meters",
};

ConVar cv_lm_upload =
{
    .type = cvart_bool,
    .name = "lm_upload",
    .value = "0",
    .desc = "Upload lightmap data to GPU",
};

ConVar cv_r_whitepoint =
{
    .type = cvart_float,
    .name = "r_whitepoint",
    .value = "5.0",
    .minFloat = 1.0f,
    .maxFloat = 20.0f,
    .desc = "Luminance at which tonemapping will clip",
};

ConVar cv_r_display_nits =
{
    .type = cvart_float,
    .name = "r_display_nits",
    .value = "600.0",
    .minFloat = 100.0f,
    .maxFloat = 10000.0f,
    .desc = "Display luminance, in nits (SDR is roughly 300)",
};

ConVar cv_r_ui_nits =
{
    .type = cvart_float,
    .name = "r_ui_nits",
    .value = "600.0",
    .minFloat = 10.0f,
    .maxFloat = 10000.0f,
    .desc = "UI luminance, in nits (SDR is roughly 300)",
};

ConVar cv_r_maxdelqueue =
{
    .type = cvart_int,
    .name = "r_maxdelqueue",
    .value = "1024",
    .minInt = 1 << 6,
    .maxInt = 1 << 16,
    .desc = "Maximum number of released driver objects before force-finalizing",
};

ConVar cv_r_scale =
{
    .type = cvart_float,
    .name = "r_scale",
    .minFloat = 1.0f / 16.0f,
    .maxFloat = 4.0f,
#if _DEBUG
    .value = "0.1",
#else
    .value = "0.5",
#endif // _DEBUG
    .desc = "Render scale",
};

ConVar cv_r_width =
{
    .type = cvart_int,
    .name = "r_width",
    .minInt = 16,
    .maxInt = 8192,
    .value = "1920",
    .desc = "Unscaled render width",
};

ConVar cv_r_height =
{
    .type = cvart_int,
    .name = "r_height",
    .minInt = 16,
    .maxInt = 8192,
    .value = "1080",
    .desc = "Unscaled render height",
};

ConVar cv_r_fpslimit =
{
    .type = cvart_int,
    .name = "r_fps_limit",
    .value = "240",
    .minInt = 1,
    .maxInt = 1000,
    .desc = "Limits fps when above this value"
};

ConVar cv_r_bumpiness =
{
    .type = cvart_float,
    .name = "r_bumpiness",
    .value = "1",
    .minFloat = 0.0f,
    .maxFloat = 11.0f,
    .desc = "Bumpiness of generated normal maps [0, 11]",
};

ConVar cv_r_tex_custom =
{
    .type = cvart_bool,
    .name = "r_tex_custom",
    .value = "0",
    .desc = "Enable loading custom textures",
};

ConVar cv_pt_dist_meters =
{
    .type = cvart_float,
    .name = "pt_dist_meters",
    .value = "1.5",
    .minFloat = 0.1f,
    .maxFloat = 20.0f,
    .desc = "Path tracer light distribution meters per cell"
};

ConVar cv_pt_dist_alpha =
{
    .type = cvart_float,
    .name = "pt_dist_alpha",
    .value = "0.5",
    .minFloat = 0.0f,
    .maxFloat = 1.0f,
    .desc = "Path tracer light distribution update amount",
};

ConVar cv_pt_dist_samples =
{
    .type = cvart_int,
    .name = "pt_dist_samples",
    .value = "300",
    .minInt = 30,
    .maxInt = 1 << 20,
    .desc = "Path tracer light distribution minimum samples per update",
};

ConVar cv_pt_retro =
{
    .type = cvart_bool,
    .name = "pt_retro",
    .value = "0",
    .desc = "Path tracer retro mode (point filtering and diffuse only)",
};

ConVar cv_pt_trace =
{
    .type = cvart_bool,
    .name = "pt_trace",
    .value = "0",
    .desc = "Enable path tracing",
};

ConVar cv_pt_denoise =
{
    .type = cvart_bool,
    .name = "pt_denoise",
    .value = "0",
    .desc = "Denoise path tracing output",
};

ConVar cv_pt_normal =
{
    .type = cvart_bool,
    .name = "pt_normal",
    .value = "0",
    .desc = "Output path tracer normals",
};

ConVar cv_pt_albedo =
{
    .type = cvart_bool,
    .name = "pt_albedo",
    .value = "0",
    .desc = "Output path tracer albedo",
};

ConVar cv_r_refl_gen =
{
    .type = cvart_bool,
    .name = "r_refl_gen",
    .value = "0",
    .desc = "Enable reflection generation",
};

ConVar cv_lm_gen =
{
    .type = cvart_bool,
    .name = "lm_gen",
    .value = "0",
    .desc = "Enable lightmap generation",
};

ConVar cv_lm_density =
{
    .type = cvart_float,
    .name = "lm_density",
    .value = "4",
    .minFloat = 0.1f,
    .maxFloat = 32.0f,
    .desc = "Lightmap texels per meter [0.1, 32]",
};

ConVar cv_lm_timeslice =
{
    .type = cvart_int,
    .name = "lm_timeslice",
    .value = "1",
    .minInt = 1,
    .maxInt = 1024,
    .desc = "Lightmap timeslicing frames per sample [1, 1024]",
};

ConVar cv_lm_spp =
{
    .type = cvart_int,
    .name = "lm_spp",
    .value = "2",
    .minInt = 1,
    .maxInt = 1024,
    .desc = "Lightmap samples per pixel",
};

ConVar cv_r_sun_dir =
{
    .type = cvart_vector,
    .name = "r_sun_dir",
    .value = "0.0 0.968 0.253 0.0",
    .desc = "Sun direction",
};

ConVar cv_r_sun_col =
{
    .type = cvart_color,
    .name = "r_sun_col",
    .value = "1 1 1 1",
    .desc = "Sun color",
};

// https://en.wikipedia.org/wiki/Orders_of_magnitude_(luminance)
// noon: around 2^31
// sunrise: around 2^20
// night: around 2^-10
ConVar cv_r_sun_lum =
{
    .type = cvart_float,
    .name = "r_sun_lum",
    .value = "16.0",
    .minFloat = -10.0f,
    .maxFloat = 31.0f,
    .desc = "Log2 sun luminance",
};

ConVar cv_r_sun_res =
{
    .type = cvart_int,
    .name = "r_sun_res",
#if _DEBUG
    .value = "32",
#else
    .value = "256",
#endif // _DEBUG
    .minInt = 4,
    .maxInt = 4096,
    .desc = "Sky cubemap resolution",
};

ConVar cv_r_sun_steps =
{
    .type = cvart_int,
    .name = "r_sun_steps",
#if _DEBUG
    .value = "8",
#else
    .value = "64",
#endif // _DEBUG
    .minInt = 4,
    .maxInt = 1024,
    .desc = "Sky cubemap raymarch steps",
};

ConVar cv_r_qlights =
{
    .type = cvart_bool,
    .name = "r_qlights",
    .value = "0",
    .desc = "Enable loading quake light entities",
};

ConVar cv_ui_opacity =
{
    .type = cvart_float,
    .name = "ui_opacity",
    .value = "0.95",
    .minFloat = 0.1f,
    .maxFloat = 1.0f,
    .desc = "UI opacity",
};

void ConVars_RegisterAll(void)
{
    ConVar_Reg(&cv_basedir);
    ConVar_Reg(&cv_r_refl_gen);
    ConVar_Reg(&cv_con_logpath);
    ConVar_Reg(&cv_r_fpslimit);
    ConVar_Reg(&cv_gamedir);
    ConVar_Reg(&cv_r_display_nits);
    ConVar_Reg(&cv_r_ui_nits);
    ConVar_Reg(&cv_lm_density);
    ConVar_Reg(&cv_lm_gen);
    ConVar_Reg(&cv_lm_spp);
    ConVar_Reg(&cv_lm_timeslice);
    ConVar_Reg(&cv_lm_upload);
    ConVar_Reg(&cv_r_maxdelqueue);
    ConVar_Reg(&cv_r_bumpiness);
    ConVar_Reg(&cv_in_movescale);
    ConVar_Reg(&cv_in_pitchscale);
    ConVar_Reg(&cv_pt_albedo);
    ConVar_Reg(&cv_pt_denoise);
    ConVar_Reg(&cv_pt_dist_alpha);
    ConVar_Reg(&cv_pt_dist_meters);
    ConVar_Reg(&cv_pt_dist_samples);
    ConVar_Reg(&cv_pt_normal);
    ConVar_Reg(&cv_pt_retro);
    ConVar_Reg(&cv_pt_trace);
    ConVar_Reg(&cv_r_fov);
    ConVar_Reg(&cv_r_height);
    ConVar_Reg(&cv_r_scale);
    ConVar_Reg(&cv_r_sun_col);
    ConVar_Reg(&cv_r_sun_dir);
    ConVar_Reg(&cv_r_sun_lum);
    ConVar_Reg(&cv_r_sun_res);
    ConVar_Reg(&cv_r_sun_steps);
    ConVar_Reg(&cv_r_qlights);
    ConVar_Reg(&cv_r_width);
    ConVar_Reg(&cv_r_zfar);
    ConVar_Reg(&cv_r_znear);
    ConVar_Reg(&cv_r_tex_custom);
    ConVar_Reg(&cv_ui_opacity);
    ConVar_Reg(&cv_r_whitepoint);
    ConVar_Reg(&cv_in_yawscale);
}
