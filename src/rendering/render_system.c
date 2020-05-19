#include "rendering/render_system.h"

#include "allocator/allocator.h"
#include "components/table.h"
#include "components/components.h"
#include "components/drawables.h"
#include "threading/task.h"
#include "ui/cimgui.h"

#include "common/time.h"
#include "common/cvar.h"
#include "common/profiler.h"
#include "common/console.h"
#include "common/stringutil.h"
#include "common/sort.h"

#include "math/types.h"
#include "math/int2_funcs.h"
#include "math/float4_funcs.h"
#include "math/float3_funcs.h"
#include "math/float2_funcs.h"
#include "math/float4x4_funcs.h"
#include "math/color.h"
#include "math/sdf.h"
#include "math/sphgauss.h"
#include "math/sampling.h"
#include "math/frustum.h"

#include "rendering/constants.h"
#include "rendering/window.h"
#include "rendering/framebuffer.h"
#include "rendering/camera.h"
#include "rendering/lights.h"
#include "rendering/clear_tile.h"
#include "rendering/vertex_stage.h"
#include "rendering/fragment_stage.h"
#include "rendering/resolve_tile.h"
#include "rendering/screenblit.h"
#include "rendering/path_tracer.h"

#include "quake/q_model.h"
#include "assets/asset_system.h"

#include <string.h>

static cvar_t cv_pt_trace = { cvart_bool, 0, "pt_trace", "0", "enable path tracing of scene" };
static cvar_t cv_pt_bounces = { cvart_int, 0, "pt_bounces", "10", "number of bounces in the path tracer" };
static cvar_t cv_sg_gen = { cvart_bool, 0, "sg_gen", "0", "enable spherical gaussian light accumulation" };
static cvar_t cv_sg_count = { cvart_int, 0, "sg_count", "9", "number of spherical gaussian lobes to use" };

// ----------------------------------------------------------------------------

static meshid_t GenSphereMesh(float r, i32 steps);
static textureid_t GenCheckerTex(void);

// ----------------------------------------------------------------------------

static framebuf_t ms_buffers[2];
static i32 ms_iFrame;
static float4 ms_flatAlbedo;
static float4 ms_flatRome;

static TonemapId ms_tonemapper;
static float4 ms_toneParams;
static float4 ms_clearColor;

static pt_scene_t* ms_ptscene;
static pt_trace_t ms_trace;
static camera_t ms_ptcamera;
static i32 ms_sampleCount;

// ----------------------------------------------------------------------------

static float2 VEC_CALL CalcUv(float4 s, float4 t, float4 p)
{
    return f2_v(f4_dot3(p, s) + s.w, f4_dot3(p, t) + t.w);
}

static void VEC_CALL CalcST(
    float4 N,
    float4* pim_noalias s,
    float4* pim_noalias t)
{
    const float4 kX = { 1.0f, 0.0f, 0.0f, 0.0f };
    const float4 kY = { 0.0f, 1.0f, 0.0f, 0.0f };
    const float4 kZ = { 0.0f, 0.0f, 1.0f, 0.0f };
    float4 n = f4_abs(N);
    float k = f4_hmax3(n);
    if (k == n.x)
    {
        *s = kZ;
        *t = kY;
    }
    else if (k == n.y)
    {
        *s = kX;
        *t = f4_neg(kZ);
    }
    else
    {
        *s = f4_neg(kX);
        *t = kY;
    }
}

static i32 VEC_CALL FlattenSurface(
    const mmodel_t* model,
    const msurface_t* surface,
    float4** pim_noalias pTris,
    float4** pim_noalias pPolys)
{
    i32 numEdges = surface->numedges;
    const i32 firstEdge = surface->firstedge;
    const i32* pim_noalias surfEdges = model->surfedges;
    const medge_t* pim_noalias edges = model->edges;
    const float4* pim_noalias vertices = model->vertices;

    float4* pim_noalias polygon = tmp_realloc(*pPolys, sizeof(polygon[0]) * numEdges);
    *pPolys = polygon;

    for (i32 i = 0; i < numEdges; ++i)
    {
        i32 e = surfEdges[firstEdge + i];
        i32 v;
        if (e >= 0)
        {
            v = edges[e].v[0];
        }
        else
        {
            v = edges[-e].v[1];
        }
        polygon[i] = vertices[v];
    }

    float4* pim_noalias triangles = tmp_realloc(*pTris, sizeof(triangles[0]) * numEdges * 3);
    *pTris = triangles;

    i32 resLen = 0;
    while (numEdges >= 3)
    {
        triangles[resLen + 0] = polygon[0];
        triangles[resLen + 1] = polygon[1];
        triangles[resLen + 2] = polygon[2];
        resLen += 3;

        --numEdges;
        for (i32 j = 1; j < numEdges; ++j)
        {
            polygon[j] = polygon[j + 1];
        }
    }

    return resLen;
}

