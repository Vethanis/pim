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

// ----------------------------------------------------------------------------

static void CreateEntities(meshid_t mesh, material_t material, i32 count);
static meshid_t GenSphereMesh(float r, i32 steps);
static textureid_t GenCheckerTex(void);

// ----------------------------------------------------------------------------

static framebuf_t ms_buffer;
static meshid_t ms_meshid;
static material_t ms_material;
static prng_t ms_prng;

static TonemapId ms_tonemapper;
static float4 ms_toneParams;
static float4 ms_clearColor;

// ----------------------------------------------------------------------------

void render_sys_init(void)
{
    ms_prng = prng_create();
    ms_tonemapper = TMap_Hable;
    ms_toneParams = Tonemap_DefParams();
    ms_clearColor = f4_v(0.01f, 0.012f, 0.022f, 0.0f);
    *LightDir() = f3_normalize(f3_1);
    *LightRad() = f3_v(9.593f, 7.22f, 5.458f);
    *DiffuseGI() = f3_v(0.074f, 0.074f, 0.142f);
    *SpecularGI() = f3_v(0.299f, 0.266f, 0.236f);

    framebuf_create(&ms_buffer, kDrawWidth, kDrawHeight);
    screenblit_init(kDrawWidth, kDrawHeight);
    FragmentStage_Init();

    ms_material.flatAlbedo = f4_rgba8(f4_s(1.0f));
    ms_material.flatRome = f4_rgba8(f4_v(0.143f, 1.0f, 0.0f, 0.0f));
    ms_material.st = f4_v(1.0f, 1.0f, 0.0f, 0.0f);
    ms_material.albedo = GenCheckerTex();

    ms_meshid = GenSphereMesh(1.0, 16);

    CreateEntities(ms_meshid, ms_material, 64);
}

ProfileMark(pm_update, render_sys_update)
void render_sys_update(void)
{
    ProfileBegin(pm_update);

    camera_t camera;
    camera_get(&camera);

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

    task_t* resolveTask = ResolveTile(&ms_buffer, ms_tonemapper, ms_toneParams);
    task_sys_schedule();
    task_await(resolveTask);

    screenblit_blit(ms_buffer.color, kDrawWidth, kDrawHeight);

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
}

ProfileMark(pm_gui, render_sys_gui)
void render_sys_gui(bool* pEnabled)
{
    ProfileBegin(pm_gui);

    const u32 ldrPicker = ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_InputRGB;
    const u32 hdrPicker = ImGuiColorEditFlags_Float | ImGuiColorEditFlags_InputRGB;

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

            float4 flatAlbedo = rgba8_f4(ms_material.flatAlbedo);
            igColorEdit3("Albedo", &flatAlbedo.x, ldrPicker);
            ms_material.flatAlbedo = f4_rgba8(flatAlbedo);

            float4 flatRome = rgba8_f4(ms_material.flatRome);
            igSliderFloat("Roughness", &flatRome.x, 0.0f, 1.0f);
            igSliderFloat("Occlusion", &flatRome.x, 0.0f, 1.0f);
            igSliderFloat("Metallic", &flatRome.z, 0.0f, 1.0f);
            igSliderFloat("Emission", &flatRome.w, 0.0f, 1.0f);
            ms_material.flatRome = f4_rgba8(flatRome);

            float3 L = *LightDir();
            igSliderFloat3("Light Dir", &L.x, -1.0f, 1.0f);
            *LightDir() = f3_normalize(L);

            igColorEdit3("Light Radiance", &LightRad()->x, hdrPicker);
            igColorEdit3("Diffuse GI", &DiffuseGI()->x, hdrPicker);
            igColorEdit3("Specular GI", &SpecularGI()->x, hdrPicker);
            igUnindent(0.0f);
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
            float radius = 0.5f * f4_distance3(lo, hi);

            bounds_t bounds = { center.x, center.y, center.z, radius };
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
    position_t position = { 0.0f, 0.0f, 0.0f };
    void* rows[CompId_COUNT] = { 0 };
    rows[CompId_Position] = &position;
    rows[CompId_LocalToWorld] = &localToWorld;
    rows[CompId_Drawable] = &drawable;
    rows[CompId_Bounds] = &bounds;

    drawable.mesh = mesh;
    drawable.material = material;
    bounds = CalcMeshBounds(mesh);

    prng_t rng = ms_prng;
    compflag_t all = compflag_create(4, CompId_Position, CompId_LocalToWorld, CompId_Drawable, CompId_Bounds);
    //compflag_t some = compflag_create(1, CompId_Position);

    const float side = sqrtf((float)count);
    const float stride = 3.0f;

    float x = 0.0f;
    float z = 0.0f;
    for (i32 i = 0; i < count; ++i)
    {
        position.Value.x = stride * x;
        position.Value.z = stride * z;
        position.Value.y = 0.0f;
        ecs_create(all, rows);

        x += 1.0f;
        if (x >= side)
        {
            x = 0.0f;
            z += 1.0f;
        }
    }
    ms_prng = rng;
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
    albedo->width = size;
    albedo->height = size;
    u32* pim_noalias texels = perm_malloc(sizeof(texels[0]) * size * size);

    prng_t rng = ms_prng;
    for (i32 y = 0; y < size; ++y)
    {
        for (i32 x = 0; x < size; ++x)
        {
            const i32 i = x + y * size;
            bool c0 = (x & 7) < 4;
            bool c1 = (y & 7) < 4;
            u32 a = c1 ? 0xffffffff : 0;
            u32 b = ~a;
            texels[i] = c0 ? a : b;
        }
    }
    albedo->texels = texels;
    ms_prng = rng;

    return texture_create(albedo);
}
