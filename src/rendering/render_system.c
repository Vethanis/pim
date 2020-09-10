#include "rendering/render_system.h"

#include "allocator/allocator.h"
#include "threading/task.h"
#include "threading/taskcpy.h"
#include "ui/cimgui.h"

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

static cvar_t cv_pt_trace = { .type = cvart_bool,.name = "pt_trace",.value = "0",.desc = "enable path tracing" };
static cvar_t cv_pt_denoise = { .type = cvart_bool,.name = "pt_denoise",.value = "0",.desc = "denoise path tracing output" };
static cvar_t cv_pt_normal = { .type = cvart_bool,.name = "pt_normal",.value = "0",.desc = "output path tracer normals" };
static cvar_t cv_pt_albedo = { .type = cvart_bool,.name = "pt_albedo",.value = "0",.desc = "output path tracer albedo" };
static cvar_t cv_pt_lgrid_mpc = { .type = cvart_float,.name = "pt_lgrid_mpc",.value = "2",.minFloat = 0.1f,.maxFloat = 10.0f,.desc = "light grid meters per cell" };

static cvar_t cv_lm_gen = { .type = cvart_bool,.name = "lm_gen",.value = "0",.desc = "enable lightmap generation" };
static cvar_t cv_cm_gen = { .type = cvart_bool,.name = "cm_gen",.value = "0",.desc = "enable cubemap generation" };

static cvar_t cv_r_sw = { .type = cvart_bool,.name = "r_sw",.value = "1",.desc = "use software renderer" };

static cvar_t cv_lm_density = { .type = cvart_float,.name = "lm_density",.value = "8",.minFloat = 0.1f,.maxFloat = 32.0f,.desc = "lightmap texels per unit" };
static cvar_t cv_lm_timeslice = { .type = cvart_int,.name = "lm_timeslice",.value = "3",.minInt = 0,.maxInt = 60,.desc = "number of frames required to add 1 lighting sample to all lightmap texels" };

static cvar_t cv_r_sun_dir = { .type = cvart_vector,.name = "r_sun_dir",.value = "0.0 0.968 0.253 0.0",.desc = "Sun Direction" };
static cvar_t cv_r_sun_col = { .type = cvart_color,.name = "r_sun_col",.value = "1 1 1 1",.desc = "Sun Color" };
static cvar_t cv_r_sun_lum = { .type = cvart_float,.name = "r_sun_lum",.value = "12.0",.minFloat = -20.0f,.maxFloat = 20.0f,.desc = "Log2 Sun Luminance" };

static cvar_t cv_r_qlights = { .type = cvart_bool,.name = "r_qlights",.value = "0",.desc = "Load quake light entities" };

static void RegCVars(void)
{
    cvar_reg(&cv_r_sw);

    cvar_reg(&cv_pt_trace);
    cvar_reg(&cv_pt_denoise);
    cvar_reg(&cv_pt_normal);
    cvar_reg(&cv_pt_albedo);
    cvar_reg(&cv_pt_lgrid_mpc);

    cvar_reg(&cv_lm_gen);
    cvar_reg(&cv_lm_density);
    cvar_reg(&cv_lm_timeslice);

    cvar_reg(&cv_cm_gen);

    cvar_reg(&cv_r_sun_dir);
    cvar_reg(&cv_r_sun_col);
    cvar_reg(&cv_r_sun_lum);

    cvar_reg(&cv_r_qlights);
}

// ----------------------------------------------------------------------------

static meshid_t GenSphereMesh(i32 steps);
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

static TonemapId ms_tonemapper = TMap_ACES;
static float4 ms_toneParams;
static float4 ms_clearColor;
static exposure_t ms_exposure =
{
    .manual = false,
    .standard = true,

    .aperture = 4.0f,
    .shutterTime = 1.0f / 10.0f,
    .ISO = 100.0f,

    .adaptRate = 1.0f,
    .offsetEV = 0.0f,
};

