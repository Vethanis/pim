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
#include "math/sphgauss.h"
#include "math/sampling.h"

#include "rendering/constants.h"
#include "rendering/window.h"
#include "rendering/framebuffer.h"
#include "rendering/camera.h"
#include "rendering/lights.h"
#include "rendering/transform_compose.h"
#include "rendering/clear_tile.h"
#include "rendering/vertex_stage.h"
#include "rendering/fragment_stage.h"
#include "rendering/resolve_tile.h"
#include "rendering/screenblit.h"
#include "rendering/path_tracer.h"

#include "quake/q_model.h"
#include "assets/asset_system.h"

#include <string.h>

static cvar_t cv_pathtrace = { cvart_bool, 0, "pathtrace", "0", "enable path tracing of scene" };
static cvar_t cv_ptbounces = { cvart_int, 0, "ptbounces", "10", "number of bounces in the path tracer" };
static cvar_t cv_gensg = { cvart_bool, 0, "gensg", "0", "enable spherical gaussian light accumulation" };

// ----------------------------------------------------------------------------

static meshid_t GenSphereMesh(float r, i32 steps);
static textureid_t GenCheckerTex(void);

// ----------------------------------------------------------------------------

static framebuf_t ms_buffer;
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

static ent_t* FlattenModel(mmodel_t* model, i32* entCountOut)
{
    ASSERT(model);
    ASSERT(model->vertices);

    const float scale = 0.02f; // quake maps are big
    const quat rot = quat_angleaxis(-kPi / 2.0f, f4_v(1.0f, 0.0f, 0.0f, 0.0f));
    const float4x4 M = f4x4_trs(f4_0, rot, f4_s(scale));

    const i32 numSurfaces = model->numsurfaces;
    const msurface_t* surfaces = model->surfaces;
    const i32* surfEdges = model->surfedges;
    const float4* vertices = model->vertices;
    const medge_t* edges = model->edges;

    drawable_t drawable = { 0 };
    localtoworld_t l2w = { 0 };
    bounds_t bounds = { 0 };
    void* rows[CompId_COUNT] = { 0 };
    rows[CompId_Bounds] = &bounds;
    rows[CompId_LocalToWorld] = &l2w;
    rows[CompId_Drawable] = &drawable;

    textureid_t* textures = GenTextures(surfaces, numSurfaces);

    float4* polygon = NULL;
    float4* tris = NULL;
    i32 entCount = 0;
    ent_t* ents = NULL;

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

        const i32 vertCount = FlattenSurface(model, surface, &tris, &polygon);

        drawable.mesh = TrisToMesh(M, surface, tris, vertCount);

        drawable.material.st = f4_v(1.0f, 1.0f, 0.0f, 0.0f);
        drawable.material.flatAlbedo = LinearToColor(f4_1);
        drawable.material.flatRome = LinearToColor(f4_v(0.5f, 1.0f, 0.0f, 0.0f));

        bounds.box = mesh_calcbounds(drawable.mesh);
        l2w.Value = f4x4_id;

        ++entCount;
        ents = tmp_realloc(ents, sizeof(ents[0]) * entCount);
        ents[entCount - 1] = ecs_create(rows);
    }

    *entCountOut = entCount;
    return ents;
}

static void UpdateMaterialsFn(ecs_foreach_t* task, void** rows, const i32* indices, i32 length)
{
    drawable_t* drawables = rows[CompId_Drawable];

    u32 flatAlbedo = LinearToColor(ms_flatAlbedo);
    u32 flatRome = LinearToColor(ms_flatRome);
    for (i32 i = 0; i < length; ++i)
    {
        i32 iEnt = indices[i];
        drawables[iEnt].material.flatAlbedo = flatAlbedo;
        drawables[iEnt].material.flatRome = flatRome;
    }
}

static task_t* UpdateMaterials(void)
{
    ecs_foreach_t* task = tmp_calloc(sizeof(*task));
    ecs_foreach(task, (1 << CompId_Drawable), 0, UpdateMaterialsFn);
    return (task_t*)task;
}

static void CleanPtScene(void)
{
    bool dirty = false;
    camera_t camera;
    camera_get(&camera);
    dirty |= cvar_check_dirty(&cv_pathtrace);
    dirty |= cvar_check_dirty(&cv_gensg);
    dirty |= memcmp(&camera, &ms_ptcamera, sizeof(camera)) != 0;
    if (dirty)
    {
        ms_sampleCount = 0;
        pt_scene_del(ms_ptscene);
        ms_ptscene = pt_scene_new(5);
        ms_ptcamera = camera;
    }
}

