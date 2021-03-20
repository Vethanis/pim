#include "rendering/render_system.h"

#include "allocator/allocator.h"
#include "assets/crate.h"
#include "threading/task.h"
#include "threading/taskcpy.h"
#include "ui/cimgui_ext.h"

#include "common/time.h"
#include "common/cvar.h"
#include "common/profiler.h"
#include "common/console.h"
#include "common/stringutil.h"
#include "common/sort.h"
#include "common/cmd.h"
#include "common/fnv1a.h"
#include "input/input_system.h"

#include "math/types.h"
#include "math/int2_funcs.h"
#include "math/float4_funcs.h"
#include "math/float3_funcs.h"
#include "math/float2_funcs.h"
#include "math/float4x4_funcs.h"
#include "math/box.h"
#include "math/color.h"
#include "math/sdf.h"
#include "math/ambcube.h"
#include "math/sampling.h"
#include "math/frustum.h"
#include "math/lighting.h"
#include "math/cubic_fit.h"
#include "math/atmosphere.h"

#include "rendering/constants.h"
#include "rendering/r_window.h"
#include "rendering/framebuffer.h"
#include "rendering/camera.h"
#include "rendering/lights.h"
#include "rendering/vertex_stage.h"
#include "rendering/resolve_tile.h"
#include "rendering/screenblit.h"
#include "rendering/path_tracer.h"
#include "rendering/cubemap.h"
#include "rendering/drawable.h"
#include "rendering/model.h"
#include "rendering/lightmap.h"
#include "rendering/denoise.h"
#include "rendering/rtcdraw.h"
#include "rendering/exposure.h"
#include "rendering/mesh.h"
#include "rendering/material.h"

#include "rendering/vulkan/vkr.h"

#include "quake/q_model.h"
#include "assets/asset_system.h"

#include <stb/stb_image_write.h>
#include <string.h>
#include <time.h>

static ConVar_t cv_pt_trace =
{
    .type = cvart_bool,
    .name = "pt_trace",
    .value = "0",
    .desc = "enable path tracing",
};

static ConVar_t cv_pt_denoise =
{
    .type = cvart_bool,
    .name = "pt_denoise",
    .value = "0",
    .desc = "denoise path tracing output",
};

static ConVar_t cv_pt_normal =
{
    .type = cvart_bool,
    .name = "pt_normal",
    .value = "0",
    .desc = "output path tracer normals",
};

static ConVar_t cv_pt_albedo =
{
    .type = cvart_bool,
    .name = "pt_albedo",
    .value = "0",
    .desc = "output path tracer albedo",
};

static ConVar_t cv_cm_gen =
{
    .type = cvart_bool,
    .name = "cm_gen",
    .value = "0",
    .desc = "enable cubemap generation",
};

static ConVar_t cv_lm_gen =
{
    .type = cvart_bool,
    .name = "lm_gen",
    .value = "0",
    .desc = "enable lightmap generation",
};

static ConVar_t cv_lm_density =
{
    .type = cvart_float,
    .name = "lm_density",
    .value = "4",
    .minFloat = 0.1f,
    .maxFloat = 32.0f,
    .desc = "lightmap texels per meter",
};

static ConVar_t cv_lm_timeslice =
{
    .type = cvart_int,
    .name = "lm_timeslice",
    .value = "1",
    .minInt = 1,
    .maxInt = 1024,
    .desc = "number of frames required to add 1 lighting sample to all lightmap texels",
};

static ConVar_t cv_lm_spp =
{
    .type = cvart_int,
    .name = "lm_spp",
    .value = "2",
    .minInt = 1,
    .maxInt = 1024,
    .desc = "lightmap samples per pixel",
};

static ConVar_t cv_r_sun_dir =
{
    .type = cvart_vector,
    .name = "r_sun_dir",
    .value = "0.0 0.968 0.253 0.0",
    .desc = "Sun Direction",
};
static ConVar_t cv_r_sun_col =
{
    .type = cvart_color,
    .name = "r_sun_col",
    .value = "1 1 1 1",
    .desc = "Sun Color",
};
static ConVar_t cv_r_sun_lum =
{
    .type = cvart_float,
    .name = "r_sun_lum",
    .value = "12.0",
    .minFloat = -20.0f,
    .maxFloat = 20.0f,
    .desc = "Log2 Sun Luminance",
};
static ConVar_t cv_r_sun_res =
{
    .type = cvart_int,
    .name = "r_sun_res",
    .value = "256",
    .minInt = 4,
    .maxInt = 4096,
    .desc = "Sky Cubemap Resolution",
};
static ConVar_t cv_r_sun_steps =
{
    .type = cvart_int,
    .name = "r_sun_steps",
    .value = "64",
    .minInt = 4,
    .maxInt = 1024,
    .desc = "Sky Cubemap Raymarch Steps",
};

static ConVar_t cv_r_qlights =
{
    .type = cvart_bool,
    .name = "r_qlights",
    .value = "0",
    .desc = "Load quake light entities",
};

static void RegCVars(void)
{
    cvar_reg(&cv_pt_trace);
    cvar_reg(&cv_pt_denoise);
    cvar_reg(&cv_pt_normal);
    cvar_reg(&cv_pt_albedo);

    cvar_reg(&cv_lm_gen);
    cvar_reg(&cv_lm_density);
    cvar_reg(&cv_lm_timeslice);
    cvar_reg(&cv_lm_spp);

    cvar_reg(&cv_cm_gen);

    cvar_reg(&cv_r_sun_dir);
    cvar_reg(&cv_r_sun_col);
    cvar_reg(&cv_r_sun_lum);
    cvar_reg(&cv_r_sun_res);
    cvar_reg(&cv_r_sun_steps);

    cvar_reg(&cv_r_qlights);
}

// ----------------------------------------------------------------------------