static textureid_t* GenTextures(const msurface_t* surfaces, i32 surfCount)
{
    textureid_t* ids = tmp_calloc(sizeof(ids[0]) * surfCount);

    for (i32 i = 0; i < surfCount; ++i)
    {
        const msurface_t* surface = surfaces + i;
        const mtexinfo_t* texinfo = surface->texinfo;
        const mtexture_t* mtex = texinfo->texture;

        textureid_t texid = texture_lookup(mtex->name);
        if (!texid.handle)
        {
            const u8* mip0 = (u8*)mtex + mtex->offsets[0];
            ASSERT(mtex->offsets[0] == sizeof(mtexture_t));
            texid = texture_unpalette(mip0, i2_v(mtex->width, mtex->height));
            texture_register(mtex->name, texid);
        }
        ids[i] = texid;
    }

    return ids;
}

static meshid_t VEC_CALL TrisToMesh(
    float4x4 M,
    const msurface_t* surface,
    const float4* pim_noalias tris,
    i32 vertCount)
{
    float4* pim_noalias positions = perm_malloc(sizeof(positions[0]) * vertCount);
    float4* pim_noalias normals = perm_malloc(sizeof(normals[0]) * vertCount);
    float2* pim_noalias uvs = perm_malloc(sizeof(uvs[0]) * vertCount);

    // u = dot(P.xyz, s.xyz) + s.w
    // v = dot(P.xyz, t.xyz) + t.w
    const mtexinfo_t* texinfo = surface->texinfo;
    const mtexture_t* mtex = texinfo->texture;
    float4 s = texinfo->vecs[0];
    float4 t = texinfo->vecs[1];
    float2 uvScale = { 1.0f / mtex->width, 1.0f / mtex->height };

    for (i32 i = 0; (i + 3) <= vertCount; i += 3)
    {
        float4 A0 = tris[i + 0];
        float4 B0 = tris[i + 1];
        float4 C0 = tris[i + 2];
        float4 A = f4x4_mul_pt(M, A0);
        float4 B = f4x4_mul_pt(M, B0);
        float4 C = f4x4_mul_pt(M, C0);

        positions[i + 0] = A;
        positions[i + 2] = B;
        positions[i + 1] = C;

        uvs[i + 0] = f2_mul(CalcUv(s, t, A0), uvScale);
        uvs[i + 2] = f2_mul(CalcUv(s, t, B0), uvScale);
        uvs[i + 1] = f2_mul(CalcUv(s, t, C0), uvScale);

        float4 N = f4_normalize3(f4_cross3(f4_sub(C, A), f4_sub(B, A)));

        normals[i + 0] = N;
        normals[i + 2] = N;
        normals[i + 1] = N;
    }

    mesh_t* mesh = perm_calloc(sizeof(*mesh));
    mesh->length = vertCount;
    mesh->positions = positions;
    mesh->normals = normals;
    mesh->uvs = uvs;
    return mesh_create(mesh);
}