void render_sys_init(void)
{
    cvar_reg(&cv_pathtrace);
    cvar_reg(&cv_ptbounces);
    cvar_reg(&cv_gensg);

    ms_prng = prng_create();
    ms_tonemapper = TMap_Reinhard;
    ms_toneParams = Tonemap_DefParams();
    ms_clearColor = f4_v(0.01f, 0.012f, 0.022f, 0.0f);
    *DiffuseGI() = f4_s(0.01f);
    *SpecularGI() = f4_s(0.01f);

    framebuf_create(&ms_buffer, kDrawWidth, kDrawHeight);
    screenblit_init(kDrawWidth, kDrawHeight);
    FragmentStage_Init();

    ms_flatAlbedo = f4_s(1.0f);
    ms_flatRome = f4_v(0.5f, 1.0f, 0.0f, 0.0f);

    i32 entCount = 0;
    ent_t* ents = NULL;

    asset_t mapasset = { 0 };
    if (asset_get("maps/start.bsp", &mapasset))
    {
        mmodel_t* model = LoadModel(mapasset.pData, EAlloc_Temp);
        ents = FlattenModel(model, &entCount);
        FreeModel(model);
    }

    if (lights_pt_count() == 0)
    {
        lights_add_pt((pt_light_t) { f4_v(0.0f, 0.0f, 0.0f, 1.0f), f4_s(3.0f) });
    }

    task_t* compose = TransformCompose();
    task_sys_schedule();
    task_await(compose);

    SG_SetCount(9);
    SG_Generate(SG_Get(), SG_GetCount(), SGDist_Sphere);

    CleanPtScene();
}

ProfileMark(pm_sgtrace, sg_trace)
ProfileMark(pm_update, render_sys_update)
void render_sys_update(void)
{
    ProfileBegin(pm_update);

    camera_t camera;
    camera_get(&camera);

    if (cv_gensg.asFloat != 0.0f)
    {
        ProfileBegin(pm_sgtrace);
        CleanPtScene();
        ray_t ray;
        ray.ro = camera.position;
        for (i32 i = 0; i < 256; ++i)
        {
            ray.rd = f4_normalize3(SampleUnitSphere(f2_rand(&ms_prng)));
            float4 rad = pt_trace_frag(&ms_prng, ms_ptscene, ray, 10);
            SG_Accumulate(++ms_sampleCount, ray.rd, rad, SG_Get(), SG_GetCount());
        }
        ProfileEnd(pm_sgtrace);
    }

    task_t* drawTask = NULL;
    if (cv_pathtrace.asFloat != 0.0f)
    {
        CleanPtScene();

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

        ms_trace.bounces = i1_clamp((i32)cv_ptbounces.asFloat, 0, 100);
        ms_trace.sampleWeight = 1.0f / ++ms_sampleCount;
        ms_trace.camera = &ms_ptcamera;
        ms_trace.dstImage = ms_buffer.light;
        ms_trace.imageSize = i2_v(ms_buffer.width, ms_buffer.height);
        ms_trace.scene = ms_ptscene;

        drawTask = pt_trace(&ms_trace);
        task_sys_schedule();
    }
    else
    {
        task_t* xformTask = TransformCompose();
        task_t* clearTask = ClearTile(&ms_buffer, ms_clearColor, camera.nearFar.y);
        task_sys_schedule();
        task_await(xformTask);

        task_t* vertexTask = VertexStage(&ms_buffer);
        task_sys_schedule();

        task_await(vertexTask);
        drawTask = FragmentStage(&ms_buffer);
        task_sys_schedule();
    }

    task_await(drawTask);
    task_t* resolveTask = ResolveTile(&ms_buffer, ms_tonemapper, ms_toneParams);
    UpdateMaterials();
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

    pt_scene_del(ms_ptscene);
    ms_ptscene = NULL;

    screenblit_shutdown();
    framebuf_destroy(&ms_buffer);
    FragmentStage_Shutdown();
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
            igSliderFloat("Roughness", &ms_flatRome.x, 0.0f, 1.0f);
            igSliderFloat("Occlusion", &ms_flatRome.y, 0.0f, 1.0f);
            igSliderFloat("Metallic", &ms_flatRome.z, 0.0f, 1.0f);
            igSliderFloat("Emission", &ms_flatRome.w, 0.0f, 1.0f);

            pt_light_t light = lights_get_pt(0);
            igColorEdit3("Light Radiance", &light.rad.x, hdrPicker);
            lights_set_pt(0, light);
            igColorEdit3("Diffuse GI", &DiffuseGI()->x, hdrPicker);
            igColorEdit3("Specular GI", &SpecularGI()->x, hdrPicker);

            igUnindent(0.0f);
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
