#include "rendering/render_system.h"

#include "allocator/allocator.h"
#include "assets/crate.h"
#include "threading/task.h"
#include "threading/taskcpy.h"
#include "ui/cimgui_ext.h"

#include "common/time.h"
#include "common/cvars.h"
#include "common/profiler.h"
#include "common/console.h"
#include "common/stringutil.h"
#include "common/sort.h"
#include "common/cmd.h"
#include "common/fnv1a.h"
#include "input/input_system.h"
#include "io/dir.h"

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

#include "rendering/r_constants.h"
#include "rendering/r_window.h"
#include "rendering/framebuffer.h"
#include "rendering/camera.h"
#include "rendering/lights.h"
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
#include "rendering/vulkan/vkr_exposure.h"

#include "quake/q_model.h"
#include "assets/asset_system.h"

#include <stb/stb_image_write.h>
#include <string.h>
#include <time.h>

// ----------------------------------------------------------------------------

static cmdstat_t CmdQuit(i32 argc, const char** argv);
static cmdstat_t CmdScreenshot(i32 argc, const char** argv);
static cmdstat_t CmdCornellBox(i32 argc, const char** argv);
static cmdstat_t CmdTeleport(i32 argc, const char** argv);
static cmdstat_t CmdLookat(i32 argc, const char** argv);
static cmdstat_t CmdPtTest(i32 argc, const char** argv);
static cmdstat_t CmdPtStdDev(i32 argc, const char** argv);
static cmdstat_t CmdLoadTest(i32 argc, const char** argv);
static cmdstat_t CmdLoadMap(i32 argc, const char** argv);
static cmdstat_t CmdSaveMap(i32 argc, const char** argv);

// ----------------------------------------------------------------------------

static FrameBuf ms_buffers[1];
static i32 ms_iFrame;

static TonemapId ms_tonemapper = TMap_ACES;
static float4 ms_toneParams = { 0.5f, 0.5f, 0.5f, 0.5f };

static Camera ms_ptcam;
static PtScene* ms_ptscene;
static PtTrace ms_trace;

static i32 ms_lmSampleCount;
static i32 ms_acSampleCount;
static i32 ms_ptSampleCount;
static i32 ms_cmapSampleCount;
static i32 ms_gigridsamples;

// ----------------------------------------------------------------------------

static FrameBuf* GetFrontBuf(void)
{
    return &(ms_buffers[0]);
}

static void SwapBuffers(void)
{
    ++ms_iFrame;
}

FrameBuf* RenderSys_FrontBuf(void)
{
    return GetFrontBuf();
}

static void EnsureFramebuf(void)
{
    const i32 width = r_scaledwidth_get();
    const i32 height = r_scaledheight_get();
    for (i32 i = 0; i < NELEM(ms_buffers); ++i)
    {
        FrameBuf_Reserve(&ms_buffers[i], width, height);
    }
}

static void EnsurePtScene(void)
{
    if (!ms_ptscene)
    {
        ms_ptscene = PtScene_New();
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
        PtDofInfo dofinfo = ms_trace.dofinfo;
        PtTrace_Del(&ms_trace);
        PtTrace_New(&ms_trace, ms_ptscene, i2_v(width, height));
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
        PtScene_Del(ms_ptscene);
        ms_ptscene = NULL;
        PtTrace_Del(&ms_trace);
    }
}

static void LightmapShutdown(void)
{
    LmPack_Del(LmPack_Get());
}

static void LightmapRepack(void)
{
    EnsurePtScene();

    LmPack_Del(LmPack_Get());
    LmPack pack = LmPack_Pack(1024, ConVar_GetFloat(&cv_lm_density), 0.1f, 15.0f);
    *LmPack_Get() = pack;
}

ProfileMark(pm_Lightmap_Trace, Lightmap_Trace)
static void Lightmap_Trace(void)
{
    if (ConVar_GetBool(&cv_lm_gen))
    {
        ProfileBegin(pm_Lightmap_Trace);
        EnsurePtScene();

        bool dirty = LmPack_Get()->lmCount == 0;
        dirty |= ConVar_GetFloat(&cv_lm_density) != LmPack_Get()->texelsPerMeter;
        if (dirty)
        {
            LightmapRepack();
        }

        float timeslice = 1.0f / ConVar_GetInt(&cv_lm_timeslice);
        i32 spp = ConVar_GetInt(&cv_lm_spp);
        LmPack_Bake(ms_ptscene, timeslice, spp);

        static u64 s_lastUpload;
        u64 now = Time_Now();
        if (Time_Sec(now - s_lastUpload) > 10.0)
        {
            s_lastUpload = now;
            LmPack* pack = LmPack_Get();
            for (i32 i = 0; i < pack->lmCount; ++i)
            {
                Lightmap_Upload(&pack->lightmaps[i]);
            }
        }

        ProfileEnd(pm_Lightmap_Trace);
    }
}

ProfileMark(pm_CubemapTrace, Cubemap_Trace)
static void Cubemap_Trace(void)
{
    static u64 s_lap;

    if (ConVar_GetBool(&cv_r_refl_gen))
    {
        ProfileBegin(pm_CubemapTrace);
        EnsurePtScene();

        if (ConVar_CheckDirty(&cv_r_refl_gen, Time_Lap(&s_lap)))
        {
            ms_cmapSampleCount = 0;
        }

        Guid skyname = Guid_FromStr("sky");
        Cubemaps* maps = Cubemaps_Get();
        float weight = 1.0f / ++ms_cmapSampleCount;
        for (i32 i = 0; i < maps->count; ++i)
        {
            Cubemap* cubemap = maps->cubemaps + i;
            Box3D bounds = maps->bounds[i];
            Guid name = maps->names[i];
            if (!Guid_Equal(name, skyname))
            {
                Cubemap_Bake(cubemap, ms_ptscene, box_center(bounds), weight);
            }
            Cubemap_Convolve(cubemap, 64, weight);
        }

        ProfileEnd(pm_CubemapTrace);
    }
}