static cmdstat_t CmdCornellBox(i32 argc, const char** argv);
static cmdstat_t CmdTeleport(i32 argc, const char** argv);
static cmdstat_t CmdLookat(i32 argc, const char** argv);
static cmdstat_t CmdPtTest(i32 argc, const char** argv);
static cmdstat_t CmdPtStdDev(i32 argc, const char** argv);
static cmdstat_t CmdLoadTest(i32 argc, const char** argv);
static cmdstat_t CmdLoadMap(i32 argc, const char** argv);
static cmdstat_t CmdSaveMap(i32 argc, const char** argv);

// ----------------------------------------------------------------------------

static framebuf_t ms_buffers[2];
static i32 ms_iFrame;

static TonemapId ms_tonemapper = TMap_Uncharted2;
static float4 ms_toneParams;
static float4 ms_clearColor;
static vkrExposure ms_exposure =
{
    .manual = false,
    .standard = true,

    .aperture = 1.4f,
    .shutterTime = 0.1f,
    .ISO = 100.0f,

    .adaptRate = 0.3f,
    .offsetEV = 0.0f,
    .minCdf = 0.05f,
    .maxCdf = 0.95f,
    .minEV = -5.0f,
    .maxEV = 5.0f,
};

static Camera ms_ptcam;
static pt_scene_t* ms_ptscene;
static pt_trace_t ms_trace;

static i32 ms_lmSampleCount;
static i32 ms_acSampleCount;
static i32 ms_ptSampleCount;
static i32 ms_cmapSampleCount;
static i32 ms_gigridsamples;

// ----------------------------------------------------------------------------

static framebuf_t* GetFrontBuf(void)
{
    return &(ms_buffers[ms_iFrame & 1]);
}

static framebuf_t* GetBackBuf(void)
{
    return &(ms_buffers[(ms_iFrame + 1) & 1]);
}

static void SwapBuffers(void)
{
    ++ms_iFrame;
}

framebuf_t* render_sys_frontbuf(void)
{
    return GetFrontBuf();
}

framebuf_t* render_sys_backbuf(void)
{
    return GetBackBuf();
}

static void EnsureFramebuf(void)
{
    const i32 width = r_scaledwidth_get();
    const i32 height = r_scaledheight_get();
    for (i32 i = 0; i < NELEM(ms_buffers); ++i)
    {
        bool dirty = false;
        dirty |= ms_buffers[i].width != width;
        dirty |= ms_buffers[i].height != height;
        dirty |= !ms_buffers[i].color;
        if (dirty)
        {
            framebuf_destroy(&ms_buffers[i]);
            framebuf_create(&ms_buffers[i], width, height);
        }
    }
}

static void EnsurePtScene(void)
{
    if (!ms_ptscene)
    {
        ms_ptscene = pt_scene_new();
        ms_ptSampleCount = 0;
        ms_acSampleCount = 0;
        ms_cmapSampleCount = 0;
        ms_lmSampleCount = 0;
    }
}

static void EnsurePtTrace(void)
{
    EnsurePtScene();

    const i32 width = r_scaledwidth_get();
    const i32 height = r_scaledheight_get();
    bool dirty = false;
    dirty |= !ms_trace.color;
    dirty |= ms_trace.imageSize.x != width;
    dirty |= ms_trace.imageSize.y != height;
    if (dirty)
    {
        dofinfo_t dofinfo = ms_trace.dofinfo;
        pt_trace_del(&ms_trace);
        pt_trace_new(&ms_trace, ms_ptscene, i2_v(width, height));
        if (dofinfo.bladeCount)
        {
            ms_trace.dofinfo = dofinfo;
        }
    }
}

static void ShutdownPtScene(void)
{
    if (ms_ptscene)
    {
        pt_scene_del(ms_ptscene);
        ms_ptscene = NULL;
        pt_trace_del(&ms_trace);
    }
}

static void LightmapShutdown(void)
{
    lmpack_del(lmpack_get());
}

static void LightmapRepack(void)
{
    EnsurePtScene();

    lmpack_del(lmpack_get());
    lmpack_t pack = lmpack_pack(1024, cvar_get_float(&cv_lm_density), 0.1f, 15.0f);
    *lmpack_get() = pack;
}

static float CalcStdDev(const float3* pim_noalias color, int2 size)
{
    const i32 len = size.x * size.y;
    const float meanWeight = 1.0f / len;
    const float varianceWeight = 1.0f / (len - 1);
    float mean = 0.0f;
    for (i32 i = 0; i < len; ++i)
    {
        float lum = f4_perlum(f3_f4(color[i], 0.0f));
        mean += lum * meanWeight;
    }
    float variance = 0.0f;
    for (i32 i = 0; i < len; ++i)
    {
        float lum = f4_perlum(f3_f4(color[i], 0.0f));
        float err = lum - mean;
        variance += varianceWeight * (err * err);
    }
    float stddev = sqrtf(variance);
    return stddev;
}

static cmdstat_t CmdPtStdDev(i32 argc, const char** argv)
{
    const float3* color = ms_trace.color;
    int2 size = ms_trace.imageSize;
    if (color)
    {
        float stddev = CalcStdDev(color, size);
        con_logf(LogSev_Info, "pt", "StdDev: %f", stddev);
        char cmd[PIM_PATH] = { 0 };
        SPrintf(ARGS(cmd), "screenshot pt_stddev_%f.png", stddev);
        con_exec(cmd);
        return cmdstat_ok;
    }
    return cmdstat_err;
}

static cmdstat_t CmdLoadTest(i32 argc, const char** argv)
{
    char cmd[PIM_PATH];
    con_exec("mapload start");
    for (i32 e = 1; ; ++e)
    {
        for (i32 m = 1; ; ++m)
        {
            SPrintf(ARGS(cmd), "mapload e%dm%d", e, m);
            cmdstat_t status = cmd_text(cmd);
            if (status != cmdstat_ok)
            {
                if (m == 1)
                {
                    goto end;
                }
                break;
            }
        }
    }
end:
    con_exec("mapload end");
    con_exec("mapload start");
    return cmdstat_ok;
}

