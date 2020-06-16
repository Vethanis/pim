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

#include "rendering/constants.h"
#include "rendering/r_window.h"
#include "rendering/framebuffer.h"
#include "rendering/camera.h"
#include "rendering/lights.h"
#include "rendering/clear_tile.h"
#include "rendering/vertex_stage.h"
#include "rendering/fragment_stage.h"
#include "rendering/resolve_tile.h"
#include "rendering/screenblit.h"
#include "rendering/path_tracer.h"
#include "rendering/cubemap.h"
#include "rendering/drawable.h"
#include "rendering/model.h"
#include "rendering/lightmap.h"
#include "rendering/denoise.h"

#include "rendering/vulkan/vkr.h"

#include "quake/q_model.h"
#include "assets/asset_system.h"

#include <stb/stb_image_write.h>
#include <string.h>
#include <time.h>

static cvar_t cv_pt_trace = { cvart_bool, 0, "pt_trace", "0", "enable path tracing" };
static cvar_t cv_pt_bounces = { cvart_int, 0, "pt_bounces", "3", "path tracing bounces" };
static cvar_t cv_pt_denoise = { cvart_bool, 0, "pt_denoise", "0", "denoise path tracing output" };

static cvar_t cv_lm_gen = { cvart_bool, 0, "lm_gen", "0", "enable lightmap generation" };
static cvar_t cv_lm_denoise = { cvart_bool, 0, "lm_denoise", "0", "denoise lightmaps" };
static cvar_t cv_ac_gen = { cvart_bool, 0, "ac_gen", "0", "enable ambientcube generation" };
static cvar_t cv_cm_gen = { cvart_bool, 0, "cm_gen", "0", "enable cubemap generation" };

static cvar_t cv_r_sw = { cvart_bool, 0, "r_sw", "1", "use software renderer" };

static cvar_t cv_lm_density = { cvart_float, 0, "lm_density", "8", "lightmap texels per unit" };
static cvar_t cv_lm_timeslice = { cvart_int, 0, "lm_timeslice", "120", "number of frames required to add 1 lighting sample to all lightmap texels" };

// ----------------------------------------------------------------------------

static meshid_t GenSphereMesh(float r, i32 steps);
static textureid_t GenCheckerTex(void);
static cmdstat_t CmdCornellBox(i32 argc, const char** argv);

// ----------------------------------------------------------------------------

static framebuf_t ms_buffers[2];
static i32 ms_iFrame;

static TonemapId ms_tonemapper;
static float4 ms_toneParams;
static float4 ms_clearColor;

static pt_scene_t* ms_ptscene;
static pt_trace_t ms_trace;

static i32 ms_lmSampleCount;
static i32 ms_acSampleCount;
static i32 ms_ptSampleCount;
static i32 ms_cmapSampleCount;

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
        ms_ptscene = pt_scene_new(5);
    }
}

static void LightmapRepack(void)
{
    lmpack_del(lmpack_get());
    lmpack_t pack = lmpack_pack(1024, cv_lm_density.asFloat, 0.1f, 15.0f);
    *lmpack_get() = pack;
}