typedef struct TaskBlit_s
{
    Task task;
    const float3* pim_noalias src;
    float4* pim_noalias dst;
} TaskBlit;

static void TaskBlitFn(void* pbase, i32 begin, i32 end)
{
    TaskBlit* task = pbase;
    const float3* pim_noalias src = task->src;
    float4* pim_noalias dst = task->dst;
    for (i32 i = begin; i < end; ++i)
    {
        dst[i] = f3_f4(src[i], 1.0f);
    }
}
static void TaskBlitNormalFn(void* pbase, i32 begin, i32 end)
{
    TaskBlit* task = pbase;
    const float3* pim_noalias src = task->src;
    float4* pim_noalias dst = task->dst;
    for (i32 i = begin; i < end; ++i)
    {
        dst[i] = f4_unorm(f3_f4(src[i], 1.0f));
    }
}

ProfileMark(pm_PathTrace, PathTrace)
ProfileMark(pm_ptDenoise, Denoise)
ProfileMark(pm_ptBlit, Blit)
static bool PathTrace(void)
{
    static u64 s_lap;

    if (ConVar_GetBool(&cv_pt_trace))
    {
        ProfileBegin(pm_PathTrace);
        EnsurePtTrace();

        {
            Camera camera;
            Camera_Get(&camera);

            bool dirty = false;
            dirty |= ConVar_CheckDirty(&cv_pt_trace, Time_Lap(&s_lap));
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
        Pt_Trace(&ms_trace, &ms_ptcam);

        float3* pim_noalias output3 = ms_trace.color;
        if (ConVar_GetBool(&cv_pt_denoise))
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
                ConVar_SetBool(&cv_pt_denoise, false);
            }
            else
            {
                output3 = ms_trace.denoised;
            }
        }

        {
            ProfileBegin(pm_ptBlit);

            if (ConVar_GetBool(&cv_pt_albedo))
            {
                output3 = ms_trace.albedo;
            }
            if (ConVar_GetBool(&cv_pt_normal))
            {
                TaskBlit* task = Temp_Calloc(sizeof(*task));
                task->src = ms_trace.normal;
                task->dst = GetFrontBuf()->light;
                Task_Run(task, TaskBlitNormalFn, texCount);

                output3 = NULL;
            }

            if (output3)
            {
                TaskBlit* task = Temp_Calloc(sizeof(*task));
                task->src = output3;
                task->dst = GetFrontBuf()->light;
                Task_Run(task, TaskBlitFn, texCount);
            }

            ProfileEnd(pm_ptBlit);
        }

        ProfileEnd(pm_PathTrace);
        return true;
    }
    return false;
}

static void TakeScreenshot(void)
{
    if (Input_IsKeyDown(KeyCode_F10))
    {
        if (ConVar_GetBool(&cv_pt_trace))
        {
            cmd_enqueue("pt_denoise 0; wait; screenshot; wait; pt_denoise 1; wait; screenshot; wait; pt_denoise 0");
        }
        else
        {
            cmd_enqueue("screenshot");
        }
    }
    if (Input_IsKeyDown(KeyCode_PageUp))
    {
        r_scale_set(f1_clamp(r_scale_get() + 0.05f, 0.05f, 2.0f));
        ms_ptSampleCount = 0;
    }
    if (Input_IsKeyDown(KeyCode_PageDown))
    {
        r_scale_set(f1_clamp(r_scale_get() - 0.05f, 0.05f, 2.0f));
        ms_ptSampleCount = 0;
    }
}

ProfileMark(pm_Present, Present)
static void Present(void)
{
    if (ConVar_GetBool(&cv_pt_trace))
    {
        //ProfileBegin(pm_Present);
        //FrameBuf* frontBuf = GetFrontBuf();
        //int2 size = { frontBuf->width, frontBuf->height };
        //ExposeImage(size, frontBuf->light, vkrExposure_GetParams());
        //ResolveTile(frontBuf, ms_tonemapper, ms_toneParams);
        TakeScreenshot();
        //ProfileEnd(pm_Present);
    }
}

typedef struct task_BakeSky
{
    Task task;
    SkyMedium sky;
    Cubemap* cm;
    float3 sunDir;
    float3 sunLum;
    i32 steps;
} task_BakeSky;

static void BakeSkyFn(Task* pbase, i32 begin, i32 end)
{
    task_BakeSky* task = (task_BakeSky*)pbase;
    const SkyMedium* pim_noalias sky = &task->sky;
    Cubemap* pim_noalias cm = task->cm;
    const i32 size = cm->size;
    float3** pim_noalias faces = cm->color;
    const float3 sunDir = task->sunDir;
    const float3 sunLum = task->sunLum;
    const i32 len = size * size;
    const i32 steps = task->steps;

    for (i32 iWork = begin; iWork < end; ++iWork)
    {
        i32 iFace = iWork / len;
        i32 iTexel = iWork % len;
        i32 x = iTexel % size;
        i32 y = iTexel / size;
        float4 rd = Cubemap_CalcDir(size, iFace, i2_v(x, y), f2_0);
        float3 ro = f3_v(0.0f, sky->rCrust, 0.0f);
        faces[iFace][iTexel] = Atmosphere(sky, ro, f4_f3(rd), sunDir, sunLum, steps);
    }
}