ProfileMark(pm_Lightmap_Trace, Lightmap_Trace)
static void Lightmap_Trace(void)
{
    static u64 s_lastUpload = 0;

    if (cvar_get_bool(&cv_lm_gen))
    {
        ProfileBegin(pm_Lightmap_Trace);
        EnsurePtScene();

        bool dirty = lmpack_get()->lmCount == 0;
        dirty |= cvar_check_dirty(&cv_lm_density);
        if (dirty)
        {
            LightmapRepack();
        }

        float timeslice = 1.0f / cvar_get_int(&cv_lm_timeslice);
        i32 spp = cvar_get_int(&cv_lm_spp);
        lmpack_bake(ms_ptscene, timeslice, spp);

        u64 now = time_now();
        if (time_sec(now - s_lastUpload) > 10.0)
        {
            s_lastUpload = now;
            lmpack_t* pack = lmpack_get();
            for (i32 i = 0; i < pack->lmCount; ++i)
            {
                lightmap_upload(&pack->lightmaps[i]);
            }
        }

        ProfileEnd(pm_Lightmap_Trace);
    }
}

ProfileMark(pm_CubemapTrace, Cubemap_Trace)
static void Cubemap_Trace(void)
{
    if (cvar_get_bool(&cv_cm_gen))
    {
        ProfileBegin(pm_CubemapTrace);
        EnsurePtScene();

        if (cvar_check_dirty(&cv_cm_gen))
        {
            ms_cmapSampleCount = 0;
        }

        Guid skyname = guid_str("sky");
        Cubemaps* maps = Cubemaps_Get();
        float weight = 1.0f / ++ms_cmapSampleCount;
        for (i32 i = 0; i < maps->count; ++i)
        {
            Cubemap* cubemap = maps->cubemaps + i;
            Box3D bounds = maps->bounds[i];
            Guid name = maps->names[i];
            if (!guid_eq(name, skyname))
            {
                Cubemap_Bake(cubemap, ms_ptscene, box_center(bounds), weight);
            }
            Cubemap_Convolve(cubemap, 64, weight);
        }

        ProfileEnd(pm_CubemapTrace);
    }
}

ProfileMark(pm_PathTrace, PathTrace)
ProfileMark(pm_ptDenoise, Denoise)
ProfileMark(pm_ptBlit, Blit)
static bool PathTrace(void)
{
    if (cvar_get_bool(&cv_pt_trace))
    {
        ProfileBegin(pm_PathTrace);
        EnsurePtTrace();

        {
            Camera camera;
            camera_get(&camera);

            bool dirty = false;
            dirty |= cvar_check_dirty(&cv_pt_trace);
            dirty |= memcmp(&camera, &ms_ptcam, sizeof(camera));

            if (dirty)
            {
                ms_ptcam = camera;
                ms_ptSampleCount = 0;
            }
        }

        ms_trace.sampleWeight = 1.0f / ++ms_ptSampleCount;
        const int2 size = ms_trace.imageSize;
        const i32 texCount = size.x * size.y;
        pt_trace(&ms_trace, &ms_ptcam);

        float3* pim_noalias output3 = ms_trace.color;
        if (cvar_get_bool(&cv_pt_denoise))
        {
            bool denoised = Denoise(
                DenoiseType_Image,
                size,
                ms_trace.color,
                ms_trace.albedo,
                ms_trace.normal,
                ms_trace.denoised);

            if (!denoised)
            {
                cvar_set_bool(&cv_pt_denoise, false);
            }
            else
            {
                output3 = ms_trace.denoised;
            }
        }
        if (cvar_get_bool(&cv_pt_albedo))
        {
            output3 = ms_trace.albedo;
        }
        if (cvar_get_bool(&cv_pt_normal))
        {
            output3 = NULL;

            const float3* pim_noalias input3 = ms_trace.normal;
            float4* pim_noalias output4 = GetFrontBuf()->light;

            for (i32 i = 0; i < texCount; ++i)
            {
                output4[i] = f3_f4(f3_unorm(input3[i]), 1.0f);
            }
        }

        if (output3)
        {
            ProfileBegin(pm_ptBlit);
            float4* pim_noalias output4 = GetFrontBuf()->light;
            for (i32 i = 0; i < texCount; ++i)
            {
                output4[i] = f3_f4(output3[i], 1.0f);
            }
            ProfileEnd(pm_ptBlit);
        }

        ProfileEnd(pm_PathTrace);
        return true;
    }
    return false;
}

static cmdstat_t CmdQuit(i32 argc, const char** argv)
{
    window_close(true);
    return cmdstat_ok;
}

static cmdstat_t CmdScreenshot(i32 argc, const char** argv)
{
    char filename[PIM_PATH] = { 0 };
    if (argc > 1 && argv[1])
    {
        StrCpy(ARGS(filename), argv[1]);
    }
    else
    {
        time_t ticks = time(NULL);
        struct tm* local = localtime(&ticks);
        char timestr[PIM_PATH] = { 0 };
        strftime(ARGS(timestr), "%Y_%m_%d_%H_%M_%S", local);
        SPrintf(ARGS(filename), "screenshot_%s.png", timestr);
    }

    const framebuf_t* buf = GetFrontBuf();
    const u32* flippedColor = buf->color;
    ASSERT(flippedColor);
    const int2 size = { buf->width, buf->height };
    const i32 stride = sizeof(flippedColor[0]) * size.x;
    const i32 len = size.x * size.y;

    u32* color = tmp_calloc(len * sizeof(color[0]));
    for (i32 y = 0; y < size.y; ++y)
    {
        i32 y2 = (size.y - y) - 1;
        for (i32 x = 0; x < size.x; ++x)
        {
            i32 i1 = y * size.x + x;
            i32 i2 = y2 * size.x + x;
            color[i2] = flippedColor[i1];
            color[i2] |= 0xff << 24;
        }
    }

    if (stbi_write_png(filename, size.x, size.y, 4, color, stride))
    {
        con_logf(LogSev_Info, "Sc", "Took screenshot '%s'", filename);
        return cmdstat_ok;
    }
    else
    {
        con_logf(LogSev_Error, "Sc", "Failed to take screenshot");
        return cmdstat_err;
    }
}

