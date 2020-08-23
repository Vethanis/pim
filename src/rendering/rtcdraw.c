#include "rendering/rtcdraw.h"
#include "rendering/librtc.h"
#include "math/float4_funcs.h"
#include "math/float2_funcs.h"
#include "math/float4x4_funcs.h"
#include "math/lighting.h"
#include "math/frustum.h"
#include "math/sampling.h"
#include "math/atmosphere.h"
#include "math/ambcube.h"
#include "math/sh.h"
#include "math/sphgauss.h"
#include "math/sdf.h"
#include "math/box.h"

#include "rendering/framebuffer.h"
#include "rendering/sampler.h"
#include "rendering/camera.h"
#include "rendering/drawable.h"
#include "rendering/lights.h"
#include "rendering/lightmap.h"
#include "rendering/cubemap.h"
#include "rendering/mesh.h"
#include "rendering/material.h"

#include "common/cvar.h"
#include "common/profiler.h"
#include "common/console.h"
#include "common/fnv1a.h"
#include "common/profiler.h"

#include "threading/task.h"
#include "allocator/allocator.h"

#include <string.h>

#define kFroxelResX     16
#define kFroxelResY     8
#define kFroxelResZ     32
#define kFroxelCount    (kFroxelResX*kFroxelResY*kFroxelResZ)

typedef struct lightlist_s
{
    i32* pim_noalias ptr;
    i32 len;
} lightlist_t;

typedef struct froxels_s
{
    frusbasis_t basis;
    lightlist_t lights[kFroxelCount];
} froxels_t;

typedef struct drawhash_s
{
    u64 meshes;
    u64 materials;
    u64 translations;
    u64 rotations;
    u64 scales;
} drawhash_t;

typedef struct world_s
{
    RTCDevice device;
    RTCScene scene;
    i32 numDrawables;
    i32 numLights;
    drawhash_t drawablesHash;
    u64 lightHash;
    Cubemap* sky;
    froxels_t froxels;
} world_t;

static world_t ms_world;

static drawhash_t HashDrawables(void);
static void CreateScene(world_t* world);
static void DestroyScene(world_t* world);
static void UpdateScene(world_t* world);
static bool AddDrawable(
    world_t* world,
    u32 geomId,
    meshid_t meshid,
    material_t mat,
    float4 translation,
    quat rotation,
    float4 scale);
static bool AddLight(world_t* world, u32 geomId, float4 center, float radius);
static void UpdateLights(world_t* world);
static void DrawScene(
    world_t* world,
    framebuf_t* target,
    const camera_t* camera);
static void ClusterLights(
    world_t* world,
    framebuf_t* target,
    const camera_t* camera);

void RtcDrawInit(void)
{
    if (rtc_init())
    {
        ms_world.device = rtc.NewDevice(NULL);
        ASSERT(ms_world.device);
    }
}

void RtcDrawShutdown(void)
{
    if (ms_world.device)
    {
        DestroyScene(&ms_world);
        rtc.ReleaseDevice(ms_world.device);
        ms_world.device = NULL;
    }
}

ProfileMark(pm_rtcdraw, RtcDraw)
void RtcDraw(framebuf_t* target, const camera_t* camera)
{
    world_t* world = &ms_world;
    if (!world->device)
    {
        return;
    }
    ProfileBegin(pm_rtcdraw);
    UpdateScene(world);
    DrawScene(world, target, camera);
    ProfileEnd(pm_rtcdraw);
}

static drawhash_t HashDrawables(void)
{
    const drawables_t* drawables = drawables_get();
    const i32 count = drawables->count;
    const meshid_t* meshes = drawables->meshes;
    const material_t* materials = drawables->materials;
    const float4* translations = drawables->translations;
    const quat* rotations = drawables->rotations;
    const float4* scales = drawables->scales;

    drawhash_t hash = { 0 };
    hash.meshes = Fnv64Bytes(meshes, sizeof(meshes[0]) * count, Fnv64Bias);
    hash.materials = Fnv64Bytes(materials, sizeof(materials[0]) * count, Fnv64Bias);
    hash.translations = Fnv64Bytes(translations, sizeof(translations[0]) * count, Fnv64Bias);
    hash.rotations = Fnv64Bytes(rotations, sizeof(rotations[0]) * count, Fnv64Bias);
    hash.scales = Fnv64Bytes(scales, sizeof(scales[0]) * count, Fnv64Bias);
    return hash;
}

