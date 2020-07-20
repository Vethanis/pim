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
#include "math/color.h"
#include "math/sdf.h"
#include "math/ambcube.h"
#include "math/sampling.h"
#include "math/frustum.h"
#include "math/lighting.h"
#include "math/cubic_fit.h"

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

#include "rendering/vulkan/vkr.h"

#include "quake/q_model.h"
#include "assets/asset_system.h"

#include <stb/stb_image_write.h>
#include <string.h>
#include <time.h>

static cvar_t cv_pt_trace = { cvart_bool, 0, "pt_trace", "0", "enable path tracing" };
static cvar_t cv_pt_denoise = { cvart_bool, 0, "pt_denoise", "0", "denoise path tracing output" };
static cvar_t cv_pt_normal = { cvart_bool, 0, "pt_normal", "0", "output path tracer normals" };
static cvar_t cv_pt_albedo = { cvart_bool, 0, "pt_albedo", "0", "output path tracer albedo" };
static cvar_t cv_pt_lgrid_mpc = { cvart_float, 0, "pt_lgrid_mpc", "2", "light grid meters per cell" };

static cvar_t cv_lm_gen = { cvart_bool, 0, "lm_gen", "0", "enable lightmap generation" };
static cvar_t cv_lm_denoise = { cvart_bool, 0, "lm_denoise", "0", "denoise lightmaps" };
static cvar_t cv_cm_gen = { cvart_bool, 0, "cm_gen", "0", "enable cubemap generation" };

static cvar_t cv_r_sw = { cvart_bool, 0, "r_sw", "1", "use software renderer" };

static cvar_t cv_lm_density = { cvart_float, 0, "lm_density", "8", "lightmap texels per unit" };
static cvar_t cv_lm_timeslice = { cvart_int, 0, "lm_timeslice", "10", "number of frames required to add 1 lighting sample to all lightmap texels" };

static cvar_t cv_r_sun_az = { cvart_float, 0, "r_sun_az", "0.75", "Sun Direction Azimuth" };
static cvar_t cv_r_sun_ze = { cvart_float, 0, "r_sun_ze", "0.666f", "Sun Direction Zenith" };
static cvar_t cv_r_sun_rad = { cvart_float, 0, "r_sun_rad", "1365", "Sun Irradiance" };
static cvar_t cv_r_lm_denoised = { cvart_bool, 0, "r_lm_denoised", "0", "Render denoised lightmap" };

static void RegCVars(void)
{
    cvar_reg(&cv_r_sw);

    cvar_reg(&cv_pt_trace);
    cvar_reg(&cv_pt_denoise);
    cvar_reg(&cv_pt_normal);
    cvar_reg(&cv_pt_albedo);
    cvar_reg(&cv_pt_lgrid_mpc);

    cvar_reg(&cv_lm_gen);
    cvar_reg(&cv_lm_denoise);
    cvar_reg(&cv_lm_density);
    cvar_reg(&cv_lm_timeslice);

    cvar_reg(&cv_cm_gen);

    cvar_reg(&cv_r_sun_az);
    cvar_reg(&cv_r_sun_ze);
    cvar_reg(&cv_r_sun_rad);
    cvar_reg(&cv_r_lm_denoised);
}

// ----------------------------------------------------------------------------

static meshid_t GenSphereMesh(i32 steps);
static cmdstat_t CmdCornellBox(i32 argc, const char** argv);

// ----------------------------------------------------------------------------

static framebuf_t ms_buffers[2];
static i32 ms_iFrame;

static TonemapId ms_tonemapper = TMap_Hable;
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
    .histMinProb = 0.05f,
    .histMaxProb = 0.95f,
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

static void LightmapRepack(void)
{
    EnsurePtScene();

    lmpack_del(lmpack_get());
    lmpack_t pack = lmpack_pack(ms_ptscene, 1024, cv_lm_density.asFloat, 0.1f, 15.0f);
    *lmpack_get() = pack;
}