static void TakeScreenshot(void)
{
    if (input_keydown(KeyCode_F10))
    {
        if (cvar_get_bool(&cv_pt_trace))
        {
            con_exec("pt_denoise 0; wait; screenshot; wait; pt_denoise 1; wait; screenshot; wait; pt_denoise 0");
        }
        else
        {
            con_exec("screenshot");
        }
    }
    if (input_keydown(KeyCode_PageUp))
    {
        r_scale_set(f1_clamp(1.1f * r_scale_get(), 0.05f, 2.0f));
        ms_ptSampleCount = 0;
    }
    if (input_keydown(KeyCode_PageDown))
    {
        r_scale_set(f1_clamp((1.0f / 1.1f) * r_scale_get(), 0.05f, 2.0f));
        ms_ptSampleCount = 0;
    }
}

ProfileMark(pm_Present, Present)
static void Present(void)
{
    if (cvar_get_bool(&cv_pt_trace))
    {
        ProfileBegin(pm_Present);
        framebuf_t* frontBuf = GetFrontBuf();
        int2 size = { frontBuf->width, frontBuf->height };
        ExposeImage(size, frontBuf->light, &ms_exposure);
        ResolveTile(frontBuf, ms_tonemapper, ms_toneParams);
        TakeScreenshot();
        ProfileEnd(pm_Present);
    }
}

static cmdstat_t CmdLoadMap(i32 argc, const char** argv)
{
    if (argc != 2)
    {
        con_logf(LogSev_Error, "cmd", "mapload <map name>; map name is missing.");
        return cmdstat_err;
    }
    const char* name = argv[1];
    if (!name)
    {
        con_logf(LogSev_Error, "cmd", "mapload <map name>; map name is null.");
        return cmdstat_err;
    }

    con_logf(LogSev_Info, "cmd", "mapload is clearing drawables.");
    drawables_clear(drawables_get());
    ShutdownPtScene();
    LightmapShutdown();
    camera_reset();
    vkr_onunload();

    bool loaded = false;

    char mapname[PIM_PATH] = { 0 };
    SPrintf(ARGS(mapname), "maps/%s.bsp", name);
    con_logf(LogSev_Info, "cmd", "mapload is loading '%s'.", mapname);

    bool loadlights = cvar_get_bool(&cv_r_qlights);

    char cratepath[PIM_PATH] = { 0 };
    SPrintf(ARGS(cratepath), "data/%s.crate", name);
    Crate* crate = tmp_malloc(sizeof(*crate));
    if (crate_open(crate, cratepath))
    {
        loaded = true;
        loaded &= drawables_load(crate, drawables_get());
        loaded &= lmpack_load(crate, lmpack_get());
        loaded &= crate_close(crate);
    }

    if (!loaded)
    {
        loaded = LoadModelAsDrawables(mapname, drawables_get(), loadlights);
    }

    if (loaded)
    {
        drawables_updatetransforms(drawables_get());
        drawables_updatebounds(drawables_get());
        vkr_onload();
        con_logf(LogSev_Info, "cmd", "mapload loaded '%s'.", mapname);
        return cmdstat_ok;
    }
    else
    {
        con_logf(LogSev_Error, "cmd", "mapload failed to load '%s'.", mapname);
        return cmdstat_err;
    }
}

static cmdstat_t CmdSaveMap(i32 argc, const char** argv)
{
    if (argc != 2)
    {
        con_logf(LogSev_Error, "cmd", "mapsave <map name>; map name is missing.");
        return cmdstat_err;
    }
    const char* name = argv[1];
    if (!name)
    {
        con_logf(LogSev_Error, "cmd", "mapsave <map name>; map name is null.");
        return cmdstat_err;
    }

    bool saved = false;

    char mapname[PIM_PATH] = { 0 };
    SPrintf(ARGS(mapname), "maps/%s.bsp", name);

    con_logf(LogSev_Info, "cmd", "mapsave is saving '%s'.", mapname);

    char cratepath[PIM_PATH] = { 0 };
    SPrintf(ARGS(cratepath), "data/%s.crate", name);
    Crate* crate = tmp_malloc(sizeof(*crate));
    if (crate_open(crate, cratepath))
    {
        saved = true;
        saved &= drawables_save(crate, drawables_get());
        saved &= lmpack_save(crate, lmpack_get());
        saved &= crate_close(crate);
    }

    if (saved)
    {
        con_logf(LogSev_Info, "cmd", "mapsave saved '%s'.", mapname);
    }
    else
    {
        con_logf(LogSev_Error, "cmd", "mapsave failed to save '%s'.", mapname);
    }

    return saved ? cmdstat_ok : cmdstat_err;
}

typedef struct task_BakeSky
{
    task_t task;
    Cubemap* cm;
    float3 sunDir;
    float3 sunRad;
    i32 steps;
} task_BakeSky;

