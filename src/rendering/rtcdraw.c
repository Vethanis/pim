#include "rendering/rtcdraw.h"
#include "rendering/librtc.h"
#include "math/float4_funcs.h"
#include "math/float2_funcs.h"
#include "math/float4x4_funcs.h"
#include "math/lighting.h"
#include "math/frustum.h"
#include "math/sampling.h"

#include "rendering/framebuffer.h"
#include "rendering/sampler.h"
#include "rendering/camera.h"
#include "rendering/drawable.h"
#include "rendering/lights.h"

#include "common/cvar.h"
#include "common/profiler.h"
#include "common/console.h"
#include "common/fnv1a.h"

#include "threading/task.h"
#include "allocator/allocator.h"

#include <string.h>

typedef struct drawhash_s
{
    u64 meshes;
    u64 materials;
    u64 translations;
    u64 rotations;
    u64 scales;
} drawhash_t;

static RTCDevice ms_device;
static RTCScene ms_scene;
static drawhash_t ms_hash;

static drawhash_t HashDrawables(void);
static RTCScene CreateScene(void);
static void UpdateScene(void);
static bool AddDrawable(
    RTCScene scene,
    u32 geomId,
    meshid_t meshid,
    material_t mat,
    float4 translation,
    quat rotation,
    float4 scale);
static void DrawScene(framebuf_t* target, const camera_t* camera, RTCScene scene);

void RtcDrawInit(void)
{
    if (rtc_init())
    {
        ms_device = rtc.NewDevice(NULL);
        ms_hash = HashDrawables();
        ms_scene = CreateScene();
    }
}

void RtcDrawShutdown(void)
{
    if (ms_device)
    {
        rtc.ReleaseScene(ms_scene);
        ms_scene = NULL;
        rtc.ReleaseDevice(ms_device);
        ms_device = NULL;
    }
}