static camera_t ms_ptcam;
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

    LightmapShutdown();
    lmpack_t pack = lmpack_pack(ms_ptscene, 1024, cvar_get_float(&cv_lm_density), 0.1f, 15.0f);
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
            cmdstat_t status = cmd_exec(cmd);
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

        float timeslice = 1.0f / i1_max(1, cv_lm_timeslice.asInt);
        lmpack_bake(ms_ptscene, timeslice);

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

        guid_t skyname = guid_str("sky", guid_seed);
        cubemaps_t* maps = Cubemaps_Get();
        float weight = 1.0f / ++ms_cmapSampleCount;
        for (i32 i = 0; i < maps->count; ++i)
        {
            cubemap_t* cubemap = maps->cubemaps + i;
            box_t bounds = maps->bounds[i];
            guid_t name = maps->names[i];
            if (!guid_eq(name, skyname))
            {
                Cubemap_Bake(cubemap, ms_ptscene, box_center(bounds), weight);
            }
            Cubemap_Convolve(cubemap, 256, weight);
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
        EnsurePtScene();

        {
            camera_t camera;
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

        const int2 size = { kDrawWidth, kDrawHeight };
        ms_trace.sampleWeight = 1.0f / ++ms_ptSampleCount;
        if (!ms_trace.color)
        {
            pt_trace_new(&ms_trace, ms_ptscene, &ms_ptcam, size);
        }
        pt_trace(&ms_trace);

        float3* pim_noalias output3 = ms_trace.color;
        if (cvar_get_bool(&cv_pt_denoise))
        {
            output3 = tmp_malloc(sizeof(output3[0]) * kDrawPixels);

            bool denoised = Denoise(
                DenoiseType_Image,
                size,
                ms_trace.color,
                ms_trace.albedo,
                ms_trace.normal,
                output3);

            if (!denoised)
            {
                cvar_set_bool(&cv_pt_denoise, false);
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

            const i32 len = ms_trace.imageSize.x * ms_trace.imageSize.y;
            for (i32 i = 0; i < len; ++i)
            {
                output4[i] = f3_f4(f3_unorm(input3[i]), 1.0f);
            }
        }

        if (output3)
        {
            ProfileBegin(pm_ptBlit);
            blit_3to4(size, GetFrontBuf()->light, output3);
            ProfileEnd(pm_ptBlit);
        }

        ProfileEnd(pm_PathTrace);
        return true;
    }
    return false;
}

ProfileMark(pm_Rasterize, Rasterize)
static void Rasterize(void)
{
    if (cvar_get_bool(&cv_r_sw))
    {
        ProfileBegin(pm_Rasterize);

        framebuf_t* frontBuf = GetFrontBuf();
        framebuf_t* backBuf = GetBackBuf();

        camera_t camera;
        camera_get(&camera);

        RtcDraw(frontBuf, &camera);

        ProfileEnd(pm_Rasterize);
    }
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
        con_exec("screenshot");
    }
}

