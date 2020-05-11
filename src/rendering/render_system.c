#include "rendering/render_system.h"

#include "allocator/allocator.h"
#include "components/ecs.h"
#include "threading/task.h"
#include "ui/cimgui.h"

#include "common/time.h"
#include "common/cvar.h"
#include "common/profiler.h"
#include "common/console.h"

#include "math/types.h"
#include "math/int2_funcs.h"
#include "math/float4_funcs.h"
#include "math/float3_funcs.h"
#include "math/float2_funcs.h"
#include "math/float4x4_funcs.h"
#include "math/color.h"
#include "math/sdf.h"

#include "rendering/framebuffer.h"
#include "rendering/constants.h"
#include "rendering/camera.h"
#include "rendering/window.h"
#include "rendering/screenblit.h"
#include "rendering/transform_compose.h"
#include "rendering/clear_tile.h"
#include "rendering/vertex_stage.h"
#include "rendering/fragment_stage.h"
#include "rendering/resolve_tile.h"
#include "rendering/path_tracer.h"

#include "quake/q_model.h"
#include "assets/asset_system.h"

#include <string.h>

static cvar_t cv_pathtrace = { cvart_bool, 0, "pathtrace", "0", "enable path tracing of scene" };

// ----------------------------------------------------------------------------

static void CreateEntities(meshid_t mesh, material_t material, i32 count);
static task_t* SetEntityMaterials(void);
static task_t* SetLights(void);
static meshid_t GenSphereMesh(float r, i32 steps);
static textureid_t GenCheckerTex(void);

// ----------------------------------------------------------------------------

static framebuf_t ms_buffer;
static meshid_t ms_meshid;
static material_t ms_material;
static float4 ms_flatAlbedo;
static float4 ms_flatRome;
static prng_t ms_prng;

static TonemapId ms_tonemapper;
static float4 ms_toneParams;
static float4 ms_clearColor;

static pt_scene_t* ms_ptscene;
static pt_trace_t ms_trace;
static camera_t ms_ptcamera;
static i32 ms_sampleCount;

// ----------------------------------------------------------------------------

static float4* UnrollPolygon(
    float4 plane,
    float4* polygon,
    i32 length,
    i32* lenOut)
{
    float4* result = NULL;
    i32 resLen = 0;

    i32 i = 0;
    while (length >= 3)
    {
        i32 i0 = (i + 0) % length;
        i32 i1 = (i + 1) % length;
        i32 i2 = (i + 2) % length;
        float4 v0 = polygon[i0];
        float4 v1 = polygon[i1];
        float4 v2 = polygon[i2];

        resLen += 3;
        result = tmp_realloc(result, sizeof(result[0]) * resLen);
        result[resLen - 3] = v0;
        result[resLen - 2] = v1;
        result[resLen - 1] = v2;
        --length;
        for (i32 j = i1; j < length; ++j)
        {
            polygon[j] = polygon[j + 1];
        }
    }

    *lenOut = resLen;
    return result;
}

