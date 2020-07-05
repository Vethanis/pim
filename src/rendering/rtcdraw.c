#include "rendering/rtcdraw.h"
#include "rendering/librtc.h"
#include "math/float4_funcs.h"
#include "math/float2_funcs.h"
#include "math/float4x4_funcs.h"
#include "math/lighting.h"
#include "math/frustum.h"
#include "math/sampling.h"
#include "math/atmosphere.h"
#include "math/sphgauss.h"

#include "rendering/framebuffer.h"
#include "rendering/sampler.h"
#include "rendering/camera.h"
#include "rendering/drawable.h"
#include "rendering/lights.h"
#include "rendering/lightmap.h"
#include "rendering/gi_grid.h"

#include "common/cvar.h"
#include "common/profiler.h"
#include "common/console.h"
#include "common/fnv1a.h"
#include "common/profiler.h"

#include "threading/task.h"
#include "allocator/allocator.h"

#include <string.h>

static cvar_t cv_r_sun_az = { cvart_float, 0, "r_sun_az", "0.75", "Sun Direction Azimuth" };
static cvar_t cv_r_sun_ze = { cvart_float, 0, "r_sun_ze", "0.666f", "Sun Direction Zenith" };
static cvar_t cv_r_sun_rad = { cvart_float, 0, "r_sun_rad", "1365", "Sun Irradiance" };

static cvar_t cv_r_lm_denoised = { cvart_bool, 0, "r_lm_denoised", "0", "Render denoised lightmap" };

typedef struct drawhash_s
{
    u64 meshes;
    u64 materials;
    u64 translations;
    u64 rotations;
    u64 scales;
} drawhash_t;

typedef struct lightbuf_s
{
    i32 begin;
    i32 length;
    u64 hash;
} lightbuf_t;

typedef struct world_s
{
    RTCDevice device;
    RTCScene scene;
    i32 numDrawables;
    i32 numLights;
    drawhash_t drawablesHash;
    u64 lightHash;
    float4 sunDir;
} world_t;

static world_t ms_world;

static drawhash_t HashDrawables(void);
static void CreateScene(world_t* world);
static void UpdateScene(world_t* world);
static bool AddDrawable(
    world_t* world,
    u32 geomId,
    meshid_t meshid,
    material_t mat,
    float4 translation,
    quat rotation,
    float4 scale);
static bool AddLight(world_t* world, u32 geomId, float4 posRad);
static void UpdateLights(world_t* world);
static void DrawScene(
    world_t* world,
    framebuf_t* target,
    const camera_t* camera);

void RtcDrawInit(void)
{
    cvar_reg(&cv_r_sun_az);
    cvar_reg(&cv_r_sun_ze);
    cvar_reg(&cv_r_sun_rad);
    cvar_reg(&cv_r_lm_denoised);

    if (rtc_init())
    {
        ms_world.device = rtc.NewDevice(NULL);
        ASSERT(ms_world.device);
        CreateScene(&ms_world);
    }
}