ProfileMark(pm_Lightmap_Trace, Lightmap_Trace)
ProfileMark(pm_Lightmap_Denoise, Lightmap_Denoise)
static void Lightmap_Trace(void)
{
    if (cv_lm_gen.asFloat != 0.0f)
    {
        ProfileBegin(pm_Lightmap_Trace);
        EnsurePtScene();

        bool dirty = lmpack_get()->lmCount == 0;
        dirty |= cvar_check_dirty(&cv_lm_density);
        if (dirty)
        {
            LightmapRepack();
        }

        float timeslice = 1.0f / f1_max(1.0f, cv_lm_timeslice.asFloat);
        lmpack_bake(ms_ptscene, timeslice);

        ProfileEnd(pm_Lightmap_Trace);
    }

    if (cv_lm_denoise.asFloat != 0.0f)
    {
        ProfileBegin(pm_Lightmap_Denoise);

        cvar_set_float(&cv_lm_denoise, 0.0f);
        lmpack_denoise();
        cvar_t* cv_r_lm_denoised = cvar_find("r_lm_denoised");
        if (cv_r_lm_denoised)
        {
            cvar_set_float(cv_r_lm_denoised, 1.0f);
        }

        ProfileEnd(pm_Lightmap_Denoise);
    }
}

ProfileMark(pm_CubemapTrace, Cubemap_Trace)
static void Cubemap_Trace(void)
{
    if (cv_cm_gen.asFloat != 0.0f)
    {
        ProfileBegin(pm_CubemapTrace);
        EnsurePtScene();

        if (cvar_check_dirty(&cv_cm_gen))
        {
            ms_cmapSampleCount = 0;
        }

        Cubemaps_t* table = Cubemaps_Get();
        float weight = 1.0f / ++ms_cmapSampleCount;
        float pfweight = f1_min(1.0f, weight * 2.0f);
        for (i32 i = 0; i < table->count; ++i)
        {
            Cubemap* cubemap = table->cubemaps + i;
            sphere_t bounds = table->bounds[i];
            Cubemap_Bake(cubemap, ms_ptscene, bounds.value, weight);
            Cubemap_Denoise(cubemap);
            Cubemap_Convolve(cubemap, 256, pfweight);
        }

        ProfileEnd(pm_CubemapTrace);
    }
}

ProfileMark(pm_PathTrace, PathTrace)
ProfileMark(pm_ptDenoise, Denoise)
ProfileMark(pm_ptBlit, Blit)
static bool PathTrace(void)
{
    if (cv_pt_trace.asFloat != 0.0f)
    {
        ProfileBegin(pm_PathTrace);
        EnsurePtScene();

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

        const int2 size = { kDrawWidth, kDrawHeight };
        ms_trace.sampleWeight = 1.0f / ++ms_ptSampleCount;
        ms_trace.camera = &camera;
        ms_trace.scene = ms_ptscene;
        ms_trace.imageSize = size;
        if (!ms_trace.color)
        {
            ms_trace.color = perm_calloc(sizeof(ms_trace.color[0]) * kDrawPixels);
            ms_trace.albedo = perm_calloc(sizeof(ms_trace.albedo[0]) * kDrawPixels);
            ms_trace.normal = perm_calloc(sizeof(ms_trace.normal[0]) * kDrawPixels);
        }

        pt_trace(&ms_trace);

        float3* pim_noalias output3 = ms_trace.color;
        if (cv_pt_denoise.asFloat != 0.0f)
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
                cvar_set_float(&cv_pt_denoise, 0.0f);
            }
        }
        if (cv_pt_albedo.asFloat != 0.0f)
        {
            output3 = ms_trace.albedo;
        }
        if (cv_pt_normal.asFloat != 0.0f)
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
    ProfileBegin(pm_Rasterize);

    framebuf_t* frontBuf = GetFrontBuf();
    framebuf_t* backBuf = GetBackBuf();

    camera_t camera;
    camera_get(&camera);

    drawables_trs();
    RtcDraw(frontBuf, &camera);

    ProfileEnd(pm_Rasterize);
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
static void Present(void)
{
    ProfileBegin(pm_Present);
    framebuf_t* frontBuf = GetFrontBuf();
    int2 size = { frontBuf->width, frontBuf->height };
    ms_exposure.deltaTime = (float)time_dtf();
    ExposeImage(size, frontBuf->light, &ms_exposure);
    ResolveTile(frontBuf, ms_tonemapper, ms_toneParams);
    screenblit_blit(frontBuf->color, frontBuf->width, frontBuf->height);
    TakeScreenshot();
    SwapBuffers();
    ProfileEnd(pm_Present);
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

    char mapname[PIM_PATH] = { 0 };
    SPrintf(ARGS(mapname), "maps/%s.bsp", name);

    asset_t asset = { 0 };
    if (!asset_get(mapname, &asset))
    {
        con_logf(LogSev_Error, "cmd", "mapload <map name>; could not find map named '%s'.", name);
        return cmdstat_err;
    }

    con_logf(LogSev_Info, "cmd", "mapload is clearing drawables.");
    drawables_clear();
    pt_scene_del(ms_ptscene);
    ms_ptscene = NULL;
    lmpack_del(lmpack_get());

    con_logf(LogSev_Info, "cmd", "mapload is loading '%s'.", mapname);

    u32* ids = LoadModelAsDrawables(mapname);
    if (ids)
    {
        pim_free(ids);

        if (lights_pt_count() == 0)
        {
            lights_add_pt(
                (pt_light_t)
            {
                f4_v(0.0f, 0.0f, 0.0f, 1.0f),
                    f4_s(30.0f),
            });
        }

        drawables_trs();

        con_logf(LogSev_Info, "cmd", "mapload loaded '%s'.", mapname);

        return cmdstat_ok;
    }
    return cmdstat_err;
}