ProfileMark(pm_Lightmap_Trace, Lightmap_Trace)
ProfileMark(pm_Lightmap_Denoise, Lightmap_Denoise)
static void Lightmap_Trace(void)
{
    if (cvar_check_dirty(&cv_lm_density))
    {
        LightmapRepack();
    }

    if (cv_lm_gen.asFloat != 0.0f)
    {
        ProfileBegin(pm_Lightmap_Trace);

        if (lmpack_get()->lmCount == 0)
        {
            LightmapRepack();
        }

        EnsurePtScene();
        i32 bounces = (i32)cv_pt_bounces.asFloat;
        i32 timeSlice = (i32)cv_lm_timeslice.asFloat;
        lmpack_bake(ms_ptscene, bounces, 1.0f / timeSlice);

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

ProfileMark(pm_AmbCube_Trace, AmbCube_Trace)
static void AmbCube_Trace(void)
{
    if (cv_ac_gen.asFloat != 0.0f)
    {
        ProfileBegin(pm_AmbCube_Trace);
        EnsurePtScene();

        if (cvar_check_dirty(&cv_ac_gen))
        {
            ms_acSampleCount = 0;
        }

        camera_t camera;
        camera_get(&camera);

        i32 bounces = (i32)cv_pt_bounces.asFloat;
        AmbCube_t cube = AmbCube_Get();
        ms_acSampleCount = AmbCube_Bake(
            ms_ptscene, &cube, camera.position, 4096, ms_acSampleCount, bounces);
        AmbCube_Set(cube);

        ProfileEnd(pm_AmbCube_Trace);
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
        i32 bounces = (i32)cv_pt_bounces.asFloat;
        for (i32 i = 0; i < table->count; ++i)
        {
            Cubemap* cubemap = table->cubemaps + i;
            sphere_t bounds = table->bounds[i];
            Cubemap_Bake(cubemap, ms_ptscene, bounds.value, weight, bounces);
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

        if (cvar_check_dirty(&cv_pt_trace))
        {
            ms_ptSampleCount = 0;
        }

        const int2 size = { kDrawWidth, kDrawHeight };
        ms_trace.bounces = i1_clamp((i32)cv_pt_bounces.asFloat, 0, 20);
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

        ProfileBegin(pm_ptBlit);
        blit_3to4(size, GetFrontBuf()->light, output3);
        ProfileEnd(pm_ptBlit);

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

    ClearTile(frontBuf, ms_clearColor, camera.nearFar.y);
    drawables_trs();
    drawables_bounds();
    drawables_cull(&camera, backBuf);
    drawables_vertex();
    drawables_fragment(frontBuf, backBuf);

    ProfileEnd(pm_Rasterize);
}

static cmdstat_t CmdScreenshot(i32 argc, const char** argv)
{
    char filename[PIM_PATH] = {0};
    if (argc > 1 && argv[1])
    {
        StrCpy(ARGS(filename), argv[1]);
    }
    else
    {
        time_t ticks = time(NULL);
        struct tm* local = localtime(&ticks);
        char timestr[PIM_PATH] ={0};
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

    char mapname[PIM_PATH] = {0};
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

    con_logf(LogSev_Info, "cmd", "mapload is loading '%s'.", mapname);

    u32* ids = LoadModelAsDrawables(mapname);
    if (ids)
    {
        pim_free(ids);

        if (lights_pt_count() == 0)
        {
            lights_add_pt((pt_light_t) { f4_v(0.0f, 0.0f, 0.0f, 1.0f), f4_s(30.0f) });
        }

        drawables_trs();

        con_logf(LogSev_Info, "cmd", "mapload loaded '%s'.", mapname);

        return cmdstat_ok;
    }
    return cmdstat_err;
}

void render_sys_init(void)
{
    cvar_reg(&cv_pt_trace);
    cvar_reg(&cv_pt_bounces);
    cvar_reg(&cv_pt_denoise);
    cvar_reg(&cv_lm_gen);
    cvar_reg(&cv_lm_denoise);
    cvar_reg(&cv_ac_gen);
    cvar_reg(&cv_cm_gen);
    cvar_reg(&cv_r_sw);
    cvar_reg(&cv_lm_density);
    cvar_reg(&cv_lm_timeslice);
    cmd_reg("screenshot", CmdScreenshot);
    cmd_reg("mapload", CmdLoadMap);
    cmd_reg("cornell_box", CmdCornellBox);

    vkr_init();

    ms_iFrame = 0;
    framebuf_create(GetFrontBuf(), kDrawWidth, kDrawHeight);
    framebuf_create(GetBackBuf(), kDrawWidth, kDrawHeight);
    screenblit_init(kDrawWidth, kDrawHeight);

    ms_tonemapper = TMap_Reinhard;
    ms_toneParams = Tonemap_DefParams();
    ms_clearColor = f4_v(0.01f, 0.012f, 0.022f, 0.0f);

    con_exec("mapload start");
}

ProfileMark(pm_update, render_sys_update)
void render_sys_update(void)
{
    ProfileBegin(pm_update);

    Lightmap_Trace();
    AmbCube_Trace();
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
    task_sys_schedule();

    pt_scene_del(ms_ptscene);
    ms_ptscene = NULL;

    screenblit_shutdown();
    framebuf_destroy(GetFrontBuf());
    framebuf_destroy(GetBackBuf());

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

        if (igCollapsingHeader1("Material"))
        {
            igIndent(0.0f);

            pt_light_t light = lights_get_pt(0);
            igColorEdit3("Light Radiance", &light.rad.x, hdrPicker);
            lights_set_pt(0, light);

            igUnindent(0.0f);
        }

        if (igCollapsingHeader1("Culling Stats"))
        {
            const drawables_t* drawTable = drawables_get();
            const i32 width = drawTable->count;
            const sphere_t* bounds = drawTable->bounds;
            const u64* tileMasks = drawTable->tileMasks;

            i32 numVisible = 0;
            for (i32 i = 0; i < width; ++i)
            {
                if (tileMasks[i])
                {
                    ++numVisible;
                }
            }

            igText("Drawables: %d", width);
            igText("Visible: %d", numVisible);
            igText("Culled: %d", width - numVisible);
            igSeparator();

            camera_t camera;
            camera_get(&camera);
            frus_t frus;
            camera_frustum(&camera, &frus);

            float* distances = tmp_malloc(sizeof(distances[0]) * width);
            for (i32 i = 0; i < width; ++i)
            {
                distances[i] = sdFrusSph(frus, bounds[i]);
            }
            i32* indices = indsort(distances, width, sizeof(distances[0]), CmpFloat, NULL);
            igColumns(4);
            igText("Visible"); igNextColumn();
            igText("Distance"); igNextColumn();
            igText("Center"); igNextColumn();
            igText("Radius"); igNextColumn();
            igSeparator();
            for (i32 i = 0; i < width; ++i)
            {
                i32 j = indices[i];
                float4 sph = bounds[j].value;
                u64 tilemask = tileMasks[j];
                float dist = distances[j];
                const char* tag = (tilemask) ? "Visible" : "Culled";

                igText(tag); igNextColumn();
                igText("%.2f", dist); igNextColumn();
                igText("%.2f %.2f %.2f", sph.x, sph.y, sph.z); igNextColumn();
                igText("%.2f", sph.w); igNextColumn();
            }
            igColumns(1);
        }
    }
    igEnd();

    ProfileEnd(pm_gui);
}

// ----------------------------------------------------------------------------

static meshid_t GenSphereMesh(float r, i32 steps)
{
    const i32 vsteps = steps;       // divisions along y axis
    const i32 hsteps = steps * 2;   // divisions along x-z plane
    const float dv = kPi / vsteps;
    const float dh = kTau / hsteps;

    const i32 maxlen = 6 * vsteps * hsteps;
    i32 length = 0;
    float4* pim_noalias positions = perm_malloc(sizeof(*positions) * maxlen);
    float4* pim_noalias normals = perm_malloc(sizeof(*normals) * maxlen);
    float2* pim_noalias uvs = perm_malloc(sizeof(*uvs) * maxlen);
    float3* pim_noalias lmUvs = perm_malloc(sizeof(lmUvs[0]) * maxlen);

    for (i32 i = 0; i < maxlen; ++i)
    {
        lmUvs[i].z = -1.0f;
    }

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

    mesh_t* mesh = perm_calloc(sizeof(*mesh));
    mesh->length = length;
    mesh->positions = positions;
    mesh->normals = normals;
    mesh->uvs = uvs;
    mesh->lmUvs = lmUvs;
    return mesh_create(mesh);
}

// N = (0, 0, 1)
// centered at origin, [-0.5, 0.5]
static meshid_t GenQuadMesh(void)
{
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
        lmUvs[i].z = -1.0f;
    }

    mesh_t* mesh = perm_calloc(sizeof(*mesh));
    mesh->length = length;
    mesh->positions = positions;
    mesh->normals = normals;
    mesh->uvs = uvs;
    mesh->lmUvs = lmUvs;
    return mesh_create(mesh);
}

static meshid_t ms_quadmesh;
static meshid_t ms_spheremesh;
static cmdstat_t CmdCornellBox(i32 argc, const char** argv)
{
    drawables_clear();
    pt_scene_del(ms_ptscene);
    ms_ptscene = NULL;
    lmpack_del(lmpack_get());

    while (lights_pt_count() > 0)
    {
        lights_rm_pt(0);
    }
    while (lights_dir_count() > 0)
    {
        lights_rm_dir(0);
    }

    if (!ms_quadmesh.handle)
    {
        ms_quadmesh = GenQuadMesh();
        ms_spheremesh = GenSphereMesh(1.0f, 32);
    }

    meshid_t quad = ms_quadmesh;
    meshid_t sphere = ms_spheremesh;

    i32 iFloor = drawables_add(HashStr("CornellBox_Floor"));
    i32 iCeil = drawables_add(HashStr("CornellBox_Ceiling"));
    i32 iLeft = drawables_add(HashStr("CornellBox_LeftWall"));
    i32 iRight = drawables_add(HashStr("CornellBox_RightWall"));
    i32 iNear = drawables_add(HashStr("CornellBox_NearWall"));
    i32 iFar = drawables_add(HashStr("CornellBox_FarWall"));
    i32 iLight = drawables_add(HashStr("CornellBox_Light"));
    i32 iMetal = drawables_add(HashStr("CornellBox_MetalSphere"));
    i32 iPlastic = drawables_add(HashStr("CornellBox_PlasticSphere"));
    drawables_t* drawables = drawables_get();
    
    drawables->meshes[iFloor] = quad;
    drawables->meshes[iCeil] = quad;
    drawables->meshes[iLeft] = quad;
    drawables->meshes[iRight] = quad;
    drawables->meshes[iNear] = quad;
    drawables->meshes[iFar] = quad;
    drawables->meshes[iLight] = quad;
    drawables->meshes[iMetal] = sphere;
    drawables->meshes[iPlastic] = sphere;

    const float wallExtents = 2.0f;
    const float sphereRad = 0.3f;
    const float lightExtents = 0.5f;

    const float wallScale = 2.0f * wallExtents;
    const float sphereDiam = 2.0f * sphereRad;
    const float lightScale = 2.0f * lightExtents;

    const float4 x4 = { 1.0f, 0.0f, 0.0f };
    const float4 y4 = { 0.0f, 1.0f, 0.0f };
    const float4 z4 = { 0.0f, 0.0f, 1.0f };

    drawables->translations[iFloor] = f4_v(0.0f, -wallExtents, 0.0f, 0.0f);
    drawables->translations[iCeil] = f4_v(0.0f, wallExtents, 0.0f, 0.0f);
    drawables->translations[iLeft] = f4_v(-wallExtents, 0.0f, 0.0f, 0.0f);
    drawables->translations[iRight] = f4_v(wallExtents, 0.0f, 0.0f, 0.0f);
    drawables->translations[iNear] = f4_v(0.0f, 0.0f, wallExtents, 0.0f);
    drawables->translations[iFar] = f4_v(0.0f, 0.0f, -wallExtents, 0.0f);

    drawables->translations[iLight] = f4_v(0.0f, wallExtents - 0.01f, 0.0f, 0.0f);

    drawables->translations[iMetal] = f4_v(-sphereDiam, -wallExtents + sphereDiam, sphereDiam, 0.0f);
    drawables->translations[iPlastic] = f4_v(sphereDiam, -wallExtents + sphereDiam, -sphereDiam, 0.0f);

    drawables->scales[iFloor] = f4_s(wallScale);
    drawables->scales[iCeil] = f4_s(wallScale);
    drawables->scales[iLeft] = f4_s(wallScale);
    drawables->scales[iRight] = f4_s(wallScale);
    drawables->scales[iNear] = f4_s(wallScale);
    drawables->scales[iFar] = f4_s(wallScale);

    drawables->scales[iLight] = f4_s(lightScale);

    drawables->scales[iMetal] = f4_s(sphereDiam);
    drawables->scales[iPlastic] = f4_s(sphereDiam);

    drawables->rotations[iCeil] = quat_lookat(y4, z4);
    drawables->rotations[iFloor] = quat_lookat(f4_neg(y4), f4_neg(z4));

    drawables->rotations[iRight] = quat_lookat(x4, y4);
    drawables->rotations[iLeft] = quat_lookat(f4_neg(x4), y4);

    drawables->rotations[iFar] = quat_lookat(f4_neg(z4), y4);
    drawables->rotations[iNear] = quat_lookat(z4, y4);

    drawables->rotations[iLight] = drawables->rotations[iCeil];

    drawables->rotations[iMetal] = quat_id;
    drawables->rotations[iPlastic] = quat_id;

    const float4 st = { 1.0f, 1.0f, 0.0f, 0.0f };
    const u32 white = LinearToColor(f4_v(1.0f, 1.0f, 1.0f, 1.0f));
    const u32 red = LinearToColor(f4_v(1.0f, 0.0f, 0.0f, 1.0f));
    const u32 green = LinearToColor(f4_v(0.0f, 1.0f, 0.0f, 1.0f));
    const u32 blue = LinearToColor(f4_v(0.0f, 0.0f, 1.0f, 1.0f));
    const u32 boxRome = LinearToColor(f4_v(0.9f, 1.0f, 0.0f, 0.0f));
    const u32 lightRome = LinearToColor(f4_v(0.5f, 1.0f, 0.0f, 0.35f));
    const u32 plasticRome = LinearToColor(f4_v(1.0f, 1.0f, 0.0f, 0.0f));
    const u32 metalRome = LinearToColor(f4_v(0.25f, 1.0f, 1.0f, 0.0f));

    const material_t whiteBoxMat = (material_t)
    {
        .st = st,
        .flatAlbedo = white,
        .flatRome = boxRome,
    };
    const material_t greenBoxMat = (material_t)
    {
        .st = st,
        .flatAlbedo = green,
        .flatRome = boxRome,
    };
    const material_t redBoxMat = (material_t)
    {
        .st = st,
        .flatAlbedo = red,
        .flatRome = boxRome,
    };
    const material_t lightMat = (material_t)
    {
        .st = st,
        .flatAlbedo = white,
        .flatRome = lightRome,
    };
    const material_t plasticMat = (material_t)
    {
        .st = st,
        .flatAlbedo = white,
        .flatRome = plasticRome,
    };
    const material_t metalMat = (material_t)
    {
        .st = st,
        .flatAlbedo = white,
        .flatRome = metalRome,
    };

    drawables->materials[iFloor] = whiteBoxMat;
    drawables->materials[iCeil] = whiteBoxMat;
    drawables->materials[iNear] = whiteBoxMat;
    drawables->materials[iFar] = whiteBoxMat;
    drawables->materials[iLeft] = redBoxMat;
    drawables->materials[iRight] = greenBoxMat;

    drawables->materials[iLight] = lightMat;

    drawables->materials[iPlastic] = plasticMat;
    drawables->materials[iMetal] = metalMat;

    drawables_trs();

    return cmdstat_ok;
}

static textureid_t GenCheckerTex(void)
{
    textureid_t id = texture_lookup("checkertex");
    if (id.version)
    {
        texture_retain(id);
        return id;
    }

    const i32 size = 1 << 8;
    u32* pim_noalias texels = perm_malloc(sizeof(texels[0]) * size * size);

    const float4 a = f4_s(1.0f);
    const float4 b = f4_s(0.01f);
    for (i32 y = 0; y < size; ++y)
    {
        for (i32 x = 0; x < size; ++x)
        {
            const i32 i = x + y * size;
            bool c0 = (x & 7) < 4;
            bool c1 = (y & 7) < 4;
            bool c2 = c0 != c1;
            float4 c = c2 ? a : b;
            texels[i] = LinearToColor(c);
        }
    }

    return texture_new(i2_s(size), texels, "checkertex");
}