void RtcDrawShutdown(void)
{
    if (ms_world.device)
    {
        rtc.ReleaseScene(ms_world.scene);
        ms_world.scene = NULL;
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
        AddLight(world, numDrawables + i, ptLights[i].pos);
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

    float azimuth = f1_sat(cv_r_sun_az.asFloat);
    float zenith = f1_sat(cv_r_sun_ze.asFloat);
    world->sunDir = TanToWorld(
        f4_v(0.0f, 1.0f, 0.0f, 0.0f),
        SampleUnitSphere(f2_v(zenith, azimuth)));

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

static bool AddLight(world_t* world, u32 geomId, float4 posRad)
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
    dstPositions[0] = posRad;

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
    float4 N;
    hittype_t type;
    i32 iDrawable;
    i32 iVert;
} rayhit_t;

static struct RTCRay VEC_CALL RtcNewRay(float4 ro, float4 rd, float tNear, float tFar)
{
    ASSERT(tFar > tNear);
    ASSERT(tNear > 0.0f);
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

    struct RTCRayHit rtcHit = RtcIntersect(world->scene, ro, rd, tNear, tFar);
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
    float u = rtcHit.hit.u;
    float v = rtcHit.hit.v;
    float w = 1.0f - (u + v);
    float t = rtcHit.ray.tfar;
    hit.wuvt = f4_v(w, u, v, t);
    hit.N = f4_v(rtcHit.hit.Ng_x, rtcHit.hit.Ng_y, rtcHit.hit.Ng_z, 0.0f);

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
typedef struct task_DrawScene
{
    task_t task;
    framebuf_t* target;
    const camera_t* camera;
    world_t* world;
} task_DrawScene;

pim_inline float4 VEC_CALL ProbeEval(
    const float4* pim_noalias axii,
    giprobe_t probe,
    float4 dir)
{
    float4 sum = f4_0;
    for (i32 i = 0; i < kGiDirections; ++i)
    {
        float4 value = SG_Eval(axii[i], probe.Values[i], dir);
        sum = f4_add(sum, value);
    }
    return sum;
}

pim_inline float4 VEC_CALL ProbeIrrad(
    const float4* pim_noalias axii,
    giprobe_t probe,
    float4 dir)
{
    float4 sum = f4_0;
    for (i32 i = 0; i < kGiDirections; ++i)
    {
        float4 value = SG_Irradiance(axii[i], probe.Values[i], dir);
        sum = f4_add(sum, value);
    }
    return sum;
}

static float4 VEC_CALL DrawGiGrid(float4 ro, float4 rd)
{
    float4 color = f4_0;
    const gigrid_t* grid = gigrid_get();
    if (!grid->probes)
    {
        return color;
    }
    const float4* pim_noalias positions = grid->positions;
    const giprobe_t* pim_noalias probes = grid->probes;
    const float4* pim_noalias axii = grid->axii;
    const float radius = f4_hmax3(grid->rcpRange);
    const int3 size = grid->size;
    const i32 len = size.x * size.y * size.z;
    const ray_t ray = { ro, rd };
    float tMin = 1<<20;
    i32 iMin = -1;
    for (i32 i = 0; i < len; ++i)
    {
        sphere_t sphere = { positions[i] };
        sphere.value.w = radius;
        float t = isectSphere3D(ray, sphere);
        if (t <= 0.0f)
        {
            continue;
        }
        if (t < tMin)
        {
            tMin = t;
            iMin = i;
        }
    }
    if (iMin != -1)
    {
        float4 hit = f4_add(ro, f4_mulvs(rd, tMin));
        float4 N = f4_normalize3(f4_sub(hit, positions[iMin]));
        color = ProbeEval(axii, probes[iMin], N);
    }
    return color;
}

static void DrawSceneFn(task_t* pbase, i32 begin, i32 end)
{
    task_DrawScene* task = (task_DrawScene*)pbase;
    framebuf_t* target = task->target;
    const camera_t* camera = task->camera;
    world_t* world = task->world;
    RTCScene scene = world->scene;
    const float4 sunDir = world->sunDir;

    const lights_t* lights = lights_get();
    const pt_light_t* pim_noalias ptLights = lights->ptLights;
    const i32 ptLightCount = lights->ptCount;

    const drawables_t* drawables = drawables_get();
    const i32 drawableCount = drawables->count;
    const meshid_t* pim_noalias meshids = drawables->meshes;
    const material_t* pim_noalias materials = drawables->materials;
    const float4x4* pim_noalias matrices = drawables->matrices;
    const float3x3* pim_noalias invMatrices = drawables->invMatrices;
    const lm_uvs_t* pim_noalias lmUvList = drawables->lmUvs;

    const lmpack_t* lmpack = lmpack_get();
    const gigrid_t* gigrid = gigrid_get();

    const int2 size = { target->width, target->height };
    const float2 rcpSize = { 1.0f / size.x, 1.0f / size.y };
    const float aspect = (float)size.x / size.y;
    const float fov = f1_radians(camera->fovy);
    const float tNear = camera->nearFar.x;
    const float tFar = camera->nearFar.y;
    const float sunRad = cv_r_sun_rad.asFloat;
    const bool r_lm_denoised = cv_r_lm_denoised.asFloat != 0.0f;

    const float4 ro = camera->position;
    const quat rotation = camera->rotation;
    const float4 right = quat_right(rotation);
    const float4 up = quat_up(rotation);
    const float4 fwd = quat_fwd(rotation);
    const float2 slope = proj_slope(fov, aspect);

    float4* pim_noalias dstLight = target->light;
    float* pim_noalias dstDepth = target->depth;

    prng_t rng = prng_get();
    for (i32 iTexel = begin; iTexel < end; ++iTexel)
    {
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

        if (hit.wuvt.w < dstDepth[iTexel])
        {
            dstDepth[iTexel] = hit.wuvt.w;
        }
        else
        {
            continue;
        }

        if (hit.type == hit_light)
        {
            pt_light_t light = ptLights[hit.iVert];
            float VoN = f1_abs(f4_dot3(rd, hit.N));
            float4 rad = f4_mulvs(light.rad, VoN);
            dstLight[iTexel] = rad;
            continue;
        }

        const material_t material = materials[hit.iDrawable];
        if (material.flags & matflag_sky)
        {
            float3 atmos = EarthAtmosphere(
                f4_f3(ro),
                f4_f3(rd),
                f4_f3(sunDir),
                sunRad);
            dstLight[iTexel] = f3_f4(atmos, 1.0f);
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

        float4 P = f4_add(ro, f4_mulvs(rd, hit.wuvt.w));
        const float3x3 IM = invMatrices[hit.iDrawable];
        float4 Nws = f4_blend(
            f3x3_mul_col(IM, mesh.normals[a]),
            f3x3_mul_col(IM, mesh.normals[b]),
            f3x3_mul_col(IM, mesh.normals[c]),
            hit.wuvt);
        Nws = f4_normalize3(Nws);
        P = f4_add(P, f4_mulvs(Nws, kMilli));
        const float2 uv = f2_blend(mesh.uvs[a], mesh.uvs[b], mesh.uvs[c], hit.wuvt);

        const float4 albedo = material_albedo(&material, uv);
        const float4 rome = material_rome(&material, uv);
        const float4 Nts = material_normal(&material, uv);
        const float3x3 TBN = NormalToTBN(Nws);
        const float4 N = f3x3_mul_col(TBN, Nts);

        float4 lighting = f4_0;

        // emission
        lighting = f4_add(lighting, UnpackEmission(albedo, rome.w));

        // direct light
        for (i32 iLight = 0; iLight < ptLightCount; ++iLight)
        {
            const i32 kSamples = 4;
            const float kWeight = 1.0f / kSamples;
            pt_light_t light = lights_get_pt(iLight);
            SphereSA sa = SphereSA_New(light.pos, P);

            int4 valid;
            RTCRay4 occRay = { 0 };
            RTCIntersectContext ctx = { 0 };
            rtcInitIntersectContext(&ctx);

            for (i32 iSample = 0; iSample < kSamples; ++iSample)
            {
                float2 Xi = Hammersley2D(iSample, kSamples);
                float4 Lpt = SampleSphereSA(sa, Xi);

                float4 L = f4_sub(Lpt, P);
                float dist = f4_length3(L);
                L = f4_divvs(L, dist);
                bool isValid = f4_dot3(N, L) > 0.0f;

                (&valid.x)[iSample] = isValid ? -1 : 0;
                if (isValid)
                {
                    occRay.dir_x[iSample] = L.x;
                    occRay.dir_y[iSample] = L.y;
                    occRay.dir_z[iSample] = L.z;
                    occRay.mask[iSample] = -1;
                    occRay.org_x[iSample] = P.x;
                    occRay.org_y[iSample] = P.y;
                    occRay.org_z[iSample] = P.z;
                    occRay.tnear[iSample] = 0.0f;
                    occRay.tfar[iSample] = dist - kMilli;
                }
                else
                {
                    occRay.tfar[iSample] = -1.0f;
                }
            }

            rtc.Occluded4(&valid.x, scene, &ctx, &occRay);

            i32 hits = 0;
            for (i32 iSample = 0; iSample < kSamples; ++iSample)
            {
                if (occRay.tfar[iSample] > 0.0f)
                {
                    ++hits;
                }
            }
            if (hits)
            {
                float4 direct = EvalSphereLight(f4_neg(rd), P, N, albedo, rome.x, rome.z, light.pos, light.rad);
                direct = f4_mulvs(direct, hits * kWeight);
                lighting = f4_add(lighting, direct);
            }
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
                    const float3* pim_noalias lmBuf = r_lm_denoised ? lmap.denoised : lmap.color;
                    float2 lmUv = f2_blend(
                        lmUvs.uvs[a],
                        lmUvs.uvs[b],
                        lmUvs.uvs[c],
                        hit.wuvt);
                    float4 diffuseGI = f3_f4(UvBilinearClamp_f3(
                        lmBuf,
                        i2_s(lmap.size),
                        lmUv), 0.0f);
                    float4 R = f4_reflect3(rd, N);
                    R = SpecularDominantDir(N, R, BrdfAlpha(rome.x));
                    float4 specularGI = f4_mulvs(diffuseGI, f4_dotsat(N, R));
                    float4 indirect = IndirectBRDF(
                        f4_neg(rd),
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

            if (gigrid->probes)
            {
                giprobe_t probe = gigrid_sample(gigrid, P, N);
                const float4* pim_noalias axii = gigrid->axii;
                float4 R = f4_reflect3(rd, N);
                R = SpecularDominantDir(N, R, BrdfAlpha(rome.x));
                float4 diffuseGI = f4_0;
                float4 specularGI = f4_0;
                for (i32 iDir = 0; iDir < kGiDirections; ++iDir)
                {
                    float4 axis = axii[iDir];
                    float4 amplitude = probe.Values[iDir];
                    float4 diff = SG_Irradiance(axis, amplitude, N);
                    float4 spec = SG_Irradiance(axis, amplitude, R);
                    diffuseGI = f4_add(diffuseGI, diff);
                    specularGI = f4_add(specularGI, spec);
                }
                //lighting = diffuseGI;
                float4 indirect = IndirectBRDF(
                    f4_neg(rd),
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
    ProfileBegin(pm_drawscene);
    task_DrawScene* task = tmp_calloc(sizeof(*task));
    task->target = target;
    task->camera = camera;
    task->world = world;
    task_run(&task->task, DrawSceneFn, target->width * target->height);
    ProfileEnd(pm_drawscene);
}