static void CreateDrawables(world_t* world)
{
    ASSERT(world->scene);

    const drawables_t* drawables = drawables_get();
    const i32 numDrawables = drawables->count;
    const meshid_t* meshes = drawables->meshes;
    const material_t* materials = drawables->materials;
    const float4* translations = drawables->translations;
    const quat* rotations = drawables->rotations;
    const float4* scales = drawables->scales;

    for (i32 i = 0; i < numDrawables; ++i)
    {
        AddDrawable(
            world,
            i,
            meshes[i],
            materials[i],
            translations[i],
            rotations[i],
            scales[i]);
    }

    world->numDrawables = numDrawables;
    world->drawablesHash = HashDrawables();
}

static u64 HashLights(void)
{
    const lights_t* lights = lights_get();
    const i32 numLights = lights->ptCount;
    const pt_light_t* ptLights = lights->ptLights;
    return Fnv64Bytes(ptLights, sizeof(ptLights[0]) * numLights, Fnv64Bias);
}

static void CreateLights(world_t* world)
{
    ASSERT(world->scene);

    const i32 numDrawables = world->numDrawables;
    const lights_t* lights = lights_get();
    const i32 numLights = lights->ptCount;
    const pt_light_t* ptLights = lights->ptLights;

    for (i32 i = 0; i < numLights; ++i)
    {
        AddLight(world, numDrawables + i, ptLights[i].pos, ptLights[i].rad.w);
    }

    world->numLights = numLights;
    world->lightHash = HashLights();
}

static void DestroyLights(world_t* world)
{
    const i32 numDrawables = world->numDrawables;
    const i32 numLights = world->numLights;
    RTCScene scene = world->scene;
    if (scene)
    {
        for (i32 i = 0; i < numLights; ++i)
        {
            i32 id = numDrawables + i;
            RTCGeometry geom = rtc.GetGeometry(scene, id);
            rtc.DetachGeometry(scene, id);
            if (geom)
            {
                rtc.ReleaseGeometry(geom);
            }
        }
    }
}

static void CreateScene(world_t* world)
{
    ASSERT(world->device);

    RTCScene scene = rtc.NewScene(world->device);
    world->scene = scene;
    ASSERT(scene);

    CreateDrawables(world);
    CreateLights(world);

    rtc.CommitScene(scene);
}

static void DestroyScene(world_t* world)
{
    if (world->scene)
    {
        rtc.ReleaseScene(world->scene);
        world->scene = NULL;
    }
}

ProfileMark(pm_updatescene, UpdateScene)
static void UpdateScene(world_t* world)
{
    ProfileBegin(pm_updatescene);

    drawhash_t drawHash = HashDrawables();
    if (memcmp(&world->drawablesHash, &drawHash, sizeof(drawHash)))
    {
        world->drawablesHash = drawHash;
        DestroyScene(world);
        CreateScene(world);
    }

    UpdateLights(world);

    const u32 kSkyName = 1;
    i32 iSky = Cubemaps_Find(kSkyName);
    if (iSky != -1)
    {
        world->sky = Cubemaps_Get()->cubemaps + iSky;
    }
    else
    {
        world->sky = NULL;
    }

    ProfileEnd(pm_updatescene);
}

static bool AddDrawable(
    world_t* world,
    u32 geomId,
    meshid_t meshid,
    material_t mat,
    float4 translation,
    quat rotation,
    float4 scale)
{
    ASSERT(world->device);
    ASSERT(world->scene);

    RTCGeometry geom = NULL;

    mesh_t mesh = { 0 };
    if (!mesh_get(meshid, &mesh))
    {
        goto onfail;
    }

    geom = rtc.NewGeometry(world->device, RTC_GEOMETRY_TYPE_TRIANGLE);
    ASSERT(geom);
    if (!geom)
    {
        goto onfail;
    }

    const i32 vertCount = mesh.length;
    const i32 triCount = vertCount / 3;

    float3* dstPositions = rtc.SetNewGeometryBuffer(
        geom,
        RTC_BUFFER_TYPE_VERTEX,
        0,
        RTC_FORMAT_FLOAT3,
        sizeof(float3),
        vertCount);
    ASSERT(dstPositions);
    if (!dstPositions)
    {
        goto onfail;
    }

    float4x4 M = f4x4_trs(translation, rotation, scale);
    const float4* srcPositions = mesh.positions;
    for (i32 i = 0; i < vertCount; ++i)
    {
        float4 position = f4x4_mul_pt(M, srcPositions[i]);
        dstPositions[i] = f4_f3(position);
    }

    // kind of wasteful
    i32* dstIndices = rtc.SetNewGeometryBuffer(
        geom,
        RTC_BUFFER_TYPE_INDEX,
        0,
        RTC_FORMAT_UINT3,
        sizeof(i32) * 3,
        triCount);
    ASSERT(dstIndices);
    if (!dstIndices)
    {
        goto onfail;
    }
    for (i32 i = 0; i < vertCount; ++i)
    {
        dstIndices[i] = i;
    }

    rtc.CommitGeometry(geom);
    rtc.AttachGeometryByID(world->scene, geom, geomId);
    rtc.ReleaseGeometry(geom);
    return true;

onfail:
    if (geom)
    {
        rtc.ReleaseGeometry(geom);
    }
    return false;
}