static void BakeSky(void)
{
    static u64 s_lap;
    u64 lastCheck = Time_Lap(&s_lap);
    bool dirty = false;

    Guid skyname = Guid_FromStr("sky");
    Cubemaps* maps = Cubemaps_Get();
    if (ConVar_CheckDirty(&cv_r_sun_res, lastCheck))
    {
        Cubemaps_Rm(maps, skyname);
    }
    i32 iSky = Cubemaps_Find(maps, skyname);
    if (iSky < 0)
    {
        dirty = true;
        iSky = Cubemaps_Add(maps, skyname, ConVar_GetInt(&cv_r_sun_res), (Box3D) { 0 });
    }

    dirty |= ConVar_CheckDirty(&cv_r_sun_steps, lastCheck);
    dirty |= ConVar_CheckDirty(&cv_r_sun_dir, lastCheck);
    dirty |= ConVar_CheckDirty(&cv_r_sun_lum, lastCheck);
    dirty |= ConVar_CheckDirty(&cv_sky_rad_cr, lastCheck);
    dirty |= ConVar_CheckDirty(&cv_sky_rad_at, lastCheck);
    dirty |= ConVar_CheckDirty(&cv_sky_rlh_mfp, lastCheck);
    dirty |= ConVar_CheckDirty(&cv_sky_rlh_sh, lastCheck);
    dirty |= ConVar_CheckDirty(&cv_sky_mie_mfp, lastCheck);
    dirty |= ConVar_CheckDirty(&cv_sky_mie_sh, lastCheck);
    dirty |= ConVar_CheckDirty(&cv_sky_mie_g, lastCheck);

    if (dirty)
    {
        const SkyMedium sky =
        {
            .rCrust = ConVar_GetFloat(&cv_sky_rad_cr) * kKilo,
            .rAtmos = ConVar_GetFloat(&cv_sky_rad_at) * kKilo,
            .muR = f4_f3(f4_rcp(f4_mulvs(ConVar_GetVec(&cv_sky_rlh_mfp), kKilo))),
            .rhoR = 1.0f / (ConVar_GetFloat(&cv_sky_rlh_sh) * kKilo),
            .muM = 1.0f / (ConVar_GetFloat(&cv_sky_mie_mfp) * kKilo),
            .rhoM = 1.0f / (ConVar_GetFloat(&cv_sky_mie_sh) * kKilo),
            .gM = ConVar_GetFloat(&cv_sky_mie_g),
        };

        float4 sunDir = ConVar_GetVec(&cv_r_sun_dir);
        float sunLum = ConVar_GetFloat(&cv_r_sun_lum);

        Cubemap* cm = &maps->cubemaps[iSky];
        i32 size = cm->size;

        task_BakeSky* task = Temp_Calloc(sizeof(*task));
        task->sky = sky;
        task->cm = cm;
        task->sunDir = f4_f3(sunDir);
        task->sunLum = f3_s(sunLum);
        task->steps = ConVar_GetInt(&cv_r_sun_steps);
        Task_Run(&task->task, BakeSkyFn, Cubeface_COUNT * size * size);
        ms_ptSampleCount = 0;
    }
}

bool RenderSys_Init(void)
{
    ms_iFrame = 0;

    cmd_reg(
        "screenshot",
        "[<filename>]",
        "save a screenshot to the file in the screenshots folder. if no filename is specified the filename will contain a timestamp.",
        CmdScreenshot);
    cmd_reg("mapload", "<map name>", "load a map by name.", CmdLoadMap);
    cmd_reg("mapsave", "<map name>", "save the current map by name", CmdSaveMap);
    cmd_reg("cornell_box", "[<spheres|boxes>]", "(defaults to boxes) the type of primitive to use.", CmdCornellBox);
    cmd_reg("teleport", "<x> <y> <z>", "teleport to the given location.", CmdTeleport);
    cmd_reg("lookat", "<x> <y> <z>", "look at a given location.", CmdLookat);
    cmd_reg("quit", "", "quit the game.", CmdQuit);
    cmd_reg("pt_test", "", "run a path tracing test.", CmdPtTest);
    cmd_reg("pt_stddev", "", "path tracing standard deviation", CmdPtStdDev);
    cmd_reg("loadtest", "", "run a load test", CmdLoadTest);

    if (!vkrSys_Init())
    {
        Con_Logf(LogSev_Error, "vkr", "Failed to init RenderSys");
        return false;
    }

    TextureSys_Init();
    MeshSys_Init();
    ModelSys_Init();
    PtSys_Init();
    EntSys_Init();
    EnsureFramebuf();
    LightingSys_Init();

    cmd_enqueue("mapload start");

    return true;
}

bool RenderSys_WindowUpdate(void)
{
    return vkrSys_WindowUpdate();
}

ProfileMark(pm_update, RenderSys_Update)
void RenderSys_Update(void)
{
    ProfileBegin(pm_update);

    EnsureFramebuf();
    SwapBuffers();
    Entities_UpdateTransforms(Entities_Get());

    if (Input_IsKeyDown(KeyCode_F9))
    {
        ConVar_Toggle(&cv_pt_trace);
    }
    if (Input_IsKeyDown(KeyCode_F8))
    {
        ConVar_Toggle(&cv_pt_denoise);
    }

    TextureSys_Update();
    MeshSys_Update();
    ModelSys_Update();
    LightingSys_Update();
    PtSys_Update();
    EntSys_Update();

    BakeSky();
    Lightmap_Trace();
    Cubemap_Trace();
    PathTrace();
    Present();

    vkrSys_Update();

    Denoise_Evict();

    ProfileEnd(pm_update);
}