static void FlattenModel(tables_t* tables, const mmodel_t* model)
{
    ASSERT(tables);
    ASSERT(model);
    ASSERT(model->vertices);

    const quat rot = quat_angleaxis(-kPi / 2.0f, f4_v(1.0f, 0.0f, 0.0f, 0.0f));
    const float4x4 M = f4x4_trs(f4_0, rot, f4_s(0.02f));

    const char* name = model->name;
    const i32 numSurfaces = model->numsurfaces;
    const msurface_t* surfaces = model->surfaces;
    const i32* surfEdges = model->surfedges;
    const float4* vertices = model->vertices;
    const medge_t* edges = model->edges;

    table_t* table = Drawables_Get(tables);

    drawable_t drawable = { 0 };
    localtoworld_t l2w = { f4x4_id };
    translation_t translation = { f4_0 };
    rotation_t rotation = { quat_id };
    scale_t scale = { f4_1 };

    textureid_t* textures = GenTextures(surfaces, numSurfaces);

    float4* polygon = NULL;
    float4* tris = NULL;
    i32 entCount = 0;

    for (i32 i = 0; i < numSurfaces; ++i)
    {
        const msurface_t* surface = surfaces + i;
        i32 numEdges = surface->numedges;
        const i32 firstEdge = surface->firstedge;
        const mtexinfo_t* texinfo = surface->texinfo;
        const mtexture_t* mtex = texinfo->texture;

        if (numEdges <= 0 || firstEdge < 0)
        {
            continue;
        }

        drawable.material.albedo = textures[i];
        //drawable.material.rome = textures[i];

        i32 vertCount = FlattenSurface(model, surface, &tris, &polygon);
        drawable.mesh = TrisToMesh(M, surface, tris, vertCount);

        drawable.material.st = f4_v(1.0f, 1.0f, 0.0f, 0.0f);
        drawable.material.flatAlbedo = LinearToColor(f4_1);
        drawable.material.flatRome = LinearToColor(f4_v(0.5f, 1.0f, 0.0f, 0.0f));

        ++entCount;

        char namebuf[64];
        SPrintf(ARGS(namebuf), "%s_%d", name, i);
        i32 c = col_add_s(table, namebuf);

        drawable_t* drawables = table_row(table, TYPE_ARGS(drawable_t));
        drawables[c] = drawable;

        translation_t* translations = table_row(table, TYPE_ARGS(translation_t));
        translations[c] = translation;

        rotation_t* rotations = table_row(table, TYPE_ARGS(rotation_t));
        rotations[c] = rotation;

        scale_t* scales = table_row(table, TYPE_ARGS(scale_t));
        scales[c] = scale;


        ASSERT(drawables[c].mesh.handle);
    }
}

static void UpdateMaterials(tables_t* tables)
{
    table_t* table = Drawables_Get(tables);
    if (table)
    {
        u32 flatAlbedo = LinearToColor(ms_flatAlbedo);
        u32 flatRome = LinearToColor(ms_flatRome);

        const i32 len = table_width(table);
        drawable_t* drawables = table_row(table, TYPE_ARGS(drawable_t));
        for(i32 i = 0; i < len; ++i)
        {
            drawables[i].material.flatAlbedo = flatAlbedo;
            drawables[i].material.flatRome = flatRome;
        }
    }
}

static void CleanPtScene(tables_t* tables)
{
    bool dirty = false;
    camera_t camera;
    camera_get(&camera);
    dirty |= cvar_check_dirty(&cv_pt_trace);
    dirty |= cvar_check_dirty(&cv_sg_gen);
    dirty |= memcmp(&camera, &ms_ptcamera, sizeof(camera)) != 0;
    if (dirty)
    {
        ms_sampleCount = 0;
        pt_scene_del(ms_ptscene);
        ms_ptscene = pt_scene_new(tables, 5);
        ms_ptcamera = camera;
    }
}

typedef struct sgtrace_s
{
    task_t task;
    float4 origin;
    float4* radiances;
    float4* directions;
} sgtrace_t;

static void SGTraceFn(task_t* pBase, i32 begin, i32 end)
{
    sgtrace_t* task = (sgtrace_t*)pBase;
    prng_t rng = prng_get();
    ray_t ray;
    ray.ro = task->origin;
    for (i32 i = begin; i < end; ++i)
    {
        ray.rd = f4_normalize3(SampleUnitSphere(f2_rand(&rng)));
        float4 rad = pt_trace_frag(&rng, ms_ptscene, ray, 10);
        task->directions[i] = ray.rd;
        task->radiances[i] = rad;
    }
    prng_set(rng);
}

