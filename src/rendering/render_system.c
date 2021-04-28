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

static FrameBuf ms_buffers[2];
static i32 ms_iFrame;

static TonemapId ms_tonemapper = TMap_ACES;
static float4 ms_toneParams = { 0.5f, 0.5f, 0.5f, 0.5f };
// HDR appears to want 5 stops of headroom over SDR
static const float ms_hdrEvAdjust = -5.0f;
static const float ms_hdrExposureAdjust = 32.0f;

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
    return &(ms_buffers[ms_iFrame & 1]);
}

static FrameBuf* GetBackBuf(void)
{
    return &(ms_buffers[(ms_iFrame + 1) & 1]);
}

static void SwapBuffers(void)
{
    ++ms_iFrame;
}

FrameBuf* RenderSys_FrontBuf(void)
{
    return GetFrontBuf();
}

FrameBuf* RenderSys_BackBuf(void)
{
    return GetBackBuf();
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
        DofInfo dofinfo = ms_trace.dofinfo;
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
        if (ConVar_GetBool(&cv_pt_albedo))
        {
            output3 = ms_trace.albedo;
        }
        if (ConVar_GetBool(&cv_pt_normal))
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
        r_scale_set(f1_clamp(1.1f * r_scale_get(), 0.05f, 2.0f));
        ms_ptSampleCount = 0;
    }
    if (Input_IsKeyDown(KeyCode_PageDown))
    {
        r_scale_set(f1_clamp((1.0f / 1.1f) * r_scale_get(), 0.05f, 2.0f));
        ms_ptSampleCount = 0;
    }
}

ProfileMark(pm_Present, Present)
static void Present(void)
{
    if (ConVar_GetBool(&cv_pt_trace))
    {
        ProfileBegin(pm_Present);
        FrameBuf* frontBuf = GetFrontBuf();
        int2 size = { frontBuf->width, frontBuf->height };
        ExposeImage(size, frontBuf->light, vkrExposure_GetParams());
        ResolveTile(frontBuf, ms_tonemapper, ms_toneParams);
        TakeScreenshot();
        ProfileEnd(pm_Present);
    }
}

typedef struct task_BakeSky
{
    Task task;
    Cubemap* cm;
    float3 sunDir;
    float3 sunRad;
    i32 steps;
} task_BakeSky;

static void BakeSkyFn(Task* pbase, i32 begin, i32 end)
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
    if (iSky == -1)
    {
        dirty = true;
        iSky = Cubemaps_Add(maps, skyname, ConVar_GetInt(&cv_r_sun_res), (Box3D) { 0 });
    }

    dirty |= ConVar_CheckDirty(&cv_r_sun_steps, lastCheck);
    dirty |= ConVar_CheckDirty(&cv_r_sun_dir, lastCheck);
    dirty |= ConVar_CheckDirty(&cv_r_sun_col, lastCheck);
    dirty |= ConVar_CheckDirty(&cv_r_sun_lum, lastCheck);

    if (dirty)
    {
        float4 sunDir = ConVar_GetVec(&cv_r_sun_dir);
        float4 sunCol = ConVar_GetVec(&cv_r_sun_col);
        float log2lum = ConVar_GetFloat(&cv_r_sun_lum);
        float lum = exp2f(log2lum);
        Cubemap* cm = &maps->cubemaps[iSky];
        i32 size = cm->size;

        task_BakeSky* task = Temp_Calloc(sizeof(*task));
        task->cm = cm;
        task->sunDir = f4_f3(sunDir);
        task->sunRad = f4_f3(f4_mulvs(sunCol, lum));
        task->steps = ConVar_GetInt(&cv_r_sun_steps);
        Task_Run(&task->task, BakeSkyFn, Cubeface_COUNT * size * size);
    }
}