static void BakeSkyFn(task_t* pbase, i32 begin, i32 end)
{
    task_BakeSky* task = (task_BakeSky*)pbase;
    Cubemap* pim_noalias cm = task->cm;
    const i32 size = cm->size;
    float3** pim_noalias faces = cm->color;
    const float3 sunDir = task->sunDir;
    const float3 sunRad = task->sunRad;
    const i32 len = size * size;
    const i32 steps = task->steps;

    for (i32 iWork = begin; iWork < end; ++iWork)
    {
        i32 iFace = iWork / len;
        i32 iTexel = iWork % len;
        i32 x = iTexel % size;
        i32 y = iTexel / size;
        float4 rd = Cubemap_CalcDir(size, iFace, i2_v(x, y), f2_0);
        faces[iFace][iTexel] = EarthAtmosphere(f3_0, f4_f3(rd), sunDir, sunRad, steps);
    }
}

static void BakeSky(void)
{
    bool dirty = false;

    Guid skyname = guid_str("sky");
    Cubemaps* maps = Cubemaps_Get();
    if (cvar_check_dirty(&cv_r_sun_res))
    {
        Cubemaps_Rm(maps, skyname);
    }
    i32 iSky = Cubemaps_Find(maps, skyname);
    if (iSky == -1)
    {
        dirty = true;
        iSky = Cubemaps_Add(maps, skyname, cvar_get_int(&cv_r_sun_res), (Box3D) { 0 });
    }

    dirty |= cvar_check_dirty(&cv_r_sun_steps);
    dirty |= cvar_check_dirty(&cv_r_sun_dir);
    dirty |= cvar_check_dirty(&cv_r_sun_col);
    dirty |= cvar_check_dirty(&cv_r_sun_lum);

    if (dirty)
    {
        float4 sunDir = cvar_get_vec(&cv_r_sun_dir);
        float4 sunCol = cvar_get_vec(&cv_r_sun_col);
        float log2lum = cvar_get_float(&cv_r_sun_lum);
        float lum = exp2f(log2lum);
        Cubemap* cm = &maps->cubemaps[iSky];
        i32 size = cm->size;

        task_BakeSky* task = tmp_calloc(sizeof(*task));
        task->cm = cm;
        task->sunDir = f4_f3(sunDir);
        task->sunRad = f4_f3(f4_mulvs(sunCol, lum));
        task->steps = cvar_get_int(&cv_r_sun_steps);
        task_run(&task->task, BakeSkyFn, Cubeface_COUNT * size * size);
    }
}

void render_sys_init(void)
{
    ms_iFrame = 0;
    RegCVars();

    cmd_reg("screenshot", CmdScreenshot);
    cmd_reg("mapload", CmdLoadMap);
    cmd_reg("mapsave", CmdSaveMap);
    cmd_reg("cornell_box", CmdCornellBox);
    cmd_reg("teleport", CmdTeleport);
    cmd_reg("lookat", CmdLookat);
    cmd_reg("quit", CmdQuit);
    cmd_reg("pt_test", CmdPtTest);
    cmd_reg("pt_stddev", CmdPtStdDev);
    cmd_reg("loadtest", CmdLoadTest);

    vkr_init();
    g_vkr.exposurePass.params = ms_exposure;

    texture_sys_init();
    mesh_sys_init();
    model_sys_init();
    pt_sys_init();
    drawables_init();
    EnsureFramebuf();

    ms_toneParams.x = 0.3f; // shoulder
    ms_toneParams.y = 0.5f; // linear str
    ms_toneParams.z = 0.15f; // linear ang
    ms_toneParams.w = 0.3f; // toe
    ms_clearColor = f4_v(0.01f, 0.012f, 0.022f, 0.0f);

    con_exec("mapload start");
}

ProfileMark(pm_update, render_sys_update)
void render_sys_update(void)
{
    ProfileBegin(pm_update);

    EnsureFramebuf();
    SwapBuffers();
    drawables_updatetransforms(drawables_get());

    if (input_keydown(KeyCode_F9))
    {
        cvar_toggle(&cv_pt_trace);
    }
    if (input_keydown(KeyCode_F8))
    {
        cvar_toggle(&cv_pt_denoise);
    }

    texture_sys_update();
    mesh_sys_update();
    model_sys_update();
    pt_sys_update();
    drawables_update();

    BakeSky();
    Lightmap_Trace();
    Cubemap_Trace();
    PathTrace();
    Present();

    vkr_update();

    Denoise_Evict();

    ProfileEnd(pm_update);
}

void render_sys_shutdown(void)
{
    ShutdownPtScene();

    drawables_shutdown();
    pt_sys_shutdown();
    framebuf_destroy(GetFrontBuf());
    framebuf_destroy(GetBackBuf());

    texture_sys_shutdown();
    mesh_sys_shutdown();
    model_sys_shutdown();

    vkr_shutdown();
}

static i32 CmpFloat(const void* lhs, const void* rhs, void* usr)
{
    const float a = *(float*)lhs;
    const float b = *(float*)rhs;
    if (a != b)
    {
        return a < b ? -1 : 1;
    }
    return 0;
}