static bool AddLight(world_t* world, u32 geomId, float4 center, float radius)
{
    ASSERT(world->device);
    ASSERT(world->scene);

    RTCGeometry geom = NULL;

    geom = rtc.NewGeometry(world->device, RTC_GEOMETRY_TYPE_SPHERE_POINT);
    ASSERT(geom);
    if (!geom)
    {
        goto onfail;
    }

    float4* dstPositions = rtc.SetNewGeometryBuffer(
        geom,
        RTC_BUFFER_TYPE_VERTEX,
        0,
        RTC_FORMAT_FLOAT4,
        sizeof(float4),
        1);
    ASSERT(dstPositions);
    if (!dstPositions)
    {
        goto onfail;
    }
    center.w = radius;
    dstPositions[0] = center;

    rtc.CommitGeometry(geom);
    rtc.AttachGeometryByID(world->scene, geom, geomId);
    rtc.ReleaseGeometry(geom);
    return true;

onfail:
    if (geom)
    {
        rtc.ReleaseGeometry(geom);
    }
    return false;
}

static void UpdateLights(world_t* world)
{
    ASSERT(world->device);
    ASSERT(world->scene);

    if (HashLights() != world->lightHash)
    {
        DestroyLights(world);
        CreateLights(world);
        rtc.CommitScene(world->scene);
    }
}

typedef enum
{
    hit_nothing = 0,
    hit_triangle,
    hit_light,

    hit_COUNT
} hittype_t;

typedef struct rayhit_s
{
    float4 wuvt;
    float4 normal;
    hittype_t type;
    i32 iDrawable;
    i32 iVert;
} rayhit_t;

static struct RTCRay VEC_CALL RtcNewRay(float4 ro, float4 rd, float tNear, float tFar)
{
    ASSERT(tFar > tNear);
    ASSERT(tNear >= 0.0f);
    struct RTCRay rtcRay = { 0 };
    rtcRay.org_x = ro.x;
    rtcRay.org_y = ro.y;
    rtcRay.org_z = ro.z;
    rtcRay.tnear = tNear;
    rtcRay.dir_x = rd.x;
    rtcRay.dir_y = rd.y;
    rtcRay.dir_z = rd.z;
    rtcRay.tfar = tFar;
    rtcRay.mask = -1;
    rtcRay.flags = 0;
    return rtcRay;
}

static struct RTCRayHit VEC_CALL RtcIntersect(
    RTCScene scene,
    float4 ro,
    float4 rd,
    float tNear,
    float tFar)
{
    struct RTCIntersectContext ctx;
    rtcInitIntersectContext(&ctx);
    struct RTCRayHit rayHit = { 0 };
    rayHit.ray = RtcNewRay(ro, rd, tNear, tFar);
    rayHit.hit.primID = RTC_INVALID_GEOMETRY_ID; // triangle index
    rayHit.hit.geomID = RTC_INVALID_GEOMETRY_ID; // object id
    rayHit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID; // instance id
    rtc.Intersect1(scene, &ctx, &rayHit);
    return rayHit;
}