void render_sys_init(void)
{
    ms_iFrame = 0;
    RegCVars();

    cmd_reg("screenshot", CmdScreenshot);
    cmd_reg("mapload", CmdLoadMap);
    cmd_reg("cornell_box", CmdCornellBox);

    vkr_init();

    texture_sys_init();
    mesh_sys_init();

    framebuf_create(GetFrontBuf(), kDrawWidth, kDrawHeight);
    framebuf_create(GetBackBuf(), kDrawWidth, kDrawHeight);
    screenblit_init(kDrawWidth, kDrawHeight);
    pt_sys_init();
    RtcDrawInit();

    ms_toneParams = Tonemap_DefParams();
    ms_toneParams.x = 0.25f;
    ms_toneParams.w = 0.25f;
    ms_clearColor = f4_v(0.01f, 0.012f, 0.022f, 0.0f);

    con_exec("mapload start");

    //const i32 len = 1 << 10;
    //float* xs = tmp_malloc(sizeof(xs[0]) * len);
    //float* ys = tmp_malloc(sizeof(ys[0]) * len);
    //for (i32 i = 0; i < len; ++i)
    //{
    //    float x = (i + 0.0f) / len;
    //    float y = LinearTosRGB(x);
    //    xs[i] = x;
    //    ys[i] = y;
    //}
    //dataset_t dataset = { 0 };
    //dataset.len = len;
    //dataset.xs = xs;
    //dataset.ys = ys;
    //fit_t fit;
    //float error = SqrticFit(dataset, &fit, len << 8);
    //con_logf(LogSev_Info, "fit", "Error: %f", error);
    //for (i32 i = 0; i < NELEM(fit.value); ++i)
    //{
    //    con_logf(LogSev_Info, "fit", "fit[%d] = %f", i, fit.value[i]);
    //}
}

ProfileMark(pm_update, render_sys_update)
void render_sys_update(void)
{
    ProfileBegin(pm_update);

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

    Lightmap_Trace();
    Cubemap_Trace();
    if (!PathTrace())
    {
        if (cv_r_sw.asFloat != 0.0f)
        {
            Rasterize();
        }
        else
        {
            vkr_update();
        }
    }
    Present();
    Denoise_Evict();

    ProfileEnd(pm_update);
}

void render_sys_shutdown(void)
{
    RtcDrawShutdown();

    pt_scene_del(ms_ptscene);
    ms_ptscene = NULL;

    pt_sys_shutdown();
    screenblit_shutdown();
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
                igSliderFloat("Shoulder Strength", &ms_toneParams.x, 0.0f, 1.0f);
                igSliderFloat("Linear Strength", &ms_toneParams.y, 0.0f, 1.0f);
                igSliderFloat("Linear Angle", &ms_toneParams.z, 0.0f, 1.0f);
                igSliderFloat("Toe Strength", &ms_toneParams.w, 0.0f, 1.0f);
            }
            igUnindent(0.0f);
        }

        if (igCollapsingHeader1("Exposure"))
        {
            igIndent(0.0f);
            igCheckbox("Manual", &ms_exposure.manual);
            igCheckbox("Standard", &ms_exposure.standard);
            igSliderFloat("Output Offset EV", &ms_exposure.offsetEV, -5.0f, 5.0f);
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
    }
    igEnd();

    ProfileEnd(pm_gui);
}