ProfileMark(pm_gui, render_sys_gui)
void render_sys_gui(bool* pEnabled)
{
    ProfileBegin(pm_gui);

    const u32 hdrPicker = ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_InputRGB;
    const u32 ldrPicker = ImGuiColorEditFlags_Float | ImGuiColorEditFlags_InputRGB;

    if (igBegin("RenderSystem", pEnabled, 0x0))
    {
        if (igTreeNodeExStr("Tonemapping", ImGuiTreeNodeFlags_Framed))
        {
            igComboStr_arr("Operator", (i32*)&ms_tonemapper, Tonemap_Names(), TMap_COUNT, -1);
            if (ms_tonemapper == TMap_Hable)
            {
                igExSliderFloat("Shoulder Strength", &ms_toneParams.x, 0.01f, 0.99f);
                igExSliderFloat("Linear Strength", &ms_toneParams.y, 0.01f, 0.99f);
                igExSliderFloat("Linear Angle", &ms_toneParams.z, 0.01f, 0.99f);
                igExSliderFloat("Toe Strength", &ms_toneParams.w, 0.01f, 0.99f);
            }
            igTreePop();
        }

        if (igTreeNodeExStr("Exposure", ImGuiTreeNodeFlags_Framed))
        {
            bool r_sw = cvar_get_bool(&cv_pt_trace);
            vkrExposure* exposure = r_sw ? &ms_exposure : &g_vkr.exposurePass.params;

            bool manual = exposure->manual;
            bool standard = exposure->standard;
            igCheckbox("Manual", &manual);
            igCheckbox("Standard", &standard);
            exposure->manual = manual;
            exposure->standard = standard;

            igExSliderFloat("Output Offset EV", &exposure->offsetEV, -10.0f, 10.0f);
            if (exposure->manual)
            {
                igExSliderFloat("Aperture", &exposure->aperture, 1.4f, 22.0f);
                igExSliderFloat("Shutter Speed", &exposure->shutterTime, 1.0f / 2000.0f, 1.0f);
                float S = log2f(exposure->ISO / 100.0f);
                igExSliderFloat("log2(ISO)", &S, 0.0f, 10.0f);
                exposure->ISO = exp2f(S) * 100.0f;
            }
            else
            {
                igExSliderFloat("Adapt Rate", &exposure->adaptRate, 0.1f, 10.0f);
                igExSliderFloat("Hist Cdf Min", &exposure->minCdf, 0.0f, exposure->maxCdf - 0.01f);
                igExSliderFloat("Hist Cdf Max", &exposure->maxCdf, exposure->minCdf + 0.01f, 1.0f);
                igExSliderFloat("Min EV", &exposure->minEV, -22.0f, exposure->maxEV - 0.1f);
                igExSliderFloat("Max EV", &exposure->maxEV, exposure->minEV + 0.1f, 22.0f);
            }
            igTreePop();
        }

        if (ms_trace.scene)
        {
            pt_trace_gui(&ms_trace);
        }
        else if (ms_ptscene)
        {
            pt_scene_gui(ms_ptscene);
        }
    }
    igEnd();

    ProfileEnd(pm_gui);
}

// ----------------------------------------------------------------------------

static MeshId GenSphereMesh(const char* name, i32 steps)
{
    const i32 vsteps = steps;       // divisions along y axis
    const i32 hsteps = steps * 2;   // divisions along x-z plane
    const float dv = kPi / vsteps;
    const float dh = kTau / hsteps;

    const i32 maxlen = 6 * vsteps * hsteps;
    i32 length = 0;
    float4* pim_noalias positions = perm_malloc(sizeof(*positions) * maxlen);
    float4* pim_noalias normals = perm_malloc(sizeof(*normals) * maxlen);
    float4* pim_noalias uvs = perm_malloc(sizeof(*uvs) * maxlen);
    int4* pim_noalias texIndices = perm_calloc(sizeof(texIndices[0]) * maxlen);

    for (i32 v = 0; v < vsteps; ++v)
    {
        float theta1 = v * dv;
        float theta2 = (v + 1) * dv;
        float st1 = sinf(theta1);
        float ct1 = cosf(theta1);
        float st2 = sinf(theta2);
        float ct2 = cosf(theta2);

        for (i32 h = 0; h < hsteps; ++h)
        {
            float phi1 = h * dh;
            float phi2 = (h + 1) * dh;
            float sp1 = sinf(phi1);
            float cp1 = cosf(phi1);
            float sp2 = sinf(phi2);
            float cp2 = cosf(phi2);

            // p2  p1
            // |   |
            // 2---1 -- t1
            // |\  |
            // | \ |
            // 3---4 -- t2

            float4 u1 = f4_v(phi1 / kTau, 1.0f - theta1 / kPi, 0.0f, 0.0f);
            float4 u2 = f4_v(phi2 / kTau, 1.0f - theta1 / kPi, 0.0f, 0.0f);
            float4 u3 = f4_v(phi2 / kTau, 1.0f - theta2 / kPi, 0.0f, 0.0f);
            float4 u4 = f4_v(phi1 / kTau, 1.0f - theta2 / kPi, 0.0f, 0.0f);

            float4 n1 = f4_v(st1 * cp1, ct1, st1 * sp1, 0.0f);
            float4 n2 = f4_v(st1 * cp2, ct1, st1 * sp2, 0.0f);
            float4 n3 = f4_v(st2 * cp2, ct2, st2 * sp2, 0.0f);
            float4 n4 = f4_v(st2 * cp1, ct2, st2 * sp1, 0.0f);

            const i32 back = length;
            if (v == 0)
            {
                length += 3;
                ASSERT(length <= maxlen);

                positions[back + 0] = n1;
                positions[back + 1] = n3;
                positions[back + 2] = n4;

                normals[back + 0] = n1;
                normals[back + 1] = n3;
                normals[back + 2] = n4;

                uvs[back + 0] = u1;
                uvs[back + 1] = u3;
                uvs[back + 2] = u4;
            }
            else if ((v + 1) == vsteps)
            {
                length += 3;
                ASSERT(length <= maxlen);

                positions[back + 0] = n3;
                positions[back + 1] = n1;
                positions[back + 2] = n2;

                normals[back + 0] = n3;
                normals[back + 1] = n1;
                normals[back + 2] = n2;

                uvs[back + 0] = u3;
                uvs[back + 1] = u1;
                uvs[back + 2] = u2;
            }
            else
            {
                length += 6;
                ASSERT(length <= maxlen);

                positions[back + 0] = n1;
                positions[back + 1] = n2;
                positions[back + 2] = n4;

                normals[back + 0] = n1;
                normals[back + 1] = n2;
                normals[back + 2] = n4;

                uvs[back + 0] = u1;
                uvs[back + 1] = u2;
                uvs[back + 2] = u4;

                positions[back + 3] = n2;
                positions[back + 4] = n3;
                positions[back + 5] = n4;

                normals[back + 3] = n2;
                normals[back + 4] = n3;
                normals[back + 5] = n4;

                uvs[back + 3] = u2;
                uvs[back + 4] = u3;
                uvs[back + 5] = u4;
            }
        }
    }

    MeshId id = { 0 };
    Mesh mesh = { 0 };
    mesh.length = length;
    mesh.positions = positions;
    mesh.normals = normals;
    mesh.uvs = uvs;
    mesh.texIndices = texIndices;
    bool added = mesh_new(&mesh, guid_str(name), &id);
    ASSERT(added);
    return id;
}