static rayhit_t VEC_CALL TraceRay(
    world_t* world,
    float4 ro,
    float4 rd,
    float tNear,
    float tFar)
{
    rayhit_t hit = { 0 };
    hit.wuvt.w = tFar;

    RTCRayHit rtcHit = RtcIntersect(world->scene, ro, rd, tNear, tFar);

    bool hitNothing =
        (rtcHit.hit.geomID == RTC_INVALID_GEOMETRY_ID) ||
        (rtcHit.ray.tfar <= 0.0f);

    if (hitNothing)
    {
        hit.type = hit_nothing;
        hit.iDrawable = -1;
        hit.iVert = -1;
        return hit;
    }

    ASSERT(rtcHit.hit.primID != RTC_INVALID_GEOMETRY_ID);

    hit.normal = f4_v(rtcHit.hit.Ng_x, rtcHit.hit.Ng_y, rtcHit.hit.Ng_z, 0.0f);
    hit.normal = f4_normalize3(hit.normal);
    float u = f1_saturate(rtcHit.hit.u);
    float v = f1_saturate(rtcHit.hit.v);
    float w = f1_saturate(1.0f - (u + v));
    float t = rtcHit.ray.tfar;
    hit.wuvt = f4_v(w, u, v, t);

    const u32 numDrawables = world->numDrawables;
    if (rtcHit.hit.geomID >= numDrawables)
    {
        hit.type = hit_light;
        hit.iDrawable = rtcHit.hit.geomID - numDrawables;
        hit.iVert = rtcHit.hit.primID;
        ASSERT(hit.iDrawable < world->numLights);
    }
    else
    {
        hit.type = hit_triangle;
        hit.iDrawable = rtcHit.hit.geomID;
        hit.iVert = rtcHit.hit.primID * 3;
        ASSERT((u32)hit.iDrawable < numDrawables);
    }
    ASSERT(hit.iDrawable >= 0);
    ASSERT(hit.iVert >= 0);

    return hit;
}

pim_inline i32 VEC_CALL PositionToFroxel(frusbasis_t basis, float4 P)
{
    float3 coord = unproj_pt(basis.eye, basis.right, basis.up, basis.fwd, basis.slope, P);
    coord.x = f1_unorm(coord.x) * kFroxelResX;
    coord.y = f1_unorm(coord.y) * kFroxelResY;
    coord.z = UnlerpZ(basis.zNear, basis.zFar, coord.z)  * kFroxelResZ;
    i32 x = i1_clamp((i32)coord.x, 0, kFroxelResX - 1);
    i32 y = i1_clamp((i32)coord.y, 0, kFroxelResY - 1);
    i32 z = i1_clamp((i32)coord.z, 0, kFroxelResZ - 1);
    i32 i = x + y * kFroxelResX + z * kFroxelResX * kFroxelResY;
    return i;
}

pim_inline lightlist_t VEC_CALL GetLightList(const froxels_t* froxels, float4 P)
{
    i32 i = PositionToFroxel(froxels->basis, P);
    return froxels->lights[i];
}

typedef struct task_DrawScene
{
    task_t task;
    framebuf_t* target;
    const camera_t* camera;
    world_t* world;
} task_DrawScene;