static meshid_t FlattenModel(mmodel_t* model)
{
    ASSERT(model);
    ASSERT(model->vertices);

    i32 vertCount = 0;
    float4* positions = NULL;
    i32 polyLen = 0;
    float4* polygon = NULL;
    float2* uvs = NULL;
    for (i32 i = 0; i < model->numsurfaces; ++i)
    {
        const msurface_t* surface = model->surfaces + i;
        polyLen = 0;
        for (i32 j = 0; j < surface->numedges; ++j)
        {
            const i32 e = model->surfedges[surface->firstedge + j];
            i32 v;
            if (e >= 0)
            {
                v = model->edges[e].v[0];
            }
            else
            {
                v = model->edges[-e].v[1];
            }
            ++polyLen;
            polygon = tmp_realloc(polygon, sizeof(polygon[0]) * polyLen);
            polygon[polyLen - 1] = model->vertices[v];
        }

        const float4 plane = surface->plane->Value;
        float4 s, t;
        {
            const float4 kX = { 1.0f, 0.0f, 0.0f, 0.0f };
            const float4 kY = { 0.0f, 1.0f, 0.0f, 0.0f };
            const float4 kZ = { 0.0f, 0.0f, 1.0f, 0.0f };
            float4 n = f4_abs(plane);
            float k = f4_hmax3(n);
            if (k == n.x)
            {
                s = kZ;
                t = kY;
            }
            else if (k == n.y)
            {
                s = kX;
                t = kZ;
            }
            else
            {
                s = kX;
                t = kY;
            }
        }

        i32 triLen = 0;
        float4* tris = UnrollPolygon(plane, polygon, polyLen, &triLen);
        const i32 back = vertCount;
        vertCount += triLen;
        positions = perm_realloc(positions, sizeof(positions[0]) * vertCount);
        uvs = perm_realloc(uvs, sizeof(uvs[0]) * vertCount);
        for (i32 i = 0; i < triLen; ++i)
        {
            positions[back + i] = tris[i];
            float u = f4_dot3(tris[i], s);
            float v = f4_dot3(tris[i], t);
            uvs[back + i] = f2_fmod(f2_v(u, v), f2_1);
        }
    }

    mesh_t* mesh = perm_calloc(sizeof(*mesh));
    mesh->positions = positions;
    mesh->length = vertCount;
    mesh->uvs = uvs;
    mesh->normals = perm_malloc(sizeof(mesh->normals[0]) * vertCount);
    const float scale = 0.02f; // quake maps are big
    quat rot = quat_angleaxis(-kPi / 2.0f, f4_v(1.0f, 0.0f, 0.0f, 0.0f));
    float4x4 M = f4x4_trs(f4_0, rot, f4_s(scale));
    for (i32 i = 0; (i + 3) <= vertCount; i += 3)
    {
        float4 A = mesh->positions[i];
        float4 B = mesh->positions[i + 1];
        float4 C = mesh->positions[i + 2];
        A = f4x4_mul_pt(M, A);
        B = f4x4_mul_pt(M, B);
        C = f4x4_mul_pt(M, C);
        {
            // counterclockwise
            float4 tmp = C;
            C = B;
            B = tmp;
        }
        mesh->positions[i] = A;
        mesh->positions[i + 1] = B;
        mesh->positions[i + 2] = C;
        float4 AB = f4_sub(A, B);
        float4 AC = f4_sub(A, C);
        float4 N = f4_cross3(AB, AC);
        mesh->normals[i] = N;
        mesh->normals[i + 1] = N;
        mesh->normals[i + 2] = N;
    }

    return mesh_create(mesh);
}

void render_sys_init(void)
{
    cvar_reg(&cv_pathtrace);

    ms_prng = prng_create();
    ms_tonemapper = TMap_Hable;
    ms_toneParams = Tonemap_DefParams();
    ms_clearColor = f4_v(0.01f, 0.012f, 0.022f, 0.0f);
    *LightDir() = f4_normalize3(f4_1);
    *LightRad() = f4_v(9.593f, 7.22f, 5.458f, 0.0f);
    *DiffuseGI() = f4_v(0.074f, 0.074f, 0.142f, 0.0f);
    *SpecularGI() = f4_v(0.299f, 0.266f, 0.236f, 0.0f);

    framebuf_create(&ms_buffer, kDrawWidth, kDrawHeight);
    screenblit_init(kDrawWidth, kDrawHeight);
    FragmentStage_Init();

    ms_flatAlbedo = f4_s(1.0f);
    ms_flatRome = f4_v(0.25f, 1.0f, 0.0f, 0.0f);
    ms_material.flatAlbedo = LinearToColor(ms_flatAlbedo);
    ms_material.flatRome = LinearToColor(ms_flatRome);
    ms_material.st = f4_v(1.0f, 1.0f, 0.0f, 0.0f);
    ms_material.albedo = GenCheckerTex();

    asset_t mapasset = { 0 };
    if (asset_get("maps/start.bsp", &mapasset))
    {
        mmodel_t* model = LoadModel(mapasset.pData, EAlloc_Perm);
        ms_meshid = FlattenModel(model);
        FreeModel(model);
    }

    if (ms_meshid.handle == NULL)
    {
        ms_meshid = GenSphereMesh(1.0, 8);
    }

    CreateEntities(ms_meshid, ms_material, 2);

    TransformCompose();
    SetEntityMaterials();
    task_t* setLightsTask = SetLights();
    task_sys_schedule();
    task_await(setLightsTask);

    ms_ptscene = pt_scene_new();
}