void RenderSys_Shutdown(void)
{
    LightingSys_Shutdown();
    ShutdownPtScene();

    EntSys_Shutdown();
    PtSys_Shutdown();
    FrameBuf_Del(GetFrontBuf());

    TextureSys_Shutdown();
    MeshSys_Shutdown();
    ModelSys_Shutdown();

    vkrSys_Shutdown();
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

ProfileMark(pm_gui, RenderSys_Gui)
void RenderSys_Gui(bool* pEnabled)
{
    ProfileBegin(pm_gui);

    const u32 hdrPicker = ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_InputRGB;
    const u32 ldrPicker = ImGuiColorEditFlags_Float | ImGuiColorEditFlags_InputRGB;

    if (igBegin("RenderSystem", pEnabled, 0x0))
    {
        if (igTreeNodeExStr("Exposure", ImGuiTreeNodeFlags_Framed))
        {
            igText("Average Luminance: %g", vkrGetAvgLuminance());
            igText("Min Luminance: %g", vkrGetMinLuminance());
            igText("Max Luminance: %g", vkrGetMaxLuminance());
            igText("Exposure: %g", vkrGetExposure());

            bool manual = ConVar_GetBool(&cv_exp_manual);
            bool standard = ConVar_GetBool(&cv_exp_standard);
            float offsetEV = ConVar_GetFloat(&cv_exp_evoffset);
            float aperture = ConVar_GetFloat(&cv_exp_aperture);
            float shutterTime = ConVar_GetFloat(&cv_exp_shutter);
            float adaptRate = ConVar_GetFloat(&cv_exp_adaptrate);
            float minEV = ConVar_GetFloat(&cv_exp_evmin);
            float maxEV = ConVar_GetFloat(&cv_exp_evmax);
            float minCdf = ConVar_GetFloat(&cv_exp_cdfmin);
            float maxCdf = ConVar_GetFloat(&cv_exp_cdfmax);

            igCheckbox("Manual", &manual);
            igCheckbox("Standard", &standard);

            igExSliderFloat("Output Offset EV", &offsetEV, -10.0f, 10.0f);
            if (manual)
            {
                igExSliderFloat("Aperture", &aperture, 1.4f, 22.0f);
                igExSliderFloat("Shutter Speed", &shutterTime, 1.0f / 2000.0f, 1.0f);
            }
            else
            {
                igExSliderFloat("Adapt Rate", &adaptRate, 0.1f, 10.0f);
                igExSliderFloat("Hist Cdf Min", &minCdf, 0.0f, maxCdf - 0.01f);
                igExSliderFloat("Hist Cdf Max", &maxCdf, minCdf + 0.01f, 1.0f);
                igExSliderFloat("Min EV", &minEV, -30.0f, maxEV - 0.1f);
                igExSliderFloat("Max EV", &maxEV, minEV + 0.1f, 31.0f);
            }

            ConVar_SetBool(&cv_exp_manual, manual);
            ConVar_SetBool(&cv_exp_standard, standard);
            ConVar_SetFloat(&cv_exp_evoffset, offsetEV);
            ConVar_SetFloat(&cv_exp_aperture, aperture);
            ConVar_SetFloat(&cv_exp_shutter, shutterTime);
            ConVar_SetFloat(&cv_exp_adaptrate, adaptRate);
            ConVar_SetFloat(&cv_exp_evmin, minEV);
            ConVar_SetFloat(&cv_exp_evmax, maxEV);
            ConVar_SetFloat(&cv_exp_cdfmin, minCdf);
            ConVar_SetFloat(&cv_exp_cdfmax, maxCdf);

            igTreePop();
        }

        if (igTreeNodeExStr("Color", ImGuiTreeNodeFlags_Framed))
        {
            if (igExButton("Dump Conversion Matrices"))
            {
                Color_DumpConversionMatrices();
            }
            igTreePop();
        }

        if (ms_trace.scene)
        {
            PtTrace_Gui(&ms_trace);
        }
        else if (ms_ptscene)
        {
            PtScene_Gui(ms_ptscene);
        }
    }
    igEnd();

    ProfileEnd(pm_gui);
}

// ----------------------------------------------------------------------------

static cmdstat_t CmdQuit(i32 argc, const char** argv)
{
    Window_Close(true);
    return cmdstat_ok;
}

static cmdstat_t CmdScreenshot(i32 argc, const char** argv)
{
    char filename[PIM_PATH] = { 0 };
    if (argc > 1 && argv[1])
    {
        SPrintf(ARGS(filename), "screenshots/%s.png", argv[1]);
    }
    else
    {
        time_t ticks = time(NULL);
        struct tm* local = localtime(&ticks);
        char timestr[PIM_PATH] = { 0 };
        strftime(ARGS(timestr), "%Y_%m_%d_%H_%M_%S", local);
        SPrintf(ARGS(filename), "screenshots/%s.png", timestr);
    }
    IO_MkDir("screenshots");

    const FrameBuf* buf = GetFrontBuf();
    ASSERT(buf->light);
    const int2 size = { buf->width, buf->height };
    const i32 len = size.x * size.y;
    R8G8B8A8_t* pim_noalias color = Tex_Alloc(sizeof(color[0]) * len);
    const float exposure = vkrExposure_GetParams()->exposure;
    const GTTonemapParams gtp =
    {
        .P = 1.0f,
        .a = 1.0f,
        .m = 0.5f,
        .l = 0.4f,
        .c = 1.33f,
        .b = 0.0f,
    };

    Prng rng = Prng_Get();
    float4 const *const pim_noalias light = buf->light;
    for (i32 i = 0; i < len; ++i)
    {
        float4 v = light[i];
        v = f4_mulvs(v, exposure);
        v = Color_SceneToSDR(v);
        v = f4_maxvs(v, 0.0f);
        v = f4_GTTonemap(v, gtp);
        v = f4_sRGB_InverseEOTF(v);
        v = f4_lerpvs(v, f4_rand(&rng), 1.0f / 255.0f);
        v.w = 1.0f;
        color[i] = f4_rgba8(v);
    }
    Prng_Set(rng);

    stbi_flip_vertically_on_write(1);
    i32 wrote = stbi_write_png(filename, size.x, size.y, 4, color, sizeof(color[0]) * size.x);

    Mem_Free(color); color = NULL;

    if (wrote)
    {
        Con_Logf(LogSev_Info, "Sc", "Took screenshot '%s'", filename);
        return cmdstat_ok;
    }
    else
    {
        Con_Logf(LogSev_Error, "Sc", "Failed to take screenshot");
        return cmdstat_err;
    }
}

static MeshId GenSphereMesh(void)
{
    static MeshId s_id;
    if (Mesh_Exists(s_id))
    {
        return s_id;
    }

    const i32 vsteps = 24;          // divisions along y axis
    const i32 hsteps = vsteps * 2;  // divisions along x-z plane
    const float dv = kPi / vsteps;
    const float dh = kTau / hsteps;

    const i32 maxlen = 6 * vsteps * hsteps;
    i32 length = 0;
    float4* pim_noalias positions = Perm_Alloc(sizeof(*positions) * maxlen);
    float4* pim_noalias normals = Perm_Alloc(sizeof(*normals) * maxlen);
    float4* pim_noalias uvs = Perm_Alloc(sizeof(*uvs) * maxlen);
    int4* pim_noalias texIndices = Perm_Calloc(sizeof(texIndices[0]) * maxlen);

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

    Mesh mesh = { 0 };
    mesh.length = length;
    mesh.positions = positions;
    mesh.normals = normals;
    mesh.uvs = uvs;
    mesh.texIndices = texIndices;
    bool added = Mesh_New(&mesh, Guid_FromStr("SphereMesh"), &s_id);
    ASSERT(added);
    return s_id;
}

// N = (0, 0, 1)
// centered at origin, [-0.5, 0.5]
static MeshId GenQuadMesh(void)
{
    static MeshId s_id;
    if (Mesh_Exists(s_id))
    {
        return s_id;
    }

    const float4 tl = { -0.5f, 0.5f, 0.0f };
    const float4 tr = { 0.5f, 0.5f, 0.0f };
    const float4 bl = { -0.5f, -0.5f, 0.0f };
    const float4 br = { 0.5f, -0.5f, 0.0f };
    const float4 N = { 0.0f, 0.0f, 1.0f };

    const i32 length = 6;
    float4* positions = Perm_Alloc(sizeof(positions[0]) * length);
    float4* normals = Perm_Alloc(sizeof(normals[0]) * length);
    float4* uvs = Perm_Alloc(sizeof(uvs[0]) * length);
    int4* texIndices = Perm_Calloc(sizeof(texIndices[0]) * length);

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

    Mesh mesh = { 0 };
    mesh.length = length;
    mesh.positions = positions;
    mesh.normals = normals;
    mesh.uvs = uvs;
    mesh.texIndices = texIndices;
    bool added = Mesh_New(&mesh, Guid_FromStr("QuadMesh"), &s_id);
    ASSERT(added);
    return s_id;
}

// N = (0, 0, 1)
// centered at origin, [-0.5, 0.5]
static MeshId GenBoxMesh(void)
{
    static MeshId s_id;
    if (Mesh_Exists(s_id))
    {
        return s_id;
    }

    static const float4 v[] =
    {
        {  1.0f,  1.0f, -1.0f },
        {  1.0f, -1.0f, -1.0f },
        {  1.0f,  1.0f,  1.0f },
        {  1.0f, -1.0f,  1.0f },
        { -1.0f,  1.0f, -1.0f },
        { -1.0f, -1.0f, -1.0f },
        { -1.0f,  1.0f,  1.0f },
        { -1.0f, -1.0f,  1.0f },
    };
    static const float2 vt[] =
    {
        { 0.875000f, 0.500000f, },
        { 0.625000f, 0.750000f, },
        { 0.625000f, 0.500000f, },
        { 0.375000f, 1.000000f, },
        { 0.375000f, 0.750000f, },
        { 0.625000f, 0.000000f, },
        { 0.375000f, 0.250000f, },
        { 0.375000f, 0.000000f, },
        { 0.375000f, 0.500000f, },
        { 0.125000f, 0.750000f, },
        { 0.125000f, 0.500000f, },
        { 0.625000f, 0.250000f, },
        { 0.875000f, 0.750000f, },
        { 0.625000f, 1.000000f, },
    };
    static const float4 vn[] =
    {
        {  0.0000f,  1.0000f,  0.0000f, },
        {  0.0000f,  0.0000f,  1.0000f, },
        { -1.0000f,  0.0000f,  0.0000f, },
        {  0.0000f, -1.0000f,  0.0000f, },
        {  1.0000f,  0.0000f,  0.0000f, },
        {  0.0000f,  0.0000f, -1.0000f, },
    };
    static const int3 f[] =
    {
        { 5, 1, 1 }, { 3, 2, 1 }, { 1, 3, 1 },
        { 3, 2, 2 }, { 8, 4, 2 }, { 4, 5, 2 },
        { 7, 6, 3 }, { 6, 7, 3 }, { 8, 8, 3 },
        { 2, 9, 4 }, { 8, 10, 4 }, { 6, 11, 4 },
        { 1, 3, 5 }, { 4, 5, 5 }, { 2, 9, 5 },
        { 5, 12, 6 }, { 2, 9, 6 }, { 6, 7, 6 },
        { 5, 1, 1 }, { 7, 13, 1 }, {3, 2, 1 },
        { 3, 2, 2 }, { 7, 14, 2 }, { 8, 4, 2 },
        { 7, 6, 3 }, { 5, 12, 3 }, { 6, 7, 3 },
        { 2, 9, 4 }, { 4, 5, 4 }, { 8, 10, 4 },
        { 1, 3, 5 }, { 3, 2, 5 }, { 4, 5, 5 },
        { 5, 12, 6 }, { 1, 3, 6 }, { 2, 9, 6 },
    };

    const i32 length = NELEM(f);
    float4* positions = Perm_Alloc(sizeof(positions[0]) * length);
    float4* normals = Perm_Alloc(sizeof(normals[0]) * length);
    float4* uvs = Perm_Alloc(sizeof(uvs[0]) * length);
    int4* texIndices = Perm_Calloc(sizeof(texIndices[0]) * length);

    for (i32 i = 0; i < length; ++i)
    {
        int3 ptn = f[i];
        ptn.x -= 1;
        ptn.y -= 1;
        ptn.z -= 1;
        ASSERT(ptn.x >= 0 && ptn.x < NELEM(v));
        ASSERT(ptn.y >= 0 && ptn.y < NELEM(vt));
        ASSERT(ptn.z >= 0 && ptn.z < NELEM(vn));
        float4 pos = v[ptn.x];
        float2 uv = vt[ptn.y];
        float4 normal = vn[ptn.z];
        positions[i] = f4_mulvs(pos, 0.5f);
        uvs[i] = f4_v(uv.x, uv.y, 0.0f, 0.0f);
        normals[i] = normal;
    }

    Mesh mesh = { 0 };
    mesh.length = length;
    mesh.positions = positions;
    mesh.normals = normals;
    mesh.uvs = uvs;
    mesh.texIndices = texIndices;
    bool added = Mesh_New(&mesh, Guid_FromStr("BoxMesh"), &s_id);
    ASSERT(added);
    return s_id;
}

static TextureId GenFlatTexture(const char* name, const char* suffix, float4 value)
{
    TextureId id = { 0 };
    char fullname[PIM_PATH];
    SPrintf(ARGS(fullname), "%s_%s", name, suffix);
    Texture tex = { 0 };
    tex.size = i2_1;
    R8G8B8A8_t* texels = Tex_Alloc(sizeof(texels[0]));
    texels[0] = GammaEncode_rgba8(value);
    tex.texels = texels;
    Texture_New(&tex, VK_FORMAT_R8G8B8A8_SRGB, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, Guid_FromStr(fullname), &id);
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
    albedo = Color_SDRToScene(albedo); // project color into rendering space (correct-ish)
    mat.albedo = GenFlatTexture(name, "albedo", albedo);
    mat.rome = GenFlatTexture(name, "rome", rome);
    if (rome.w > 0.0f)
    {
        mat.flags |= MatFlag_Emissive;
    }
    mat.flags |= flags;
    return mat;
}

static i32 CreateQuad(
    const char* name,
    float4 t,
    quat r,
    float4 s,
    float4 albedo,
    float4 rome,
    MatFlag flags)
{
    Entities* dr = Entities_Get();
    i32 i = Entities_Add(dr, Guid_FromStr(name));
    dr->meshes[i] = GenQuadMesh();
    dr->materials[i] = GenMaterial(name, albedo, rome, flags);
    dr->translations[i] = t;
    dr->rotations[i] = r;
    dr->scales[i] = s;
    return i;
}

static i32 CreateBox(
    const char* name,
    float4 t,
    quat r,
    float4 s,
    float4 albedo,
    float4 rome,
    MatFlag flags)
{
    Entities* dr = Entities_Get();
    i32 i = Entities_Add(dr, Guid_FromStr(name));
    dr->meshes[i] = GenBoxMesh();
    dr->materials[i] = GenMaterial(name, albedo, rome, flags);
    dr->translations[i] = t;
    dr->rotations[i] = r;
    dr->scales[i] = s;
    return i;
}

static i32 CreateSphere(
    const char* name,
    float4 t,
    quat r,
    float4 s,
    float4 albedo,
    float4 rome,
    MatFlag flags)
{
    Entities* dr = Entities_Get();
    i32 i = Entities_Add(dr, Guid_FromStr(name));
    dr->meshes[i] = GenSphereMesh();
    dr->materials[i] = GenMaterial(name, albedo, rome, flags);
    dr->translations[i] = t;
    dr->rotations[i] = r;
    dr->scales[i] = s;
    return i;
}

static cmdstat_t CmdCornellBox(i32 argc, const char** argv)
{
    Entities* dr = Entities_Get();
    Entities_Clear(dr);
    ShutdownPtScene();
    LightmapShutdown();

    Camera_Reset();

    const float wallExtents = 5.0f;
    const float4 wallScale = f4_v(2.0f * wallExtents, 2.0f * wallExtents, kDeci, 0.0f);
    const float lightScale = 1.0f;

    const float4 right = Cubemap_kForwards[Cubeface_XP];
    const float4 right_up = Cubemap_kUps[Cubeface_XP];
    const float4 left = Cubemap_kForwards[Cubeface_XM];
    const float4 left_up = Cubemap_kUps[Cubeface_XM];
    const float4 up = Cubemap_kForwards[Cubeface_YP];
    const float4 up_up = Cubemap_kUps[Cubeface_YP];
    const float4 down = Cubemap_kForwards[Cubeface_YM];
    const float4 down_up = Cubemap_kUps[Cubeface_YM];
    const float4 fwd = Cubemap_kForwards[Cubeface_ZM];
    const float4 fwd_up = Cubemap_kUps[Cubeface_ZM];
    const float4 back = Cubemap_kForwards[Cubeface_ZP];
    const float4 back_up = Cubemap_kUps[Cubeface_ZP];

    const float cHi = 0.9f;
    const float cLo = 1.0f - cHi;
    const float4 red = { cHi, cLo, cLo, 1.0f };
    const float4 green = { cLo, cHi, cLo, 1.0f };
    const float4 blue = { cLo, cLo, cHi, 1.0f };
    const float4 white = { cHi, cHi, cHi, 1.0f };
    const float4 plastic = { 0.9f, 1.0f, 0.0f, 0.0f };
    const float4 metal = { 0.1f, 1.0f, 1.0f, 0.0f };
    const float4 light = { 0.9f, 1.0f, 0.0f, 1.0f };

    i32 i = 0;
    i = CreateBox(
        "Cornell_Floor",
        f4_mulvs(down, wallExtents),
        quat_lookat(up, up_up),
        wallScale,
        white,
        plastic,
        0x0);
    i = CreateBox(
        "Cornell_Ceil",
        f4_mulvs(up, wallExtents),
        quat_lookat(down, down_up),
        wallScale,
        white,
        plastic,
        0x0);
    i = CreateBox(
        "Cornell_Light",
        f4_mulvs(up, wallExtents - kDeci * 2.0f),
        quat_lookat(down, down_up),
        f4_v(lightScale, lightScale, kDeci, 0.0f),
        f4_s(1.0f),
        light,
        0x0);

    i = CreateBox(
        "Cornell_Left",
        f4_mulvs(left, wallExtents),
        quat_lookat(right, right_up),
        wallScale,
        green,
        plastic,
        0x0);
    i = CreateBox(
        "Cornell_Right",
        f4_mulvs(right, wallExtents),
        quat_lookat(left, left_up),
        wallScale,
        red,
        plastic,
        0x0);

    i = CreateBox(
        "Cornell_Near",
        f4_mulvs(back, wallExtents),
        quat_lookat(back, back_up),
        wallScale,
        white,
        plastic,
        0x0);
    i = CreateBox(
        "Cornell_Far",
        f4_mulvs(fwd, wallExtents),
        quat_lookat(fwd, fwd_up),
        wallScale,
        blue,
        plastic,
        0x0);

    typedef enum
    {
        PrimType_Boxes,
        PrimType_Spheres,
    } PrimType;
    PrimType primType = PrimType_Boxes;
    if (1 < argc)
    {
        const char* prim = argv[1];
        if (!StrICmp(prim, PIM_PATH, "spheres"))
        {
            primType = PrimType_Spheres;
        }
        else if (!StrICmp(prim, PIM_PATH, "boxes"))
        {
            primType = PrimType_Boxes;
        }
    }

    if (primType == PrimType_Spheres)
    {
        const float sphereScale = 0.75f;
        const float margin = sphereScale * 1.5f;
        const float lo = -wallExtents + margin;
        const float hi = wallExtents - margin;

        const i32 kRows = 3;
        const i32 kColumns = 5;
        const MatFlag kMatFlags[] = { 0x0, 0x0, MatFlag_Refractive };
        const float kMetallics[] = { 1.0f, 0.0f, 0.0f };
        const float kIors[] = { 1.0f, 1.0f, 1.5f };
        ASSERT(NELEM(kMatFlags) == kRows);
        ASSERT(NELEM(kMetallics) == kRows);
        ASSERT(NELEM(kIors) == kRows);

        for (i32 iRow = 0; iRow < kRows; ++iRow)
        {
            const float tRow = (iRow + 0.5f) / kRows;
            const float z = f1_lerp(lo, hi, tRow);
            const float y = lo;

            for (i32 iCol = 0; iCol < kColumns; ++iCol)
            {
                float tCol = (iCol + 0.5f) / kColumns;
                float roughness = f1_lerp(0.0f, 1.0f, tCol);
                float x = f1_lerp(lo, hi, tCol);
                char name[PIM_PATH];
                SPrintf(ARGS(name), "Cornell_Sphere_%d_%d", iRow, iCol);
                i = CreateSphere(
                    name,
                    f4_v(x, y, z, 0.0f),
                    quat_id,
                    f4_s(sphereScale),
                    white,
                    f4_v(roughness, 1.0f, kMetallics[iRow], 0.0f),
                    kMatFlags[iRow]);
                dr->materials[i].ior = kIors[iRow];
            }
        }
    }
    else if (primType == PrimType_Boxes)
    {
        const float boxScale = 2.0f;
        const float margin = boxScale * 0.5f;
        const float lo = -wallExtents + margin;
        const float hi = wallExtents - margin;
        {
            float x = f1_lerp(lo, hi, 0.2f);
            float y = -wallExtents + boxScale;
            float z = f1_lerp(lo, hi, 0.2f);

            i = CreateBox(
                "Cornell_MetalBox",
                f4_v(x, y, z, 0.0f),
                quat_lookat(f4_normalize3(f4_v(0.2f, 0.0f, 1.0f, 0.0f)), up),
                f4_v(boxScale, boxScale * 2.0f, boxScale, 0.0f),
                white,
                metal,
                0x0);
        }
        {
            float x = f1_lerp(lo, hi, 0.8f);
            float y = -wallExtents + boxScale * 0.5f;
            float z = f1_lerp(lo, hi, 0.8f);
            i = CreateBox(
                "Cornell_PlasticBox",
                f4_v(x, y, z, 0.0f),
                quat_lookat(f4_normalize3(f4_v(-0.2f, 0.0f, 1.0f, 0.0f)), up),
                f4_v(boxScale, boxScale, boxScale, 0.0f),
                white,
                plastic,
                0x0);
        }
    }

    Entities_UpdateTransforms(dr);
    Entities_UpdateBounds(dr);

    return cmdstat_ok;
}

static cmdstat_t CmdTeleport(i32 argc, const char** argv)
{
    if (argc < 4)
    {
        Con_Logf(LogSev_Error, "cmd", "usage: teleport x y z");
        return cmdstat_err;
    }
    float x = (float)atof(argv[1]);
    float y = (float)atof(argv[2]);
    float z = (float)atof(argv[3]);
    Camera cam;
    Camera_Get(&cam);
    cam.position.x = x;
    cam.position.y = y;
    cam.position.z = z;
    Camera_Set(&cam);
    return cmdstat_ok;
}

static cmdstat_t CmdLookat(i32 argc, const char** argv)
{
    if (argc < 4)
    {
        Con_Logf(LogSev_Error, "cmd", "usage: lookat x y z");
        return cmdstat_err;
    }
    float x = (float)atof(argv[1]);
    float y = (float)atof(argv[2]);
    float z = (float)atof(argv[3]);
    Camera cam;
    Camera_Get(&cam);
    float4 ro = cam.position;
    float4 at = { x, y, z };
    float4 rd = f4_normalize3(f4_sub(at, ro));
    const float4 up = { 0.0f, 1.0f, 0.0f, 0.0f };
    quat rot = quat_lookat(rd, up);
    cam.rotation = rot;
    Camera_Set(&cam);
    return cmdstat_ok;
}

static cmdstat_t CmdPtTest(i32 argc, const char** argv)
{
    char cmd[PIM_PATH] = { 0 };

    const char* framesopt = cmd_getopt(argc, argv, "frames");
    i32 frames = 500;
    if (framesopt)
    {
        frames = ParseInt(framesopt);
        frames = i1_clamp(frames, 1, 1 << 20);
    }

    cmd_enqueue("cornell_box");
    cmd_enqueue("teleport -4 0 4");
    cmd_enqueue("lookat 0 -1 0");
    cmd_enqueue("pt_denoise 0");
    cmd_enqueue("exp_manual 1");
    cmd_enqueue("exp_evoffset 5");
    cmd_enqueue("pt_trace 1");
    SPrintf(ARGS(cmd), "wait %d", frames); cmd_enqueue(cmd);
    cmd_enqueue("pt_stddev");
    cmd_enqueue("pt_denoise 1; wait; screenshot; pt_denoise 0; pt_trace 0");
    cmd_enqueue("quit");
    return cmdstat_ok;
}

static float CalcStdDev(const float3* pim_noalias color, int2 size)
{
    const i32 len = size.x * size.y;
    const float meanWeight = 1.0f / len;
    const float varianceWeight = 1.0f / (len - 1);
    float mean = 0.0f;
    for (i32 i = 0; i < len; ++i)
    {
        float lum = f4_avglum(f3_f4(color[i], 0.0f));
        mean += lum * meanWeight;
    }
    float variance = 0.0f;
    for (i32 i = 0; i < len; ++i)
    {
        float lum = f4_avglum(f3_f4(color[i], 0.0f));
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
        Con_Logf(LogSev_Info, "pt", "StdDev: %f", stddev);
        char cmd[PIM_PATH] = { 0 };
        SPrintf(ARGS(cmd), "screenshot pt_stddev_%f", stddev);
        cmd_immediate(cmd);
        return cmdstat_ok;
    }
    return cmdstat_err;
}

static cmdstat_t CmdLoadTest(i32 argc, const char** argv)
{
    char cmd[PIM_PATH];
    char mapname[PIM_PATH];
    cmd_enqueue("mapload start; wait");
    for (i32 e = 1; ; ++e)
    {
        for (i32 m = 1; ; ++m)
        {
            SPrintf(ARGS(mapname), "maps/e%dm%d.bsp", e, m);
            asset_t asset;
            if (Asset_Get(mapname, &asset))
            {
                SPrintf(ARGS(cmd), "mapload e%dm%d; wait", e, m);
                cmd_enqueue(cmd);
            }
            else
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
    cmd_enqueue("mapload end; wait");
    cmd_enqueue("mapload start; wait");
    return cmdstat_ok;
}

static cmdstat_t CmdLoadMap(i32 argc, const char** argv)
{
    if (argc != 2)
    {
        Con_Logf(LogSev_Error, "cmd", "mapload <map name>; map name is missing.");
        return cmdstat_err;
    }
    const char* name = argv[1];
    if (!name)
    {
        Con_Logf(LogSev_Error, "cmd", "mapload <map name>; map name is null.");
        return cmdstat_err;
    }

    Con_Logf(LogSev_Info, "cmd", "mapload is clearing drawables.");
    Entities_Clear(Entities_Get());
    ShutdownPtScene();
    LightmapShutdown();
    Camera_Reset();

    bool loaded = false;

    char mapname[PIM_PATH] = { 0 };
    SPrintf(ARGS(mapname), "maps/%s.bsp", name);
    Con_Logf(LogSev_Info, "cmd", "mapload is loading '%s'.", mapname);

    char cratepath[PIM_PATH] = { 0 };
    SPrintf(ARGS(cratepath), "data/%s.crate", name);
    Crate* crate = Temp_Alloc(sizeof(*crate));
    if (Crate_Open(crate, cratepath))
    {
        loaded = true;
        loaded &= Entities_Load(crate, Entities_Get());
        loaded &= LmPack_Load(crate, LmPack_Get());
        loaded &= Crate_Close(crate);
    }

    if (!loaded)
    {
        loaded = LoadModelAsDrawables(mapname, Entities_Get());
    }

    if (loaded)
    {
        Entities_UpdateTransforms(Entities_Get());
        Entities_UpdateBounds(Entities_Get());
        Con_Logf(LogSev_Info, "cmd", "mapload loaded '%s'.", mapname);
        return cmdstat_ok;
    }
    else
    {
        Con_Logf(LogSev_Error, "cmd", "mapload failed to load '%s'.", mapname);
        return cmdstat_err;
    }
}

static cmdstat_t CmdSaveMap(i32 argc, const char** argv)
{
    if (argc != 2)
    {
        Con_Logf(LogSev_Error, "cmd", "mapsave <map name>; map name is missing.");
        return cmdstat_err;
    }
    const char* name = argv[1];
    if (!name)
    {
        Con_Logf(LogSev_Error, "cmd", "mapsave <map name>; map name is null.");
        return cmdstat_err;
    }

    bool saved = false;

    char mapname[PIM_PATH] = { 0 };
    SPrintf(ARGS(mapname), "maps/%s.bsp", name);

    Con_Logf(LogSev_Info, "cmd", "mapsave is saving '%s'.", mapname);

    char cratepath[PIM_PATH] = { 0 };
    SPrintf(ARGS(cratepath), "data/%s.crate", name);
    Crate* crate = Temp_Alloc(sizeof(*crate));
    if (Crate_Open(crate, cratepath))
    {
        saved = true;
        saved &= Entities_Save(crate, Entities_Get());
        saved &= LmPack_Save(crate, LmPack_Get());
        saved &= Crate_Close(crate);
    }

    if (saved)
    {
        Con_Logf(LogSev_Info, "cmd", "mapsave saved '%s'.", mapname);
    }
    else
    {
        Con_Logf(LogSev_Error, "cmd", "mapsave failed to save '%s'.", mapname);
    }

    return saved ? cmdstat_ok : cmdstat_err;
}