static void DrawSceneFn(task_t* pbase, i32 begin, i32 end)
{
    task_DrawScene* task = (task_DrawScene*)pbase;
    framebuf_t* target = task->target;
    const camera_t* camera = task->camera;
    world_t* world = task->world;
    RTCScene scene = world->scene;
    const Cubemap* pim_noalias sky = world->sky;
    const froxels_t* pim_noalias froxels = &world->froxels;

    const drawables_t* drawables = drawables_get();
    const i32 drawableCount = drawables->count;
    const meshid_t* pim_noalias meshids = drawables->meshes;
    const material_t* pim_noalias materials = drawables->materials;
    const float4x4* pim_noalias matrices = drawables->matrices;
    const float3x3* pim_noalias invMatrices = drawables->invMatrices;
    const lm_uvs_t* pim_noalias lmUvList = drawables->lmUvs;
    const pt_light_t* pim_noalias lights = lights_get()->ptLights;

    const lmpack_t* lmpack = lmpack_get();

    const int2 size = { target->width, target->height };
    const float2 rcpSize = { 1.0f / size.x, 1.0f / size.y };
    const float aspect = (float)size.x / size.y;
    const float fov = f1_radians(camera->fovy);
    const float tNear = camera->zNear;
    const float tFar = camera->zFar;

    const float4 ro = camera->position;
    const quat rotation = camera->rotation;
    const float4 right = quat_right(rotation);
    const float4 up = quat_up(rotation);
    const float4 fwd = quat_fwd(rotation);
    const float2 slope = proj_slope(fov, aspect);

    float4* pim_noalias dstLight = target->light;

    prng_t rng = prng_get();
    for (i32 iTexel = begin; iTexel < end; ++iTexel)
    {
        dstLight[iTexel] = f4_0;

        const i32 x = iTexel % size.x;
        const i32 y = iTexel / size.x;
        float2 coord = { (x + 0.5f) * rcpSize.x, (y + 0.5f) * rcpSize.y };
        coord = f2_snorm(coord);
        const float4 rd = proj_dir(right, up, fwd, slope, coord);

        const rayhit_t hit = TraceRay(world, ro, rd, tNear, tFar);
        if (hit.type == hit_nothing)
        {
            continue;
        }

        if (hit.type == hit_light)
        {
            pt_light_t light = lights_get_pt(hit.iVert);
            float VoN = f4_dotsat(f4_neg(rd), hit.normal);
            float4 rad = f4_mulvs(light.rad, VoN);
            dstLight[iTexel] = rad;
            continue;
        }

        const material_t material = materials[hit.iDrawable];
        if (material.flags & matflag_sky)
        {
            if (sky)
            {
                float3 atmos = Cubemap_ReadColor(sky, rd);
                dstLight[iTexel] = f3_f4(atmos, 0.0f);
            }
            continue;
        }

        mesh_t mesh = { 0 };
        if (!mesh_get(meshids[hit.iDrawable], &mesh))
        {
            continue;
        }

        ASSERT(hit.iVert >= 0);
        ASSERT(hit.iVert < mesh.length);

        const i32 a = hit.iVert + 0;
        const i32 b = hit.iVert + 1;
        const i32 c = hit.iVert + 2;

        const float4 V = f4_neg(rd);
        const float3x3 IM = invMatrices[hit.iDrawable];
        const float4 N0 = f4_normalize3(f4_blend(
            f3x3_mul_col(IM, mesh.normals[a]),
            f3x3_mul_col(IM, mesh.normals[b]),
            f3x3_mul_col(IM, mesh.normals[c]),
            hit.wuvt));
        const float3x3 TBN = NormalToTBN(N0);
        const float4 P = f4_add(f4_add(ro, f4_mulvs(rd, hit.wuvt.w)), f4_mulvs(N0, kMilli));
        const float2 uv = f2_blend(mesh.uvs[a], mesh.uvs[b], mesh.uvs[c], hit.wuvt);

        float4 albedo = ColorToLinear(material.flatAlbedo);
        texture_t tex;
        if (texture_get(material.albedo, &tex))
        {
            albedo = f4_mul(albedo, UvBilinearWrap_c32(tex.texels, tex.size, uv));
        }
        float4 rome = ColorToLinear(material.flatRome);
        if (texture_get(material.rome, &tex))
        {
            rome = f4_mul(rome, UvBilinearWrap_c32(tex.texels, tex.size, uv));
        }
        float4 N = N0;
        if (texture_get(material.normal, &tex))
        {
            float4 Nts = UvBilinearWrap_dir8(tex.texels, tex.size, uv);
            N = TbnToWorld(TBN, Nts);
        }

        float4 lighting = f4_0;

        // emission
        lighting = f4_add(lighting, UnpackEmission(albedo, rome.w));

        // direct light
        const lightlist_t llist = GetLightList(froxels, P);
        for (i32 iList = 0; iList < llist.len; ++iList)
        {
            i32 iLight = llist.ptr[iList];
            pt_light_t light = lights[iLight];
            float4 direct = EvalPointLight(V, P, N, albedo, rome.x, rome.z, light.pos, light.rad);
            lighting = f4_add(lighting, direct);
        }

        // indirect light
        {
            const lm_uvs_t lmUvs = lmUvList[hit.iDrawable];
            if (lmUvs.length > c)
            {
                i32 lmIndex = lmUvs.indices[a / 3];
                if (lmIndex >= 0)
                {
                    ASSERT(lmIndex < lmpack->lmCount);
                    const lightmap_t lmap = lmpack->lightmaps[lmIndex];
                    float2 lmUv = f2_blend(
                        lmUvs.uvs[a],
                        lmUvs.uvs[b],
                        lmUvs.uvs[c],
                        hit.wuvt);
                    lmUv = f2_subvs(lmUv, 0.5f / lmap.size);
                    float4 probe[kGiDirections];
                    float4 axii[kGiDirections];
                    for (i32 i = 0; i < kGiDirections; ++i)
                    {
                        probe[i] = UvBilinearClamp_f4(lmap.probes[i], i2_s(lmap.size), lmUv);
                        float4 ax = lmpack->axii[i];
                        float sharpness = ax.w;
                        ax = TbnToWorld(TBN, ax);
                        ax.w = sharpness;
                        axii[i] = ax;
                    }
                    float4 R = f4_normalize3(f4_reflect3(rd, N));
                    float4 diffuseGI = SGv_Irradiance(kGiDirections, axii, probe, N);
                    float4 specularGI = SGv_Eval(kGiDirections, axii, probe, R);
                    float4 indirect = IndirectBRDF(
                        V,
                        N,
                        diffuseGI,
                        specularGI,
                        albedo,
                        rome.x,
                        rome.z,
                        rome.y);
                    lighting = f4_add(lighting, indirect);
                }
            }
        }

        dstLight[iTexel] = lighting;
    }
    prng_set(rng);
}