bool RenderSys_Init(void)
{
    ms_iFrame = 0;

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

    if (!vkrSys_Init())
    {
        Con_Logf(LogSev_Error, "vkr", "Failed to init RenderSys");
        return false;
    }
    if (vkrSys_HdrEnabled())
    {
        ConVar_SetFloat(&cv_exp_evoffset, ConVar_GetFloat(&cv_exp_evoffset) + ms_hdrEvAdjust);
    }

    TextureSys_Init();
    MeshSys_Init();
    ModelSys_Init();
    PtSys_Init();
    EntSys_Init();
    EnsureFramebuf();

    cmd_enqueue("mapload start");

    return true;
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
    ShutdownPtScene();

    EntSys_Shutdown();
    PtSys_Shutdown();
    FrameBuf_Del(GetFrontBuf());
    FrameBuf_Del(GetBackBuf());

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
        if (igTreeNodeExStr("Tonemapping", ImGuiTreeNodeFlags_Framed))
        {
            igComboStr_arr("Operator", (i32*)&ms_tonemapper, Tonemap_Names(), TMap_COUNT, -1);
            switch (ms_tonemapper)
            {
            default:
                break;
            case TMap_Reinhard:
                igExSliderFloat("White Point", &ms_toneParams.x, 0.1f, 10.0f);
                break;
            case TMap_Hable:
                igExSliderFloat("Shoulder Strength", &ms_toneParams.x, 0.01f, 0.99f);
                igExSliderFloat("Linear Strength", &ms_toneParams.y, 0.01f, 0.99f);
                igExSliderFloat("Linear Angle", &ms_toneParams.z, 0.01f, 0.99f);
                igExSliderFloat("Toe Strength", &ms_toneParams.w, 0.01f, 0.99f);
                break;
            }
            igTreePop();
        }

        if (igTreeNodeExStr("Exposure", ImGuiTreeNodeFlags_Framed))
        {
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
    ASSERT(buf->color);
    const int2 size = { buf->width, buf->height };
    const i32 len = size.x * size.y;
    R8G8B8A8_t* pim_noalias color = Tex_Alloc(sizeof(color[0]) * len);

    const float exposureAdjust = vkrSys_HdrEnabled() ? ms_hdrExposureAdjust : 1.0f;
    for (i32 i = 0; i < len; ++i)
    {
        float4 v = buf->light[i];
        v = f4_mulvs(v, exposureAdjust);
        v = Color_SceneToSDR(v);
        v = f4_aceskfit(v);
        v.w = 1.0f;
        color[i] = f4_rgba8(f4_sRGB_InverseEOTF_Fit(v));
    }

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

static MeshId GenSphereMesh(const char* name, i32 steps)
{
    const i32 vsteps = steps;       // divisions along y axis
    const i32 hsteps = steps * 2;   // divisions along x-z plane
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

    MeshId id = { 0 };
    Mesh mesh = { 0 };
    mesh.length = length;
    mesh.positions = positions;
    mesh.normals = normals;
    mesh.uvs = uvs;
    mesh.texIndices = texIndices;
    bool added = Mesh_New(&mesh, Guid_FromStr(name), &id);
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

    MeshId id = { 0 };
    Mesh mesh = { 0 };
    mesh.length = length;
    mesh.positions = positions;
    mesh.normals = normals;
    mesh.uvs = uvs;
    mesh.texIndices = texIndices;
    Guid guid = Guid_FromStr(name);
    bool added = Mesh_New(&mesh, guid, &id);
    ASSERT(added);
    return id;
}

// N = (0, 0, 1)
// centered at origin, [-0.5, 0.5]
static MeshId GenBoxMesh(const char* name)
{
    const float4 tl = { -0.5f, 0.5f, 0.0f };
    const float4 tr = { 0.5f, 0.5f, 0.0f };
    const float4 bl = { -0.5f, -0.5f, 0.0f };
    const float4 br = { 0.5f, -0.5f, 0.0f };

    const i32 length = 36;
    float4* positions = Perm_Alloc(sizeof(positions[0]) * length);
    float4* normals = Perm_Alloc(sizeof(normals[0]) * length);
    float4* uvs = Perm_Alloc(sizeof(uvs[0]) * length);
    int4* texIndices = Perm_Calloc(sizeof(texIndices[0]) * length);

    // counter clockwise
    float4 quadPos[6];
    quadPos[0] = tl; uvs[0] = f4_v(0.0f, 1.0f, 0.0f, 0.0f);
    quadPos[1] = bl; uvs[1] = f4_v(0.0f, 0.0f, 0.0f, 0.0f);
    quadPos[2] = tr; uvs[2] = f4_v(1.0f, 1.0f, 0.0f, 0.0f);
    quadPos[3] = tr; uvs[3] = f4_v(1.0f, 1.0f, 0.0f, 0.0f);
    quadPos[4] = bl; uvs[4] = f4_v(0.0f, 0.0f, 0.0f, 0.0f);
    quadPos[5] = br; uvs[5] = f4_v(1.0f, 0.0f, 0.0f, 0.0f);
    for (i32 i = 0; i < 6; ++i)
    {
        const float4 N = Cubemap_kForwards[i];
        const quat rot = quat_lookat(N, Cubemap_kUps[i]);
        const float4x4 M = f4x4_trs(f4_mulvs(N, -0.5f), rot, f4_1);
        for (i32 j = 0; j < 6; ++j)
        {
            positions[i * 6 + j] = f4x4_mul_pt(M, quadPos[j]);
            normals[i * 6 + j] = N;
            uvs[i * 6 + j] = uvs[j];
        }
    }

    MeshId id = { 0 };
    Mesh mesh = { 0 };
    mesh.length = length;
    mesh.positions = positions;
    mesh.normals = normals;
    mesh.uvs = uvs;
    mesh.texIndices = texIndices;
    Guid guid = Guid_FromStr(name);
    bool added = Mesh_New(&mesh, guid, &id);
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
    R8G8B8A8_t* texels = Tex_Alloc(sizeof(texels[0]));
    texels[0] = GammaEncode_rgba8(value);
    tex.texels = texels;
    Texture_New(&tex, VK_FORMAT_R8G8B8A8_SRGB, Guid_FromStr(fullname), &id);
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
    //albedo = Color_AP1_AP0(albedo); // project color from current space to widest space as a test (incorrect)
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
    float4 center,
    float4 forward,
    float4 up,
    float scale,
    float4 albedo,
    float4 rome,
    MatFlag flags)
{
    Entities* dr = Entities_Get();
    i32 i = Entities_Add(dr, Guid_FromStr(name));
    dr->meshes[i] = GenQuadMesh(name);
    dr->materials[i] = GenMaterial(name, albedo, rome, flags);
    dr->translations[i] = center;
    dr->rotations[i] = quat_lookat(forward, up);
    dr->scales[i] = f4_s(scale);
    return i;
}

static i32 CreateBox(
    const char* name,
    float4 center,
    float4 forward,
    float4 up,
    float4 scale,
    float4 albedo,
    float4 rome,
    MatFlag flags)
{
    Entities* dr = Entities_Get();
    i32 i = Entities_Add(dr, Guid_FromStr(name));
    dr->meshes[i] = GenBoxMesh(name);
    dr->materials[i] = GenMaterial(name, albedo, rome, flags);
    dr->translations[i] = center;
    dr->rotations[i] = quat_lookat(forward, up);
    dr->scales[i] = scale;
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
    Entities* dr = Entities_Get();
    i32 i = Entities_Add(dr, Guid_FromStr(name));
    dr->meshes[i] = GenSphereMesh(name, 24);
    dr->materials[i] = GenMaterial(name, albedo, rome, flags);
    dr->translations[i] = center;
    dr->rotations[i] = quat_id;
    dr->scales[i] = f4_s(scale * 0.5f);
    return i;
}

static cmdstat_t CmdCornellBox(i32 argc, const char** argv)
{
    ConVar_SetFloat(&cv_pt_dist_alpha, 0.01f);
    ConVar_SetInt(&cv_pt_dist_samples, 10000);

    Entities* dr = Entities_Get();
    Entities_Clear(dr);
    ShutdownPtScene();
    LightmapShutdown();

    Camera_Reset();

    const float wallExtents = 5.0f;
    const float wallScale = 2.0f * wallExtents;
    const float lightScale = 1.0f;
    const float sphereScale = 1.0f;
    const float margin = sphereScale;
    const float lo = -wallExtents + margin;
    const float hi = wallExtents - margin;

    const float4 right = { 1.0f, 0.0f, 0.0f };
    const float4 left = { -1.0f, 0.0f, 0.0f };
    const float4 up = { 0.0f, 1.0f, 0.0f };
    const float4 down = { 0.0f, -1.0f, 0.0f };
    const float4 fwd = { 0.0f, 0.0f, -1.0f };
    const float4 back = { 0.0f, 0.0f, 1.0f };

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
    i = CreateQuad(
        "Cornell_Floor",
        f4_mulvs(down, wallExtents),
        down, back,
        wallScale,
        white,
        plastic,
        0x0);
    i = CreateQuad(
        "Cornell_Ceil",
        f4_mulvs(up, wallExtents),
        up, back,
        wallScale,
        white,
        plastic,
        0x0);
    i = CreateQuad(
        "Cornell_Light",
        f4_mulvs(up, wallExtents - kMilli),
        up, back,
        lightScale,
        f4_s(1.0f),
        light,
        0x0);

    i = CreateQuad(
        "Cornell_Left",
        f4_mulvs(left, wallExtents),
        left, up,
        wallScale,
        red,
        plastic,
        0x0);
    i = CreateQuad(
        "Cornell_Right",
        f4_mulvs(right, wallExtents),
        right, up,
        wallScale,
        green,
        plastic,
        0x0);

    i = CreateQuad(
        "Cornell_Near",
        f4_mulvs(back, wallExtents),
        back, up,
        wallScale,
        white,
        plastic,
        0x0);
    i = CreateQuad(
        "Cornell_Far",
        f4_mulvs(fwd, wallExtents),
        fwd, up,
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
                white,
                f4_v(roughness, 1.0f, 1.0f, 0.0f),
                0x0);
        }

        for (i32 j = 0; j < 5; ++j)
        {
            float t = (j + 0.5f) / 5;
            float roughness = f1_lerp(0.0f, 1.0f, t);
            float x = f1_lerp(lo, hi, t);
            float y = -wallExtents + sphereScale;
            float z = f1_lerp(lo, hi, 1.0f / 3.0f);
            char name[PIM_PATH];
            SPrintf(ARGS(name), "Cornell_PlasticSphere_%d", j);
            i = CreateSphere(
                name,
                f4_v(x, y, z, 0.0f),
                sphereScale,
                white,
                f4_v(roughness, 1.0f, 0.0f, 0.0f),
                0x0);
}

        for (i32 j = 0; j < 5; ++j)
        {
            float t = (j + 0.5f) / 5;
            float roughness = f1_lerp(0.0f, 1.0f, t);
            float x = f1_lerp(lo, hi, t);
            float y = -wallExtents + sphereScale;
            float z = f1_lerp(lo, hi, 2.0f / 3.0f);
            char name[PIM_PATH];
            SPrintf(ARGS(name), "Cornell_GlassSphere_%d", j);
            i = CreateSphere(
                name,
                f4_v(x, y, z, 0.0f),
                sphereScale,
                white,
                f4_v(roughness, 1.0f, 0.0f, 0.0f),
                MatFlag_Refractive);
            dr->materials[i].ior = 1.5f;
        }

    }
    else if (primType == PrimType_Boxes)
    {
        const float boxScale = 2.0f * sphereScale;
        {
            float x = f1_lerp(lo, hi, 0.2f);
            float y = -wallExtents + boxScale;
            float z = f1_lerp(lo, hi, 0.2f);
            i = CreateBox(
                "Cornell_MetalBox",
                f4_v(x, y, z, 0.0f),
                f4_normalize3(f4_v(0.2f, 0.0f, 1.0f, 0.0f)),
                f4_v(0.0f, 1.0f, 0.0f, 0.0f),
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
                f4_normalize3(f4_v(-0.2f, 0.0f, 1.0f, 0.0f)),
                f4_v(0.0f, 1.0f, 0.0f, 0.0f),
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
    cmd_enqueue("cornell_box");
    cmd_enqueue("teleport -4 0 4");
    cmd_enqueue("lookat 0 -1 0");
    cmd_enqueue("pt_denoise 0");
    cmd_enqueue("exp_manual 1");
    cmd_enqueue("exp_evoffset 0");
    cmd_enqueue("pt_trace 1");
    cmd_enqueue("wait 500");
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
    vkrSys_OnUnload();

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
        ConVar_SetFloat(&cv_pt_dist_alpha, 0.5f);
        ConVar_SetInt(&cv_pt_dist_samples, 1000);
        Entities_UpdateTransforms(Entities_Get());
        Entities_UpdateBounds(Entities_Get());
        vkrSys_OnLoad();
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