ProfileMark(pm_sgtrace, SGTrace)
static void SGTrace(tables_t* tables)
{
    if (cvar_check_dirty(&cv_sg_count))
    {
        SG_SetCount((i32)cv_sg_count.asFloat);
        SG_Generate(SG_Get(), SG_GetIntegrals(), SG_GetCount(), SGDist_Sphere);
        ms_sampleCount = 0;
    }
    if (cv_sg_gen.asFloat != 0.0f)
    {
        ProfileBegin(pm_sgtrace);
        CleanPtScene(tables);

        camera_t camera;
        camera_get(&camera);

        const i32 sampleCount = 1024;
        float4* pim_noalias directions = tmp_malloc(sizeof(directions[0]) * sampleCount);
        float4* pim_noalias radiances = tmp_malloc(sizeof(radiances[0]) * sampleCount);

        sgtrace_t* task = tmp_calloc(sizeof(*task));
        task->origin = camera.position;
        task->directions = directions;
        task->radiances = radiances;

        task_submit((task_t*)task, SGTraceFn, sampleCount);
        task_sys_schedule();
        task_await((task_t*)task);

        SG_t* sgs = SG_Get();
        float* weights = SG_GetWeights();
        const float* integrals = SG_GetIntegrals();
        const i32 sgCount = SG_GetCount();

        const i32 s = ms_sampleCount;
        for (i32 i = 0; i < sampleCount; ++i)
        {
            SG_Accumulate(s + i, directions[i], radiances[i], sgs, weights, integrals, sgCount);
        }
        ms_sampleCount += sampleCount;

        ProfileEnd(pm_sgtrace);
    }
}

ProfileMark(pm_PathTrace, PathTrace)
static task_t* PathTrace(tables_t* tables)
{
    task_t* task = NULL;
    if (cv_pt_trace.asFloat != 0.0f)
    {
        ProfileBegin(pm_PathTrace);

        CleanPtScene(tables);

        pt_scene_t* scene = ms_ptscene;
        if (scene)
        {
            u32 flatAlbedo = LinearToColor(ms_flatAlbedo);
            u32 flatRome = LinearToColor(ms_flatRome);
            for (i32 i = 0; i < scene->matCount; ++i)
            {
                scene->materials[i].flatAlbedo = flatAlbedo;
                scene->materials[i].flatRome = flatRome;
                scene->materials[i].rome = (textureid_t) { 0 };
            }
        }

        framebuf_t* fbuf = &(ms_buffers[ms_iFrame & 1]);

        ms_trace.bounces = i1_clamp((i32)cv_pt_bounces.asFloat, 0, 100);
        ms_trace.sampleWeight = 1.0f / ++ms_sampleCount;
        ms_trace.camera = &ms_ptcamera;
        ms_trace.dstImage = fbuf->light;
        ms_trace.imageSize = i2_v(fbuf->width, fbuf->height);
        ms_trace.scene = ms_ptscene;

        task = pt_trace(&ms_trace);
        task_sys_schedule();

        ProfileEnd(pm_PathTrace);
    }
    return task;
}

void render_sys_init(void)
{
    cvar_reg(&cv_pt_trace);
    cvar_reg(&cv_pt_bounces);
    cvar_reg(&cv_sg_gen);
    cvar_reg(&cv_sg_count);

    ms_iFrame = 0;
    framebuf_create(&(ms_buffers[0]), kDrawWidth, kDrawHeight);
    framebuf_create(&(ms_buffers[1]), kDrawWidth, kDrawHeight);
    screenblit_init(kDrawWidth, kDrawHeight);

    ms_tonemapper = TMap_Reinhard;
    ms_toneParams = Tonemap_DefParams();
    ms_clearColor = f4_v(0.01f, 0.012f, 0.022f, 0.0f);
    ms_flatAlbedo = f4_s(1.0f);
    ms_flatRome = f4_v(0.5f, 1.0f, 0.0f, 0.0f);

    tables_t* tables = tables_main();

    Drawables_New(tables);

    table_t* lights = tables_add_s(tables, "Lights");
    table_add_h(lights, TYPE_ARGS(radiance_t));
    table_add_h(lights, TYPE_ARGS(translation_t));
    table_add_h(lights, TYPE_ARGS(rotation_t));

    table_t* cameras = tables_add_s(tables, "Cameras");
    table_add_h(cameras, TYPE_ARGS(camera_t));

    table_t* meshes = tables_add_s(tables, "Meshes");
    table_add_h(meshes, TYPE_ARGS(meshid_t));

    table_t* textures = tables_add_s(tables, "Textures");
    table_add_h(textures, TYPE_ARGS(textureid_t));

    asset_t mapasset = { 0 };
    if (asset_get("maps/start.bsp", &mapasset))
    {
        mmodel_t* model = LoadModel(mapasset.pData, EAlloc_Temp);
        StrCpy(ARGS(model->name), "maps/start.bsp");
        FlattenModel(tables, model);
        FreeModel(model);
    }

    if (lights_pt_count() == 0)
    {
        lights_add_pt((pt_light_t) { f4_v(0.0f, 0.0f, 0.0f, 1.0f), f4_s(3.0f) });
    }

    task_t* compose = Drawables_TRS(tables);
    task_sys_schedule();
    task_await(compose);

    SG_SetCount((i32)cv_sg_count.asFloat);
    SG_Generate(SG_Get(), SG_GetIntegrals(), SG_GetCount(), SGDist_Sphere);

    CleanPtScene(tables);
}

