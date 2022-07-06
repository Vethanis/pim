#include "common/cvars.h"

ConVar cv_basedir =
{
    .type = cvart_text,
    .name = "basedir",
    .value = "data",
    .desc = "Base directory for game data",
};

ConVar cv_game =
{
    .type = cvart_text,
    .name = "game",
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

ConVar cv_r_whitepoint =
{
    .type = cvart_float,
    .name = "r_whitepoint",
    .value = "1.2",
    .minFloat = 1.0f,
    .maxFloat = 5.0f,
    .desc = "Luminance at which tonemapping will clip",
};

ConVar cv_r_display_nits_min =
{
    .type = cvart_float,
    .name = "r_display_nits_min",
    .value = "0.0",
    .minFloat = 0.0f,
    .maxFloat = 1000.0f,
    .desc = "Min display luminance, in nits",
};
ConVar cv_r_display_nits_max =
{
    .type = cvart_float,
    .name = "r_display_nits_max",
    .value = "600.0",
    .minFloat = 100.0f,
    .maxFloat = 10000.0f,
    .desc = "Max display luminance, in nits",
};

ConVar cv_r_ui_nits =
{
    .type = cvart_float,
    .name = "r_ui_nits",
    .value = "300.0",
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
    .value = "0.75",
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

ConVar cv_r_brdflut_spf =
{
    .type = cvart_int,
    .name = "r_brdflut_spf",
    .value = "10",
    .minInt = 0,
    .maxInt = 1000,
    .desc = "Brdf LUT samples per frame",
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

ConVar cv_r_sun_dir =
{
    .type = cvart_vector,
    .name = "r_sun_dir",
    .value = "0.882 0.195 0.429 0.0",
    .desc = "Sun direction",
};

// https://en.wikipedia.org/wiki/Orders_of_magnitude_(luminance)
// noon: around 2^31 (2147483648)
// sunrise: around 2^20 (1048576)
// night: around 2^-10 (0.0009765625)
ConVar cv_r_sun_lum =
{
    .type = cvart_float,
    .flags = cvarf_logarithmic,
    .name = "r_sun_lum",
    .value = "3800.0",
    .minFloat = 0.0009765625f,
    .maxFloat = 2147483648.0f,
    .desc = "Sun luminance",
};

ConVar cv_r_sun_res =
{
    .type = cvart_int,
    .name = "r_sun_res",
#if _DEBUG
    .value = "16",
#else
    .value = "64",
#endif // _DEBUG
    .minInt = 4,
    .maxInt = 1024,
    .desc = "Sky cubemap resolution",
};

ConVar cv_r_sun_steps =
{
    .type = cvart_int,
    .name = "r_sun_steps",
#if _DEBUG
    .value = "1",
#else
    .value = "4",
#endif // _DEBUG
    .minInt = 1,
    .maxInt = 16,
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

// ----------------------------------------------------------------------------
// exposure

ConVar cv_exp_standard =
{
    .type = cvart_bool,
    .name = "exp_standard",
    .value = "0",
    .desc = "1: Standard Calibration Exposure; 0: Saturation Based Exposure;",
};
ConVar cv_exp_manual =
{
    .type = cvart_bool,
    .name = "exp_manual",
    .value = "0",
    .desc = "1: Manual Exposure; 0: Automatic Exposure",
};
ConVar cv_exp_aperture =
{
    .type = cvart_float,
    .name = "exp_aperture",
    .value = "1.4",
    .minFloat = 1.4f,
    .maxFloat = 22.0f,
    .desc = "Exposure Aperture, in millimeters",
};
ConVar cv_exp_shutter =
{
    .type = cvart_float,
    .name = "exp_shutter",
    .value = "0.1",
    .minFloat = 0.001f,
    .maxFloat = 1.0f,
    .desc = "Exposure Shutter Time, in seconds",
};
ConVar cv_exp_adaptrate =
{
    .type = cvart_float,
    .name = "exp_adaptrate",
    .value = "1.0",
    .minFloat = 0.1f,
    .maxFloat = 10.0f,
    .desc = "Autoexposure Adaptation Rate",
};
ConVar cv_exp_evoffset =
{
    .type = cvart_float,
    .name = "exp_evoffset",
    .value = "0.0",
    .minFloat = -10.0f,
    .maxFloat = 10.0f,
    .desc = "Exposure EV compensation",
};
ConVar cv_exp_evmin =
{
    .type = cvart_float,
    .name = "exp_evmin",
    .value = "-10.0",
    .minFloat = -23.0f,
    .maxFloat = 23.0f,
    .desc = "Autoexposure Sensor Minimum",
};
ConVar cv_exp_evmax =
{
    .type = cvart_float,
    .name = "exp_evmax",
    .value = "23.0",
    .minFloat = -23.0f,
    .maxFloat = 23.0f,
    .desc = "Autoexposure Sensor Maximum",
};
ConVar cv_exp_cdfmin =
{
    .type = cvart_float,
    .name = "exp_cdfmin",
    .value = "0.1",
    .minFloat = 0.0f,
    .maxFloat = 1.0f,
    .desc = "Autoexposure Histogram Cdf Min",
};
ConVar cv_exp_cdfmax =
{
    .type = cvart_float,
    .name = "exp_cdfmax",
    .value = "0.9",
    .minFloat = 0.0f,
    .maxFloat = 1.0f,
    .desc = "Autoexposure Histogram Cdf Max",
};

// ----------------------------------------------------------------------------

ConVar cv_sky_rad_cr =
{
    .type = cvart_float,
    .flags = cvarf_logarithmic,
    .name = "sky_rad_cr",
    .value = "6360",
    .minFloat = 636.0f,
    .maxFloat = 63600.0f,
    .desc = "Sky crust radius, kilometers",
};
ConVar cv_sky_rad_at =
{
    .type = cvart_float,
    .flags = cvarf_logarithmic,
    .name = "sky_rad_at",
    .value = "60",
    .minFloat = 6.0f,
    .maxFloat = 600.0f,
    .desc = "Sky atmosphere relative radius, kilometers",
};
ConVar cv_sky_rlh_mfp =
{
    .type = cvart_point,
    .flags = cvarf_logarithmic,
    .name = "sky_rlh_mfp",
    .value = "192 82 34",
    .minFloat = 10.0f,
    .maxFloat = 1000.0f,
    .desc = "Rayleigh mean free path, kilometers",
};
ConVar cv_sky_rlh_sh =
{
    .type = cvart_float,
    .flags = cvarf_logarithmic,
    .name = "sky_rlh_sh",
    .value = "8.5",
    .minFloat = 0.1f,
    .maxFloat = 10.0f,
    .desc = "Rayleigh scale height, kilometers",
};
ConVar cv_sky_mie_mfp =
{
    .type = cvart_float,
    .flags = cvarf_logarithmic,
    .name = "sky_mie_mfp",
    .value = "48",
    .minFloat = 10.0f,
    .maxFloat = 1000.0f,
    .desc = "Mie mean free path, kilometers",
};
ConVar cv_sky_mie_sh =
{
    .type = cvart_float,
    .flags = cvarf_logarithmic,
    .name = "sky_mie_sh",
    .value = "1.2",
    .minFloat = 0.1f,
    .maxFloat = 10.0f,
    .desc = "Mie scale height, kilometers",
};
ConVar cv_sky_mie_g =
{
    .type = cvart_float,
    .name = "sky_mie_g",
    .value = "0.758",
    .minFloat = -0.99f,
    .maxFloat = 0.99f,
    .desc = "Mie mean scatter angle, cosine theta",
};

// ----------------------------------------------------------------------------

ConVar cv_lm_upload =
{
    .type = cvart_bool,
    .name = "lm_upload",
    .value = "0",
    .desc = "Upload the latest lightmap data to the GPU",
};

ConVar cv_lm_gen =
{
    .type = cvart_bool,
    .name = "lm_gen",
    .value = "0",
    .desc = "Progressively bake lightmaps every frame",
};

ConVar cv_lm_density =
{
    .type = cvart_float,
    .name = "lm_density",
    .value = DEBUG_ONLY("2") RELEASE_ONLY("4"),
    .minFloat = 0.1f,
    .maxFloat = 32.0f,
    .desc = "Lightmap baking: texels per meter [0.1, 32]",
};

ConVar cv_lm_timeslice =
{
    .type = cvart_int,
    .name = "lm_timeslice",
    .value = DEBUG_ONLY("60") RELEASE_ONLY("1"),
    .minInt = 1,
    .maxInt = 1024,
    .desc = "Lightmap baking: number of frames per sample",
};

ConVar cv_lm_spp =
{
    .type = cvart_int,
    .name = "lm_spp",
    .value = "1",
    .minInt = 1,
    .maxInt = 1024,
    .desc = "Lightmap baking: samples per pixel",
};

// ----------------------------------------------------------------------------

ConVar cv_fullscreen =
{
    .type = cvart_bool,
    .name = "fullscreen",
    .value = "0",
    .desc = "Fullscreen windowing mode",
};

// ----------------------------------------------------------------------------

void ConVars_RegisterAll(void)
{
    ConVar_Reg(&cv_basedir);
    ConVar_Reg(&cv_r_refl_gen);
    ConVar_Reg(&cv_con_logpath);
    ConVar_Reg(&cv_r_fpslimit);
    ConVar_Reg(&cv_game);
    ConVar_Reg(&cv_r_display_nits_min);
    ConVar_Reg(&cv_r_display_nits_max);
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
    ConVar_Reg(&cv_pt_dist_meters);
    ConVar_Reg(&cv_pt_normal);
    ConVar_Reg(&cv_pt_trace);
    ConVar_Reg(&cv_r_fov);
    ConVar_Reg(&cv_r_height);
    ConVar_Reg(&cv_r_scale);
    ConVar_Reg(&cv_r_sun_dir);
    ConVar_Reg(&cv_r_sun_lum);
    ConVar_Reg(&cv_r_sun_res);
    ConVar_Reg(&cv_r_sun_steps);
    ConVar_Reg(&cv_r_qlights);
    ConVar_Reg(&cv_r_width);
    ConVar_Reg(&cv_r_zfar);
    ConVar_Reg(&cv_r_znear);
    ConVar_Reg(&cv_r_tex_custom);
    ConVar_Reg(&cv_r_brdflut_spf);
    ConVar_Reg(&cv_ui_opacity);
    ConVar_Reg(&cv_r_whitepoint);
    ConVar_Reg(&cv_in_yawscale);

    ConVar_Reg(&cv_exp_standard);
    ConVar_Reg(&cv_exp_manual);
    ConVar_Reg(&cv_exp_aperture);
    ConVar_Reg(&cv_exp_shutter);
    ConVar_Reg(&cv_exp_adaptrate);
    ConVar_Reg(&cv_exp_evoffset);
    ConVar_Reg(&cv_exp_evmin);
    ConVar_Reg(&cv_exp_evmax);
    ConVar_Reg(&cv_exp_cdfmin);
    ConVar_Reg(&cv_exp_cdfmax);

    ConVar_Reg(&cv_sky_rad_cr);
    ConVar_Reg(&cv_sky_rad_at);
    ConVar_Reg(&cv_sky_rlh_mfp);
    ConVar_Reg(&cv_sky_rlh_sh);
    ConVar_Reg(&cv_sky_mie_mfp);
    ConVar_Reg(&cv_sky_mie_sh);
    ConVar_Reg(&cv_sky_mie_g);

    ConVar_Reg(&cv_fullscreen);
}