// N = (0, 0, 1)
// centered at origin, [-0.5, 0.5]
static MeshId GenQuadMesh(const char* name)
{
    const float4 tl = { -0.5f, 0.5f, 0.0f };
    const float4 tr = { 0.5f, 0.5f, 0.0f };
    const float4 bl = { -0.5f, -0.5f, 0.0f };
    const float4 br = { 0.5f, -0.5f, 0.0f };
    const float4 N = { 0.0f, 0.0f, 1.0f };

    const i32 length = 6;
    float4* positions = perm_malloc(sizeof(positions[0]) * length);
    float4* normals = perm_malloc(sizeof(normals[0]) * length);
    float4* uvs = perm_malloc(sizeof(uvs[0]) * length);
    int4* texIndices = perm_calloc(sizeof(texIndices[0]) * length);

    // counter clockwise
    positions[0] = tl; uvs[0] = f4_v(0.0f, 1.0f, 0.0f, 0.0f);
    positions[1] = bl; uvs[1] = f4_v(0.0f, 0.0f, 0.0f, 0.0f);
    positions[2] = tr; uvs[2] = f4_v(1.0f, 1.0f, 0.0f, 0.0f);
    positions[3] = tr; uvs[3] = f4_v(1.0f, 1.0f, 0.0f, 0.0f);
    positions[4] = bl; uvs[4] = f4_v(0.0f, 0.0f, 0.0f, 0.0f);
    positions[5] = br; uvs[5] = f4_v(1.0f, 0.0f, 0.0f, 0.0f);
    for (i32 i = 0; i < length; ++i)
    {
        normals[i] = N;
    }

    MeshId id = { 0 };
    Mesh mesh = { 0 };
    mesh.length = length;
    mesh.positions = positions;
    mesh.normals = normals;
    mesh.uvs = uvs;
    mesh.texIndices = texIndices;
    Guid guid = guid_str(name);
    bool added = mesh_new(&mesh, guid, &id);
    ASSERT(added);
    return id;
}

static TextureId GenFlatTexture(const char* name, const char* suffix, float4 value)
{
    TextureId id = { 0 };
    char fullname[PIM_PATH];
    SPrintf(ARGS(fullname), "%s_%s", name, suffix);
    Texture tex = { 0 };
    tex.size = i2_1;
    tex.texels = tex_malloc(sizeof(u32));
    *(u32*)tex.texels = LinearToColor(value);
    texture_new(&tex, VK_FORMAT_R8G8B8A8_SRGB, guid_str(fullname), &id);
    return id;
}

static Material GenMaterial(
    const char* name,
    float4 albedo,
    float4 rome,
    MatFlag flags)
{
    Material mat = { 0 };
    mat.ior = 1.0f;
    mat.albedo = GenFlatTexture(name, "albedo", albedo);
    mat.rome = GenFlatTexture(name, "rome", rome);
    if (rome.w > 0.0f)
    {
        mat.flags |= matflag_emissive;
    }
    mat.flags |= flags;
    return mat;
}

static i32 CreateQuad(
    const char* name,
    float4 center,
    float4 forward,
    float4 up,
    float scale,
    float4 albedo,
    float4 rome,
    MatFlag flags)
{
    Entities* dr = drawables_get();
    i32 i = drawables_add(dr, guid_str(name));
    dr->meshes[i] = GenQuadMesh(name);
    dr->materials[i] = GenMaterial(name, albedo, rome, flags);
    float4x4 localToWorld = f4x4_trs(center, quat_lookat(forward, up), f4_s(scale));
    mesh_settransform(dr->meshes[i], localToWorld);
    mesh_setmaterial(dr->meshes[i], &dr->materials[i]);
    return i;
}

static i32 CreateSphere(
    const char* name,
    float4 center,
    float scale,
    float4 albedo,
    float4 rome,
    MatFlag flags)
{
    Entities* dr = drawables_get();
    i32 i = drawables_add(dr, guid_str(name));
    dr->meshes[i] = GenSphereMesh(name, 24);
    dr->materials[i] = GenMaterial(name, albedo, rome, flags);
    float4x4 localToWorld = f4x4_trs(center, quat_id, f4_s(scale * 0.5f));
    mesh_settransform(dr->meshes[i], localToWorld);
    mesh_setmaterial(dr->meshes[i], &dr->materials[i]);
    return i;
}