ProfileMark(pm_update, render_sys_update)
void render_sys_update(void)
{
    ProfileBegin(pm_update);

    camera_t camera;
    camera_get(&camera);

    if (cv_pathtrace.asFloat != 0.0f)
    {
        if (memcmp(&camera, &ms_ptcamera, sizeof(camera)) != 0)
        {
            ms_sampleCount = 0;
        }
        ms_ptcamera = camera;
        ms_trace.bounces = 3;
        ms_trace.sampleWeight = 1.0f / ++ms_sampleCount;
        ms_trace.camera = &ms_ptcamera;
        ms_trace.dstImage = ms_buffer.light;
        ms_trace.imageSize = i2_v(ms_buffer.width, ms_buffer.height);
        ms_trace.scene = ms_ptscene;
        task_t* traceTask = pt_trace(&ms_trace);
        task_sys_schedule();
        task_await(traceTask);
    }
    else
    {
        ms_sampleCount = 0;
        task_t* xformTask = TransformCompose();
        task_t* clearTask = ClearTile(&ms_buffer, ms_clearColor, camera.nearFar.y);
        task_sys_schedule();
        task_await(xformTask);

        task_t* vertexTask = VertexStage(&ms_buffer);
        task_sys_schedule();

        task_await(vertexTask);
        task_t* fragTask = FragmentStage(&ms_buffer);
        task_sys_schedule();

        task_await(fragTask);
    }

    task_t* resolveTask = ResolveTile(&ms_buffer, ms_tonemapper, ms_toneParams);
    task_sys_schedule();

    task_await(resolveTask);
    screenblit_blit(ms_buffer.color, kDrawWidth, kDrawHeight);

    SetEntityMaterials();
    SetLights();

    ProfileEnd(pm_update);
}

void render_sys_present(void)
{
}

void render_sys_shutdown(void)
{
    task_sys_schedule();
    screenblit_shutdown();
    framebuf_destroy(&ms_buffer);
    mesh_destroy(ms_meshid);
    texture_destroy(ms_material.albedo);
    FragmentStage_Shutdown();

    pt_scene_del(ms_ptscene);
    ms_ptscene = NULL;
}

ProfileMark(pm_gui, render_sys_gui)
void render_sys_gui(bool* pEnabled)
{
    ProfileBegin(pm_gui);

    const u32 hdrPicker = ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_InputRGB;
    const u32 ldrPicker = ImGuiColorEditFlags_Float | ImGuiColorEditFlags_InputRGB;

    if (igBegin("RenderSystem", pEnabled, 0))
    {
#if CULLING_STATS
        igText("Vert Ents Culled: %I64u", Vert_EntsCulled());
        igText("Vert Ents Drawn: %I64u", Vert_EntsDrawn());
        igText("Frag Tris Culled: %I64u", Frag_TrisCulled());
        igText("Frag Tris Drawn: %I64u", Frag_TrisDrawn());
#endif // CULLING_STATS

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
            ms_material.flatAlbedo = LinearToColor(ms_flatAlbedo);

            igSliderFloat("Roughness", &ms_flatRome.x, 0.0f, 1.0f);
            igSliderFloat("Occlusion", &ms_flatRome.y, 0.0f, 1.0f);
            igSliderFloat("Metallic", &ms_flatRome.z, 0.0f, 1.0f);
            igSliderFloat("Emission", &ms_flatRome.w, 0.0f, 1.0f);
            ms_material.flatRome = LinearToColor(ms_flatRome);

            float4 L = *LightDir();
            igSliderFloat3("Light Dir", &L.x, -1.0f, 1.0f);
            *LightDir() = f4_normalize3(L);

            igColorEdit3("Light Radiance", &LightRad()->x, hdrPicker);
            igColorEdit3("Diffuse GI", &DiffuseGI()->x, hdrPicker);
            igColorEdit3("Specular GI", &SpecularGI()->x, hdrPicker);
            igUnindent(0.0f);

            // lazy hack
            pt_scene_t* scene = ms_ptscene;
            if (scene)
            {
                for (i32 i = 0; i < scene->matCount; ++i)
                {
                    scene->materials[i] = ms_material;
                }
            }
        }
    }
    igEnd();

    ProfileEnd(pm_gui);
}