// ----------------------------------------------------------------------------

static meshid_t GenSphereMesh(i32 steps)
{
    char name[PIM_PATH];
    SPrintf(ARGS(name), "SphereMesh_%d", steps);

    meshid_t id = { 0 };
    if (mesh_find(name, &id))
    {
        mesh_retain(id);
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
    float2* pim_noalias uvs = perm_malloc(sizeof(*uvs) * maxlen);

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

            float2 u1 = f2_v(phi1 / kTau, 1.0f - theta1 / kPi);
            float2 u2 = f2_v(phi2 / kTau, 1.0f - theta1 / kPi);
            float2 u3 = f2_v(phi2 / kTau, 1.0f - theta2 / kPi);
            float2 u4 = f2_v(phi1 / kTau, 1.0f - theta2 / kPi);

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
    bool added = mesh_new(&mesh, name, &id);
    ASSERT(added);
    return id;
}

// N = (0, 0, 1)
// centered at origin, [-0.5, 0.5]
static meshid_t GenQuadMesh(void)
{
    const char* name = "QuadMesh";
    meshid_t id = { 0 };
    if (mesh_find(name, &id))
    {
        mesh_retain(id);
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
    float2* uvs = perm_malloc(sizeof(uvs[0]) * length);
    float3* lmUvs = perm_malloc(sizeof(lmUvs[0]) * length);

    // counter clockwise
    positions[0] = tl; uvs[0] = f2_v(0.0f, 1.0f);
    positions[1] = bl; uvs[1] = f2_v(0.0f, 0.0f);
    positions[2] = tr; uvs[2] = f2_v(1.0f, 1.0f);
    positions[3] = tr; uvs[3] = f2_v(1.0f, 1.0f);
    positions[4] = bl; uvs[4] = f2_v(0.0f, 0.0f);
    positions[5] = br; uvs[5] = f2_v(1.0f, 0.0f);
    for (i32 i = 0; i < length; ++i)
    {
        normals[i] = N;
    }

    mesh_t mesh = { 0 };
    mesh.length = length;
    mesh.positions = positions;
    mesh.normals = normals;
    mesh.uvs = uvs;
    bool added = mesh_new(&mesh, name, &id);
    ASSERT(added);
    return id;
}

static meshid_t ms_quadmesh;
static i32 CreateQuad(const char* name, float4 center, float4 forward, float4 up, float scale, float4 albedo, float4 rome)
{
    if (!ms_quadmesh.version)
    {
        ms_quadmesh = GenQuadMesh();
    }

    i32 i = drawables_add(HashStr(name));
    drawables_t* drawables = drawables_get();
    drawables->meshes[i] = ms_quadmesh;
    drawables->translations[i] = center;
    drawables->scales[i] = f4_s(scale);
    drawables->rotations[i] = quat_lookat(forward, up);

    material_t mat = (material_t)
    {
        .st = f4_v(1.0f, 1.0f, 0.0f, 0.0f),
            .flatAlbedo = LinearToColor(albedo),
            .flatRome = LinearToColor(rome),
    };
    drawables->materials[i] = mat;

    return i;
}

static meshid_t ms_spheremesh;
static i32 CreateSphere(const char* name, float4 center, float radius, float4 albedo, float4 rome)
{
    if (!ms_spheremesh.version)
    {
        ms_spheremesh = GenSphereMesh(32);
    }

    i32 i = drawables_add(HashStr(name));
    drawables_t* drawables = drawables_get();
    drawables->meshes[i] = ms_spheremesh;
    drawables->translations[i] = center;
    drawables->scales[i] = f4_s(radius);
    drawables->rotations[i] = quat_id;

    material_t mat = (material_t)
    {
        .st = f4_v(1.0f, 1.0f, 0.0f, 0.0f),
            .flatAlbedo = LinearToColor(albedo),
            .flatRome = LinearToColor(rome),
    };
    drawables->materials[i] = mat;

    return i;
}

static cmdstat_t CmdCornellBox(i32 argc, const char** argv)
{
    drawables_clear();
    pt_scene_del(ms_ptscene);
    ms_ptscene = NULL;
    lmpack_del(lmpack_get());

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

    drawables_trs();

    return cmdstat_ok;
}