ProfileMark(pm_drawscene, DrawScene)
static void DrawScene(
    world_t* world,
    framebuf_t* target,
    const camera_t* camera)
{
    ClusterLights(world, target, camera);

    ProfileBegin(pm_drawscene);
    task_DrawScene* task = tmp_calloc(sizeof(*task));
    task->target = target;
    task->camera = camera;
    task->world = world;
    task_run(&task->task, DrawSceneFn, target->width * target->height);
    ProfileEnd(pm_drawscene);
}

typedef struct task_ClusterLights
{
    task_t task;
    camera_t camera;
    froxels_t* froxels;
    const lights_t* lights;
} task_ClusterLights;

static void ClusterLightsFn(task_t* pbase, i32 begin, i32 end)
{
    task_ClusterLights* task = (task_ClusterLights*)pbase;
    froxels_t* pim_noalias froxels = task->froxels;
    const lights_t* pim_noalias lights = task->lights;
    const pt_light_t* pim_noalias ptLights = lights->ptLights;
    const i32 ptCount = lights->ptCount;
    const camera_t camera = task->camera;
    const frusbasis_t basis = froxels->basis;

    const float rcpResX = 1.0f / kFroxelResX;
    const float rcpResY = 1.0f / kFroxelResY;
    const float rcpResZ = 1.0f / kFroxelResZ;
    for (i32 iFroxel = begin; iFroxel < end; ++iFroxel)
    {
        i32 z = iFroxel / (kFroxelResX * kFroxelResY);
        i32 rem = iFroxel % (kFroxelResX * kFroxelResY);
        i32 y = rem / kFroxelResX;
        i32 x = rem % kFroxelResX;
        float2 lo = { (x + 0) * rcpResX, (y + 0) * rcpResY };
        float2 hi = { (x + 1) * rcpResX, (y + 1) * rcpResY };
        lo = f2_snorm(lo);
        hi = f2_snorm(hi);
        float zNear = LerpZ(basis.zNear, basis.zFar, (z + 0) * rcpResZ);
        float zFar = LerpZ(basis.zNear, basis.zFar, (z + 1) * rcpResZ);
        frus_t frus;
        camera_subfrustum(&camera, &frus, lo, hi, zNear, zFar);

        lightlist_t list = { 0 };
        for (i32 iLight = 0; iLight < ptCount; ++iLight)
        {
            const sphere_t sph = { ptLights[iLight].pos };
            bool overlaps = false;
            if (sdFrusSph(frus, sph) <= 0.0f)
            {
                list.len++;
                list.ptr = tmp_realloc(list.ptr, sizeof(list.ptr[0]) * list.len);
                list.ptr[list.len - 1] = iLight;
            }
        }
        froxels->lights[iFroxel] = list;
    }
}

pim_inline frusbasis_t VEC_CALL CameraToBasis(const camera_t* camera, i32 width, i32 height)
{
    frusbasis_t basis;
    basis.eye = camera->position;
    quat rot = camera->rotation;
    basis.right = quat_right(rot);
    basis.up = quat_up(rot);
    basis.fwd = quat_fwd(rot);
    basis.slope = proj_slope(f1_radians(camera->fovy), (float)width / (float)height);
    basis.zNear = camera->zNear;
    basis.zFar = camera->zFar;
    return basis;
}

ProfileMark(pm_ClusterLights, ClusterLights)
static void ClusterLights(
    world_t* world,
    framebuf_t* target,
    const camera_t* camera)
{
    ProfileBegin(pm_ClusterLights);

    froxels_t* froxels = &world->froxels;
    memset(froxels, 0, sizeof(*froxels));
    froxels->basis = CameraToBasis(camera, target->width, target->height);

    const lights_t* pim_noalias lights = lights_get();
    const i32 ptCount = lights->ptCount;
    if (ptCount > 0)
    {
        task_ClusterLights* task = tmp_calloc(sizeof(*task));
        task->camera = *camera;
        task->froxels = &world->froxels;
        task->lights = lights;
        task_run(&task->task, ClusterLightsFn, kFroxelCount);
    }

    ProfileEnd(pm_ClusterLights);
}