ProfileMark(pm_update, render_sys_update)
void render_sys_update(void)
{
    ProfileBegin(pm_update);

    ++ms_iFrame;
    const i32 iFront = ms_iFrame & 1;
    const i32 iBack = (ms_iFrame + 1) & 1;

    framebuf_t* frontBuf = &(ms_buffers[iFront]);
    framebuf_t* backBuf = &(ms_buffers[iBack]);

    tables_t* tables = tables_main();
    camera_t camera;
    camera_get(&camera);

    UpdateMaterials(tables);
    SGTrace(tables);

    task_t* xformTask = Drawables_TRS(tables);
    task_sys_schedule();
    task_await(xformTask);

    task_t* boundsTask = Drawables_Bounds(tables);
    task_sys_schedule();
    task_await(boundsTask);

    task_t* cullTask = Drawables_Cull(tables, &camera, backBuf);
    task_sys_schedule();
    task_await(cullTask);

    task_t* drawTask = PathTrace(tables);
    if (!drawTask)
    {
        task_t* vertexTask = Drawables_Vertex(tables, &camera);
        task_sys_schedule();
        ClearTile(frontBuf, ms_clearColor, camera.nearFar.y);
        task_await(vertexTask);

        drawTask = Drawables_Fragment(tables, frontBuf, backBuf);
        task_sys_schedule();
    }

    task_await(drawTask);
    task_t* resolveTask = ResolveTile(frontBuf, ms_tonemapper, ms_toneParams);

    task_sys_schedule();
    task_await(resolveTask);
    screenblit_blit(frontBuf->color, kDrawWidth, kDrawHeight);

    ProfileEnd(pm_update);
}

void render_sys_shutdown(void)
{
    task_sys_schedule();

    pt_scene_del(ms_ptscene);
    ms_ptscene = NULL;

    screenblit_shutdown();
    framebuf_destroy(&(ms_buffers[0]));
    framebuf_destroy(&(ms_buffers[1]));

    Drawables_Del(tables_main());
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

            igColorEdit3("Albedo", &ms_flatAlbedo.x, ldrPicker);
            igSliderFloat("Roughness", &ms_flatRome.x, 0.0f, 1.0f);
            igSliderFloat("Occlusion", &ms_flatRome.y, 0.0f, 1.0f);
            igSliderFloat("Metallic", &ms_flatRome.z, 0.0f, 1.0f);
            igSliderFloat("Emission", &ms_flatRome.w, 0.0f, 1.0f);

            pt_light_t light = lights_get_pt(0);
            igColorEdit3("Light Radiance", &light.rad.x, hdrPicker);
            lights_set_pt(0, light);

            igUnindent(0.0f);
        }

        if (igCollapsingHeader1("Culling Stats"))
        {
            table_t* table = Drawables_Get(tables_main());
            if (table)
            {
                const i32 width = table_width(table);
                const drawable_t* drawables = table_row(table, TYPE_ARGS(drawable_t));
                const bounds_t* bounds = table_row(table, TYPE_ARGS(bounds_t));
                i32 numVisible = 0;
                for (i32 i = 0; i < width; ++i)
                {
                    if (drawables[i].tilemask)
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
                    distances[i] = sdFrusSph(frus, bounds[i].Value);
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
                    float4 sph = bounds[j].Value.value;
                    u64 tilemask = drawables[j].tilemask;
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
    return mesh_create(mesh);
}

static textureid_t GenCheckerTex(void)
{
    texture_t* albedo = perm_calloc(sizeof(*albedo));
    const i32 size = 1 << 8;
    albedo->size = i2_s(size);
    u32* pim_noalias texels = perm_malloc(sizeof(texels[0]) * size * size);

    const float4 a = f4_s(1.0f);
    const float4 b = f4_s(0.01f);
    prng_t rng = prng_get();
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
    albedo->texels = texels;
    prng_set(rng);

    return texture_create(albedo);
}