ProfileMark(pm_Present, Present)
static bool Present(void)
{
    bool sw_present = false;
    if (cvar_get_bool(&cv_r_sw))
    {
        ProfileBegin(pm_Present);
        sw_present = true;
        framebuf_t* frontBuf = GetFrontBuf();
        int2 size = { frontBuf->width, frontBuf->height };
        ms_exposure.deltaTime = (float)time_dtf();
        ExposeImage(size, frontBuf->light, &ms_exposure);
        ResolveTile(frontBuf, ms_tonemapper, ms_toneParams);
        TakeScreenshot();
        ProfileEnd(pm_Present);
    }
    return sw_present;
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

    char mapname[PIM_PATH] = { 0 };
    SPrintf(ARGS(mapname), "maps/%s.bsp", name);
    con_logf(LogSev_Info, "cmd", "mapload is loading '%s'.", mapname);

    bool loadlights = cvar_get_bool(&cv_r_qlights);
    guid_t guid = guid_str(mapname, guid_seed);

    bool loaded = drawables_load(drawables_get(), guid);
    if (loaded)
    {
        lmpack_load(lmpack_get(), guid);
    }
    else
    {
        loaded = LoadModelAsDrawables(mapname, loadlights);
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

    char mapname[PIM_PATH] = { 0 };
    SPrintf(ARGS(mapname), "maps/%s.bsp", name);

    con_logf(LogSev_Info, "cmd", "mapsave is saving '%s'.", mapname);

    guid_t guid = guid_str(mapname, guid_seed);

    bool saved = drawables_save(drawables_get(), guid);
    if (saved)
    {
        con_logf(LogSev_Info, "cmd", "mapsave saved '%s' drawables.", mapname);
    }
    else
    {
        con_logf(LogSev_Error, "cmd", "mapsave failed to saved '%s' drawables.", mapname);
    }

    if (saved)
    {
        saved = lmpack_save(lmpack_get(), guid);
        if (saved)
        {
            con_logf(LogSev_Info, "cmd", "mapsave saved '%s' lightmaps.", mapname);
        }
        else
        {
            con_logf(LogSev_Error, "cmd", "mapsave failed to saved '%s' lightmaps.", mapname);
        }
    }

    return saved ? cmdstat_ok : cmdstat_err;
}

typedef struct task_BakeSky
{
    task_t task;
    cubemap_t* cm;
    float3 sunDir;
    float3 sunRad;
    i32 steps;
} task_BakeSky;

static void BakeSkyFn(task_t* pbase, i32 begin, i32 end)
{
    task_BakeSky* task = (task_BakeSky*)pbase;
    cubemap_t* pim_noalias cm = task->cm;
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

    guid_t skyname = guid_str("sky", guid_seed);
    cubemaps_t* maps = Cubemaps_Get();
    i32 iSky = Cubemaps_Find(maps, skyname);
    if (iSky == -1)
    {
        dirty = true;
        iSky = Cubemaps_Add(maps, skyname, 64, (box_t) { 0 });
    }

    dirty |= cvar_check_dirty(&cv_r_sun_dir);
    dirty |= cvar_check_dirty(&cv_r_sun_col);
    dirty |= cvar_check_dirty(&cv_r_sun_lum);

    if (dirty)
    {
        float4 sunDir = cvar_get_vec(&cv_r_sun_dir);
        float4 sunCol = cvar_get_vec(&cv_r_sun_col);
        float log2lum = cvar_get_float(&cv_r_sun_lum);
        float lum = exp2f(log2lum);
        cubemap_t* cm = maps->cubemaps + iSky;
        i32 size = cm->size;

        task_BakeSky* task = tmp_calloc(sizeof(*task));
        task->cm = cm;
        task->sunDir = f4_f3(sunDir);
        task->sunRad = f4_f3(f4_mulvs(sunCol, lum));
        task->steps = 64;
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

    vkr_init(1920, 1080);

    texture_sys_init();
    mesh_sys_init();

    framebuf_create(GetFrontBuf(), kDrawWidth, kDrawHeight);
    framebuf_create(GetBackBuf(), kDrawWidth, kDrawHeight);
    pt_sys_init();
    RtcDrawInit();

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
    pt_sys_update();

    BakeSky();
    Lightmap_Trace();
    Cubemap_Trace();
    if (!PathTrace())
    {
        Rasterize();
    }
    Present();

    vkr_update();

    Denoise_Evict();

    ProfileEnd(pm_update);
}

void render_sys_shutdown(void)
{
    RtcDrawShutdown();

    ShutdownPtScene();

    pt_sys_shutdown();
    framebuf_destroy(GetFrontBuf());
    framebuf_destroy(GetBackBuf());

    mesh_sys_shutdown();
    texture_sys_shutdown();

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

    if (igBegin("RenderSystem", pEnabled, 0))
    {
        if (igCollapsingHeader1("Tonemapping"))
        {
            igIndent(0.0f);
            igComboStr_arr("Operator", (i32*)&ms_tonemapper, Tonemap_Names(), TMap_COUNT);
            if (ms_tonemapper == TMap_Hable)
            {
                igSliderFloat("Shoulder Strength", &ms_toneParams.x, 0.01f, 0.99f);
                igSliderFloat("Linear Strength", &ms_toneParams.y, 0.01f, 0.99f);
                igSliderFloat("Linear Angle", &ms_toneParams.z, 0.01f, 0.99f);
                igSliderFloat("Toe Strength", &ms_toneParams.w, 0.01f, 0.99f);
            }
            igUnindent(0.0f);
        }

        if (igCollapsingHeader1("Exposure"))
        {
            igIndent(0.0f);
            igCheckbox("Manual", &ms_exposure.manual);
            igCheckbox("Standard", &ms_exposure.standard);
            igSliderFloat("Output Offset EV", &ms_exposure.offsetEV, -10.0f, 10.0f);
            if (ms_exposure.manual)
            {
                igSliderFloat("Aperture", &ms_exposure.aperture, 1.4f, 22.0f);
                igSliderFloat("Shutter Speed", &ms_exposure.shutterTime, 1.0f / 2000.0f, 1.0f);
                float S = log2f(ms_exposure.ISO / 100.0f);
                igSliderFloat("log2(ISO)", &S, 0.0f, 10.0f);
                ms_exposure.ISO = exp2f(S) * 100.0f;
            }
            else
            {
                igSliderFloat("Adapt Rate", &ms_exposure.adaptRate, 0.1f, 10.0f);
                igSliderFloat("Hist Cdf Min", &ms_exposure.histMinProb, 0.0f, ms_exposure.histMaxProb);
                igSliderFloat("Hist Cdf Max", &ms_exposure.histMaxProb, ms_exposure.histMinProb, 1.0f);
            }
            igUnindent(0.0f);
        }

        if (igCollapsingHeader1("Lights"))
        {
            igIndent(0.0f);

            const i32 ptLightCount = lights_pt_count();
            for (i32 i = 0; i < ptLightCount; ++i)
            {
                igPushIDInt(i);
                pt_light_t light = lights_get_pt(i);
                igColorEdit3("Radiance", &light.rad.x, hdrPicker);
                lights_set_pt(i, light);
                igPopID();
            }

            igUnindent(0.0f);
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

static meshid_t GenSphereMesh(i32 steps)
{
    char name[PIM_PATH];
    SPrintf(ARGS(name), "SphereMesh_%d", steps);
    guid_t guid = guid_str(name, guid_seed);

    meshid_t id = { 0 };
    if (mesh_find(guid, &id))
    {
        return id;
    }

    const float r = 1.0f;
    const i32 vsteps = steps;       // divisions along y axis
    const i32 hsteps = steps * 2;   // divisions along x-z plane
    const float dv = kPi / vsteps;
    const float dh = kTau / hsteps;

    const i32 maxlen = 6 * vsteps * hsteps;
    i32 length = 0;
    float4* pim_noalias positions = perm_malloc(sizeof(*positions) * maxlen);
    float4* pim_noalias normals = perm_malloc(sizeof(*normals) * maxlen);
    float4* pim_noalias uvs = perm_malloc(sizeof(*uvs) * maxlen);

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

            float4 v1 = f4_mulvs(n1, r);
            float4 v2 = f4_mulvs(n2, r);
            float4 v3 = f4_mulvs(n3, r);
            float4 v4 = f4_mulvs(n4, r);

            const i32 back = length;
            if (v == 0)
            {
                length += 3;
                ASSERT(length <= maxlen);

                positions[back + 0] = v1;
                positions[back + 1] = v3;
                positions[back + 2] = v4;

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

                positions[back + 0] = v3;
                positions[back + 1] = v1;
                positions[back + 2] = v2;

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

                positions[back + 0] = v1;
                positions[back + 1] = v2;
                positions[back + 2] = v4;

                normals[back + 0] = n1;
                normals[back + 1] = n2;
                normals[back + 2] = n4;

                uvs[back + 0] = u1;
                uvs[back + 1] = u2;
                uvs[back + 2] = u4;

                positions[back + 3] = v2;
                positions[back + 4] = v3;
                positions[back + 5] = v4;

                normals[back + 3] = n2;
                normals[back + 4] = n3;
                normals[back + 5] = n4;

                uvs[back + 3] = u2;
                uvs[back + 4] = u3;
                uvs[back + 5] = u4;
            }
        }
    }


    mesh_t mesh = { 0 };
    mesh.length = length;
    mesh.positions = positions;
    mesh.normals = normals;
    mesh.uvs = uvs;
    bool added = mesh_new(&mesh, guid, &id);
    ASSERT(added);
    return id;
}

// N = (0, 0, 1)
// centered at origin, [-0.5, 0.5]
static meshid_t GenQuadMesh(void)
{
    const char* name = "QuadMesh";
    guid_t guid = guid_str(name, guid_seed);
    meshid_t id = { 0 };
    if (mesh_find(guid, &id))
    {
        return id;
    }

    const float4 tl = { -0.5f, 0.5f, 0.0f };
    const float4 tr = { 0.5f, 0.5f, 0.0f };
    const float4 bl = { -0.5f, -0.5f, 0.0f };
    const float4 br = { 0.5f, -0.5f, 0.0f };
    const float4 N = { 0.0f, 0.0f, 1.0f };

    const i32 length = 6;
    float4* positions = perm_malloc(sizeof(positions[0]) * length);
    float4* normals = perm_malloc(sizeof(normals[0]) * length);
    float4* uvs = perm_malloc(sizeof(uvs[0]) * length);

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

    mesh_t mesh = { 0 };
    mesh.length = length;
    mesh.positions = positions;
    mesh.normals = normals;
    mesh.uvs = uvs;
    bool added = mesh_new(&mesh, guid, &id);
    ASSERT(added);
    return id;
}

static meshid_t ms_quadmesh;
static i32 CreateQuad(const char* name, float4 center, float4 forward, float4 up, float scale, float4 albedo, float4 rome)
{
    if (!mesh_exists(ms_quadmesh))
    {
        ms_quadmesh = GenQuadMesh();
    }
    mesh_retain(ms_quadmesh);

    guid_t guid = guid_str(name, guid_seed);
    drawables_t* dr = drawables_get();
    i32 i = drawables_add(dr, guid);
    dr->meshes[i] = ms_quadmesh;
    dr->translations[i] = center;
    dr->scales[i] = f4_s(scale);
    dr->rotations[i] = quat_lookat(forward, up);

    material_t mat =
    {
        .flatAlbedo = albedo,
        .flatRome = rome,
    };
    dr->materials[i] = mat;

    return i;
}

static meshid_t ms_spheremesh;
static i32 CreateSphere(const char* name, float4 center, float radius, float4 albedo, float4 rome)
{
    if (!mesh_exists(ms_spheremesh))
    {
        ms_spheremesh = GenSphereMesh(12);
    }
    mesh_retain(ms_spheremesh);

    guid_t guid = guid_str(name, guid_seed);
    drawables_t* dr = drawables_get();
    i32 i = drawables_add(dr, guid);
    dr->meshes[i] = ms_spheremesh;
    dr->translations[i] = center;
    dr->scales[i] = f4_s(radius);
    dr->rotations[i] = quat_id;

    material_t mat =
    {
        .flatAlbedo = albedo,
        .flatRome = rome,
    };
    dr->materials[i] = mat;

    return i;
}

static cmdstat_t CmdCornellBox(i32 argc, const char** argv)
{
    drawables_t* dr = drawables_get();
    drawables_clear(dr);
    ShutdownPtScene();
    LightmapShutdown();

    camera_reset();

    const float wallExtents = 5.0f;
    const float sphereRad = 0.3f;
    const float lightExtents = 0.25f;

    const float wallScale = 2.0f * wallExtents;
    const float sphereScale = 2.0f * sphereRad;
    const float lightScale = 2.0f * lightExtents;

    const float4 x = { 1.0f, 0.0f, 0.0f };
    const float4 y = { 0.0f, 1.0f, 0.0f };
    const float4 z = { 0.0f, 0.0f, 1.0f };

    const float4 white = f4_s(0.9f);
    const float4 red = f4_v(0.9f, 0.1f, 0.1f, 1.0f);
    const float4 green = f4_v(0.1f, 0.9f, 0.1f, 1.0f);
    //const float4 blue = f4_v(0.1f, 0.1f, 0.9f, 1.0f);
    const float4 boxRome = f4_v(0.9f, 1.0f, 0.0f, 0.0f);
    const float4 lightRome = f4_v(0.9f, 1.0f, 0.0f, 1.0f);
    const float4 plasticRome = f4_v(0.35f, 1.0f, 0.0f, 0.0f);
    const float4 metalRome = f4_v(0.125f, 1.0f, 1.0f, 0.0f);
    const float4 floorRome = f4_v(0.1f, 1.0f, 0.0f, 0.0f);

    CreateQuad(
        "Cornell_Floor",
        f4_mulvs(y, -wallExtents),
        f4_neg(y), f4_neg(z),
        wallScale,
        white,
        floorRome);
    CreateQuad(
        "Cornell_Ceil",
        f4_mulvs(y, wallExtents),
        y, z,
        wallScale,
        white,
        boxRome);
    CreateQuad(
        "Cornell_Light",
        f4_mulvs(y, wallExtents - 0.01f),
        y, z,
        lightScale,
        white,
        lightRome);

    CreateQuad(
        "Cornell_Left",
        f4_mulvs(x, -wallExtents),
        f4_neg(x), y,
        wallScale,
        red,
        boxRome);
    CreateQuad(
        "Cornell_Right",
        f4_mulvs(x, wallExtents),
        x, y,
        wallScale,
        green,
        boxRome);

    CreateQuad(
        "Cornell_Near",
        f4_mulvs(z, wallExtents),
        z, y,
        wallScale,
        white,
        boxRome);
    CreateQuad(
        "Cornell_Far",
        f4_mulvs(z, -wallExtents),
        f4_neg(z), y,
        wallScale,
        white,
        boxRome);

    CreateSphere(
        "Cornell_MetalSphere",
        f4_v(-sphereScale, -wallExtents + sphereScale, sphereScale, 0.0f),
        sphereScale,
        white,
        metalRome);

    CreateSphere(
        "Cornell_PlasticSphere",
        f4_v(sphereScale, -wallExtents + sphereScale, -sphereScale, 0.0f),
        sphereScale,
        white,
        plasticRome);

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
    camera_t cam;
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
    camera_t cam;
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