static cmdstat_t CmdCornellBox(i32 argc, const char** argv)
{
    Entities* dr = drawables_get();
    drawables_clear(dr);
    ShutdownPtScene();
    LightmapShutdown();

    camera_reset();

    const float wallExtents = 5.0f;
    const float wallScale = 2.0f * wallExtents;
    const float sphereScale = 1.0f;
    const float lightScale = 0.5f;

    const float4 x = { 1.0f, 0.0f, 0.0f };
    const float4 y = { 0.0f, 1.0f, 0.0f };
    const float4 z = { 0.0f, 0.0f, 1.0f };

    i32 i = 0;
    i = CreateQuad(
        "Cornell_Floor",
        f4_mulvs(y, -wallExtents),
        f4_neg(y), f4_neg(z),
        wallScale,
        f4_s(0.9f),
        f4_v(0.1f, 1.0f, 0.0f, 0.0f),
        0x0);
    i = CreateQuad(
        "Cornell_Ceil",
        f4_mulvs(y, wallExtents),
        y, z,
        wallScale,
        f4_s(0.9f),
        f4_v(0.9f, 1.0f, 0.0f, 0.0f),
        0x0);
    i = CreateQuad(
        "Cornell_Light",
        f4_mulvs(y, wallExtents - 0.01f),
        y, z,
        lightScale,
        f4_s(0.9f),
        f4_v(0.9f, 1.0f, 0.0f, 1.0f),
        matflag_sky);

    i = CreateQuad(
        "Cornell_Left",
        f4_mulvs(x, -wallExtents),
        f4_neg(x), y,
        wallScale,
        f4_v(0.9f, 0.1f, 0.1f, 1.0f),
        f4_v(0.9f, 1.0f, 0.0f, 0.0f),
        0x0);
    i = CreateQuad(
        "Cornell_Right",
        f4_mulvs(x, wallExtents),
        x, y,
        wallScale,
        f4_v(0.1f, 0.9f, 0.1f, 1.0f),
        f4_v(0.9f, 1.0f, 0.0f, 0.0f),
        0x0);

    i = CreateQuad(
        "Cornell_Near",
        f4_mulvs(z, wallExtents),
        z, y,
        wallScale,
        f4_s(0.9f),
        f4_v(0.9f, 1.0f, 0.0f, 0.0f),
        0x0);
    i = CreateQuad(
        "Cornell_Far",
        f4_mulvs(z, -wallExtents),
        f4_neg(z), y,
        wallScale,
        f4_v(0.1f, 0.1f, 0.9f, 1.0f),
        f4_v(0.9f, 1.0f, 0.0f, 0.0f),
        0x0);

    const float margin = sphereScale * 1.5f;
    const float lo = -wallExtents + margin;
    const float hi = wallExtents - margin;
    for (i32 j = 0; j < 5; ++j)
    {
        float t = (j + 0.5f) / 5;
        float roughness = f1_lerp(0.0f, 1.0f, t);
        float x = f1_lerp(lo, hi, t);
        float y = -wallExtents + sphereScale;
        float z = lo;
        char name[PIM_PATH];
        SPrintf(ARGS(name), "Cornell_MetalSphere_%d", j);
        i = CreateSphere(
            name,
            f4_v(x, y, z, 0.0f),
            sphereScale,
            f4_s(0.9f),
            f4_v(roughness, 1.0f, 1.0f, 0.0f),
            0x0);
    }

    for (i32 j = 0; j < 5; ++j)
    {
        float t = (j + 0.5f) / 5;
        float roughness = f1_lerp(0.0f, 1.0f, t);
        float x = f1_lerp(lo, hi, t);
        float y = -wallExtents + sphereScale;
        float z = lo + margin;
        char name[PIM_PATH];
        SPrintf(ARGS(name), "Cornell_PlasticSphere_%d", j);
        i = CreateSphere(
            name,
            f4_v(x, y, z, 0.0f),
            sphereScale,
            f4_s(0.9f),
            f4_v(roughness, 1.0f, 0.0f, 0.0f),
            0x0);
    }

    for (i32 j = 0; j < 5; ++j)
    {
        float t = (j + 0.5f) / 5;
        float roughness = f1_lerp(0.0f, 1.0f, t);
        float x = f1_lerp(lo, hi, t);
        float y = -wallExtents + sphereScale;
        float z = lo + margin * 2.0f;
        char name[PIM_PATH];
        SPrintf(ARGS(name), "Cornell_GlassSphere_%d", j);
        i = CreateSphere(
            name,
            f4_v(x, y, z, 0.0f),
            sphereScale,
            f4_s(0.9f),
            f4_v(roughness, 1.0f, 0.0f, 0.0f),
            matflag_refractive);
        dr->materials[i].ior = 1.5f;
    }

    drawables_updatetransforms(dr);
    drawables_updatebounds(dr);

    return cmdstat_ok;
}

static cmdstat_t CmdTeleport(i32 argc, const char** argv)
{
    if (argc < 4)
    {
        con_logf(LogSev_Error, "cmd", "usage: teleport x y z");
        return cmdstat_err;
    }
    float x = (float)atof(argv[1]);
    float y = (float)atof(argv[2]);
    float z = (float)atof(argv[3]);
    Camera cam;
    camera_get(&cam);
    cam.position.x = x;
    cam.position.y = y;
    cam.position.z = z;
    camera_set(&cam);
    return cmdstat_ok;
}

static cmdstat_t CmdLookat(i32 argc, const char** argv)
{
    if (argc < 4)
    {
        con_logf(LogSev_Error, "cmd", "usage: lookat x y z");
        return cmdstat_err;
    }
    float x = (float)atof(argv[1]);
    float y = (float)atof(argv[2]);
    float z = (float)atof(argv[3]);
    Camera cam;
    camera_get(&cam);
    float4 ro = cam.position;
    float4 at = { x, y, z };
    float4 rd = f4_normalize3(f4_sub(at, ro));
    const float4 up = { 0.0f, 1.0f, 0.0f, 0.0f };
    quat rot = quat_lookat(rd, up);
    cam.rotation = rot;
    camera_set(&cam);
    return cmdstat_ok;
}

static cmdstat_t CmdPtTest(i32 argc, const char** argv)
{
    con_exec("cornell_box");
    con_exec("teleport -4 4 -4");
    con_exec("lookat 0 2 0");
    con_exec("pt_trace 1");
    con_exec("wait 500");
    con_exec("pt_stddev");
    con_exec("quit");
    return cmdstat_ok;
}