void RtcDraw(framebuf_t* target, const camera_t* camera)
{
    if (!ms_device)
    {
        return;
    }
    UpdateScene();
    DrawScene(target, camera, ms_scene);
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

static RTCScene CreateScene(void)
{
    RTCDevice device = ms_device;
    ASSERT(device);

    RTCScene scene = rtc.NewScene(device);
    ASSERT(scene);
    if (!scene)
    {
        return NULL;
    }

    const drawables_t* drawables = drawables_get();
    const i32 count = drawables->count;
    const meshid_t* meshes = drawables->meshes;
    const material_t* materials = drawables->materials;
    const float4* translations = drawables->translations;
    const quat* rotations = drawables->rotations;
    const float4* scales = drawables->scales;

    for (i32 i = 0; i < count; ++i)
    {
        AddDrawable(
            scene,
            i,
            meshes[i],
            materials[i],
            translations[i],
            rotations[i],
            scales[i]);
    }

    rtc.CommitScene(scene);
    return scene;
}

static void UpdateScene(void)
{
    drawhash_t newHash = HashDrawables();
    if (memcmp(&ms_hash, &newHash, sizeof(ms_hash)))
    {
        ms_hash = newHash;
        rtc.ReleaseScene(ms_scene);
        ms_scene = CreateScene();
    }
}

static bool AddDrawable(
    RTCScene scene,
    u32 geomId,
    meshid_t meshid,
    material_t mat,
    float4 translation,
    quat rotation,
    float4 scale)
{
    RTCDevice device = ms_device;
    ASSERT(device);
    ASSERT(scene);

    mesh_t mesh = { 0 };
    if (!mesh_get(meshid, &mesh))
    {
        goto onfail;
    }

    RTCGeometry geom = rtc.NewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);
    ASSERT(geom);
    if (!geom)
    {
        goto onfail;
    }

    const i32 vertCount = mesh.length;
    const i32 triCount = vertCount / 3;

    float3* pim_noalias dstPositions = rtc.SetNewGeometryBuffer(
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
    const float4* pim_noalias srcPositions = mesh.positions;
    for (i32 i = 0; i < vertCount; ++i)
    {
        float4 position = f4x4_mul_pt(M, srcPositions[i]);
        dstPositions[i] = f4_f3(position);
    }

    // kind of wasteful
    i32* pim_noalias dstIndices = rtc.SetNewGeometryBuffer(
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
    rtc.AttachGeometryByID(scene, geom, geomId);
    rtc.ReleaseGeometry(geom);
    return true;

onfail:
    if (geom)
    {
        rtc.ReleaseGeometry(geom);
    }
    return false;
}

typedef enum
{
    hit_nothing = 0,
    hit_triangle,

    hit_COUNT
} hittype_t;

typedef struct rayhit_s
{
    float4 wuvt;
    hittype_t type;
    i32 iDrawable;
    i32 iVert;
} rayhit_t;

static RTCRay VEC_CALL RtcNewRay(float4 ro, float4 rd, float tNear, float tFar)
{
    ASSERT(tFar > tNear);
    ASSERT(tNear > 0.0f);
    RTCRay rtcRay = { 0 };
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

static RTCRayHit VEC_CALL RtcIntersect(
    RTCScene scene,
    float4 ro,
    float4 rd,
    float tNear,
    float tFar)
{
    RTCIntersectContext ctx;
    rtcInitIntersectContext(&ctx);
    RTCRayHit rayHit = { 0 };
    rayHit.ray = RtcNewRay(ro, rd, tNear, tFar);
    rayHit.hit.primID = RTC_INVALID_GEOMETRY_ID; // triangle index
    rayHit.hit.geomID = RTC_INVALID_GEOMETRY_ID; // object id
    rayHit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID; // instance id
    rtc.Intersect1(scene, &ctx, &rayHit);
    return rayHit;
}

static rayhit_t VEC_CALL TraceRay(
    RTCScene scene,
    float4 ro,
    float4 rd,
    float tNear,
    float tFar)
{
    rayhit_t hit = { 0 };
    hit.wuvt.w = tFar;

    RTCRayHit rtcHit = RtcIntersect(scene, ro, rd, tNear, tFar);
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
    i32 iVert = rtcHit.hit.primID * 3;
    ASSERT(iVert >= 0);
    float u = rtcHit.hit.u;
    float v = rtcHit.hit.v;
    float w = 1.0f - (u + v);
    float t = rtcHit.ray.tfar;

    hit.type = hit_triangle;
    hit.wuvt = f4_v(w, u, v, t);
    hit.iDrawable = rtcHit.hit.geomID;
    hit.iVert = iVert;

    return hit;
}
typedef struct task_DrawScene
{
    task_t task;
    framebuf_t* target;
    const camera_t* camera;
    RTCScene scene;
} task_DrawScene;

static void DrawSceneFn(task_t* pbase, i32 begin, i32 end)
{
    task_DrawScene* task = (task_DrawScene*)pbase;
    framebuf_t* target = task->target;
    const camera_t* camera = task->camera;
    RTCScene scene = task->scene;

    const drawables_t* drawables = drawables_get();
    const i32 drawableCount = drawables->count;
    const meshid_t* pim_noalias meshids = drawables->meshes;
    const material_t* pim_noalias materials = drawables->materials;
    const float4x4* pim_noalias matrices = drawables->matrices;

    const int2 size = { target->width, target->height };
    const float2 rcpSize = { 1.0f / size.x, 1.0f / size.y };
    const float aspect = (float)size.x / size.y;
    const float fov = f1_radians(camera->fovy);
    const float tNear = camera->nearFar.x;
    const float tFar = camera->nearFar.y;

    float4 ro = camera->position;
    quat rotation = camera->rotation;
    float4 right = quat_right(rotation);
    float4 up = quat_up(rotation);
    float4 fwd = quat_fwd(rotation);
    float2 slope = proj_slope(fov, aspect);

    float4* pim_noalias dstLight = target->light;
    float* pim_noalias dstDepth = target->depth;

    float4 spherePts[16];
    for (i32 i = 0; i < NELEM(spherePts); ++i)
    {
        spherePts[i] = SampleUnitSphere(Hammersley2D(i, NELEM(spherePts)));
    }

    for (i32 i = begin; i < end; ++i)
    {
        i32 x = i % size.x;
        i32 y = i / size.x;
        float2 coord = { (x + 0.5f) * rcpSize.x, (y + 0.5f) * rcpSize.y };
        coord = f2_snorm(coord);
        float4 rd = proj_dir(right, up, fwd, slope, coord);

        rayhit_t hit = TraceRay(scene, ro, rd, tNear, tFar);
        if (hit.type == hit_nothing)
        {
            continue;
        }
        ASSERT(hit.iDrawable >= 0);
        ASSERT(hit.iDrawable < drawableCount);
        if (hit.wuvt.w < dstDepth[i])
        {
            dstDepth[i] = hit.wuvt.w;
        }
        else
        {
            continue;
        }

        mesh_t mesh = {0};
        if (!mesh_get(meshids[hit.iDrawable], &mesh))
        {
            continue;
        }

        ASSERT(hit.iVert >= 0);
        ASSERT(hit.iVert < mesh.length);

        float3x3 IM = f3x3_IM(matrices[hit.iDrawable]);
        material_t material = materials[hit.iDrawable];

        const i32 a = hit.iVert + 0;
        const i32 b = hit.iVert + 1;
        const i32 c = hit.iVert + 2;

        float4 P = f4_add(ro, f4_mulvs(rd, hit.wuvt.w));
        float4 Nws = f4_blend(
            f3x3_mul_col(IM, mesh.normals[a]),
            f3x3_mul_col(IM, mesh.normals[b]),
            f3x3_mul_col(IM, mesh.normals[c]),
            hit.wuvt);
        Nws = f4_normalize3(Nws);
        float2 uv = f2_blend(mesh.uvs[a], mesh.uvs[b], mesh.uvs[c], hit.wuvt);

        float4 albedo = material_albedo(&material, uv);
        float4 rome = material_rome(&material, uv);
        float4 Nts = material_normal(&material, uv);

        float4 N = TanToWorld(Nws, Nts);

        float4 lighting = f4_mulvs(albedo, 0.005f);
        const i32 lightCount = lights_pt_count();
        for (i32 iLight = 0; iLight < lightCount; ++iLight)
        {
            pt_light_t light = lights_get_pt(iLight);

            const i32 kSamples = 16;
            const float kWeight = 1.0f / kSamples;
            RTCRay16 ray16 = { 0 };
            i32 valid[16];
            for (i32 iSample = 0; iSample < kSamples; ++iSample)
            {
                float4 L = f4_add(light.pos, f4_mulvs(spherePts[iSample], light.pos.w));
                L = f4_sub(L, P);
                float dist = f4_distance3(P, L);
                dist = f1_max(dist, kMinLightDist);
                L = f4_divvs(L, dist);

                ray16.dir_x[iSample] = L.x;
                ray16.dir_y[iSample] = L.y;
                ray16.dir_z[iSample] = L.z;
                ray16.mask[iSample] = -1;
                ray16.org_x[iSample] = P.x;
                ray16.org_y[iSample] = P.y;
                ray16.org_z[iSample] = P.z;
                ray16.tnear[iSample] = kMinLightDist;
                ray16.tfar[iSample] = dist;
                valid[iSample] = -1;
            }

            float hits = 0.0f;
            RTCIntersectContext ctx;
            rtcInitIntersectContext(&ctx);
            rtc.Occluded16(valid, scene, &ctx, &ray16);
            for (i32 iSample = 0; iSample < kSamples; ++iSample)
            {
                if (ray16.tfar[iSample] > 0.0f)
                {
                    hits += kWeight;
                }
            }

            if (hits > 0.0f)
            {
                float4 sphLighting = EvalSphereLight(
                    f4_neg(rd), P, N,
                    albedo, rome.x, rome.z,
                    light.pos, light.rad, light.pos.w);
                sphLighting = f4_mulvs(sphLighting, hits);
                lighting = f4_add(lighting, sphLighting);
            }
        }

        dstLight[i] = lighting;
    }
}

static void DrawScene(framebuf_t* target, const camera_t* camera, RTCScene scene)
{
    task_DrawScene* task = tmp_calloc(sizeof(*task));
    task->target = target;
    task->camera = camera;
    task->scene = scene;
    task_run(&task->task, DrawSceneFn, target->width * target->height);
}