// ----------------------------------------------------------------------------

static bounds_t CalcMeshBounds(meshid_t id)
{
    mesh_t mesh = { 0 };
    if (mesh_get(id, &mesh))
    {
        const float kBig = 1 << 20;
        const float kSmall = 1.0f / (1 << 20);

        float4 lo = f4_s(kBig);
        float4 hi = f4_s(kSmall);

        const i32 len = mesh.length;
        const float4* pim_noalias positions = mesh.positions;
        for (i32 i = 0; i < len; ++i)
        {
            const float4 pos = positions[i];
            lo = f4_min(lo, pos);
            hi = f4_max(hi, pos);
        }

        if (len > 0)
        {
            float4 center = f4_lerp(lo, hi, 0.5f);
            float4 extents = f4_sub(hi, center);

            bounds_t bounds;
            bounds.box.center = center;
            bounds.box.extents = extents;
            return bounds;
        }
    }

    bounds_t bounds = { 0.0f, 0.0f, 0.0f, 0.0f };
    return bounds;
}

static void CreateEntities(meshid_t mesh, material_t material, i32 count)
{
    localtoworld_t localToWorld = { f4x4_id };
    bounds_t bounds = { 0.0f, 0.0f, 0.0f, 1.0f };
    drawable_t drawable = { 0 };
    translation_t translation = { 0.0f, 0.0f, 0.0f };
    void* rows[CompId_COUNT] = { 0 };
    rows[CompId_Translation] = &translation;
    rows[CompId_LocalToWorld] = &localToWorld;
    rows[CompId_Drawable] = &drawable;
    rows[CompId_Bounds] = &bounds;

    drawable.mesh = mesh;
    drawable.material = material;
    bounds = CalcMeshBounds(mesh);

    translation.Value.x = -1.0f;
    translation.Value.z = 0.0f;
    translation.Value.y = 0.0f;
    ecs_create(rows);
    translation.Value.x = 1.0f;
    ecs_create(rows);

    memset(rows, 0, sizeof(rows));
    light_t light;
    rotation_t rotation;
    rows[CompId_Light] = &light;
    rows[CompId_Rotation] = &rotation;
    rows[CompId_Translation] = &translation;

    light.radiance = *LightRad();
    rotation.Value = quat_lookat(*LightDir(), f4_v(0.0f, 1.0f, 0.0f, 0.0f));
    translation.Value = f4_0;
    ecs_create(rows);
}

static void SetEntityMaterialsFn(
    ecs_foreach_t* task,
    void** rows,
    const i32* indices,
    i32 length)
{
    drawable_t* pim_noalias drawables = rows[CompId_Drawable];
    const material_t mat = ms_material;
    for (i32 i = 0; i < length; ++i)
    {
        const i32 e = indices[i];
        drawables[e].material = mat;
    }
}

static task_t* SetEntityMaterials(void)
{
    ecs_foreach_t* task = tmp_calloc(sizeof(*task));
    ecs_foreach(task, 1 << CompId_Drawable, 0, SetEntityMaterialsFn);
    return (task_t*)task;
}

static void SetLightFn(
    ecs_foreach_t* task,
    void** rows,
    const i32* indices,
    i32 length)
{
    light_t* pim_noalias lights = rows[CompId_Light];
    rotation_t* pim_noalias rotations = rows[CompId_Rotation];

    const float4 lDir = *LightDir();
    const float4 lRad = *LightRad();
    const quat rotation = quat_lookat(lDir, f4_v(0.0f, 1.0f, 0.0f, 0.0f));
    for (i32 i = 0; i < length; ++i)
    {
        const i32 e = indices[i];
        lights[e].radiance = lRad;
        rotations[e].Value = rotation;
    }
}

static task_t* SetLights(void)
{
    ecs_foreach_t* task = tmp_calloc(sizeof(*task));
    ecs_foreach(
        task,
        (1 << CompId_Light) | (1 << CompId_Rotation),
        0,
        SetLightFn);
    return (task_t*)task;
}

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
    prng_t rng = ms_prng;
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
    ms_prng = rng;

    return texture_create(albedo);
}
