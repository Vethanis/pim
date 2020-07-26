#include "rendering/path_tracer.h"
#include "rendering/material.h"
#include "rendering/mesh.h"
#include "rendering/camera.h"
#include "rendering/sampler.h"
#include "rendering/lights.h"
#include "rendering/drawable.h"
#include "rendering/librtc.h"
#include "rendering/cubemap.h"

#include "math/float2_funcs.h"
#include "math/float4_funcs.h"
#include "math/int2_funcs.h"
#include "math/quat_funcs.h"
#include "math/float4x4_funcs.h"
#include "math/sampling.h"
#include "math/sdf.h"
#include "math/area.h"
#include "math/frustum.h"
#include "math/color.h"
#include "math/lighting.h"
#include "math/atmosphere.h"
#include "math/dist1d.h"
#include "math/grid.h"
#include "math/box.h"

#include "allocator/allocator.h"
#include "threading/task.h"
#include "common/random.h"
#include "common/profiler.h"
#include "common/console.h"
#include "common/cvar.h"
#include "ui/cimgui.h"
#include "io/fd.h"
#include "common/stringutil.h"

#include <stb/stb_perlin.h>
#include <string.h>

// ----------------------------------------------------------------------------

#define kStepsPerMeter      2.0f
#define kMinSteps           3

// ----------------------------------------------------------------------------

typedef struct surfhit_s
{
    float4 P;
    float4 N;
    float4 albedo;
    float roughness;
    float occlusion;
    float metallic;
    float4 emission;
} surfhit_t;

typedef struct scatter_s
{
    float4 pos;
    float4 dir;
    float4 attenuation;
    float4 irradiance;
    float pdf;
} scatter_t;

typedef struct lightsample_s
{
    float4 direction;
    float4 irradiance;
    float4 wuvt;
    float pdf;
} lightsample_t;

typedef struct media_desc_s
{
    float4 constantAlbedo;
    float4 noiseAlbedo;
    float absorption;
    float constantAmt;
    float noiseAmt;
    i32 noiseOctaves;
    float noiseGain;
    float noiseLacunarity;
    float noiseFreq;
    float noiseScale;
    float noiseHeight;
    float noiseRange;
} media_desc_t;

typedef struct media_s
{
    float4 albedo;
    float scattering;
    float extinction;
} media_t;

typedef struct pt_scene_s
{
    RTCScene rtcScene;

    // all geometry within the scene
    // xyz: vertex position
    //   w: 1
    // [vertCount]
    float4* pim_noalias positions;
    // xyz: vertex normal
    // [vertCount]
    float4* pim_noalias normals;
    //  xy: texture coordinate
    // [vertCount]
    float2* pim_noalias uvs;
    // material indices
    // [vertCount]
    i32* pim_noalias matIds;

    // emissive triangle indices
    // [emissiveCount]
    i32* pim_noalias emissives;
    // [emissiveCount]
    float* pim_noalias emPdfs;

    // grid of discrete light distributions
    grid_t lightGrid;
    // [lightGrid.size]
    dist1d_t* lightDists;

    // surface description, indexed by matIds
    // [matCount]
    material_t* pim_noalias materials;

    Cubemap* sky;

    // array lengths
    i32 vertCount;
    i32 matCount;
    i32 emissiveCount;
    // hyperparameters
    float3 sunDirection;
    float sunIntensity;
    media_desc_t mediaDesc;
} pt_scene_t;

// ----------------------------------------------------------------------------

static void OnRtcError(void* user, RTCError error, const char* msg);
static bool InitRTC(void);
pim_inline RTCRay VEC_CALL RtcNewRay(ray_t ray, float tNear, float tFar);
pim_inline RTCRayHit VEC_CALL RtcIntersect(
    RTCScene scene,
    ray_t ray,
    float tNear,
    float tFar);
static RTCScene RtcNewScene(const pt_scene_t* scene);
static void FlattenDrawables(pt_scene_t* scene);
static float EmissionPdf(
    const pt_scene_t* scene,
    prng_t* rng,
    i32 iVert,
    i32 attempts);
static void CalcEmissionPdfFn(task_t* pbase, i32 begin, i32 end);
static void SetupEmissives(pt_scene_t* scene);
static void SetupLightGridFn(task_t* pbase, i32 begin, i32 end);
static void SetupLightGrid(pt_scene_t* scene);
static void UpdateScene(pt_scene_t* scene);
pim_inline float4 VEC_CALL GetVert4(
    const float4* vertices,
    i32 iVert,
    float4 wuv);
pim_inline float2 VEC_CALL GetVert2(
    const float2* vertices,
    i32 iVert,
    float4 wuv);
pim_inline float VEC_CALL GetArea(const pt_scene_t* scene, i32 iLight);
pim_inline const material_t* VEC_CALL GetMaterial(
    const pt_scene_t* scene,
    rayhit_t hit);
pim_inline float4 VEC_CALL GetSky(
    const pt_scene_t* scene,
    float4 ro,
    float4 rd);
pim_inline surfhit_t VEC_CALL GetSurface(
    const pt_scene_t* scene,
    ray_t rin,
    rayhit_t hit,
    i32 bounce);
pim_inline rayhit_t VEC_CALL pt_intersect_local(
    const pt_scene_t* scene,
    ray_t ray,
    float tNear,
    float tFar);
pim_inline float4 VEC_CALL SampleSpecular(
    prng_t* rng,
    float4 I,
    float4 N,
    float alpha);
pim_inline float4 VEC_CALL SampleDiffuse(prng_t* rng, float4 N);
pim_inline float4 VEC_CALL BrdfSample(
    prng_t* rng,
    const surfhit_t* surf,
    float4 I);
pim_inline float4 VEC_CALL BrdfEval(
    float4 I,
    const surfhit_t* surf,
    float4 L);
pim_inline scatter_t VEC_CALL BrdfScatter(
    prng_t* rng,
    const surfhit_t* surf,
    float4 I);
pim_inline bool VEC_CALL LightSelect(
    const pt_scene_t* scene,
    prng_t* rng,
    float4 position,
    i32* iVertOut,
    float* pdfOut);
pim_inline float4 VEC_CALL LightRad(
    const pt_scene_t* scene,
    i32 iLight,
    float4 wuv,
    float4 ro,
    float4 rd);
pim_inline lightsample_t VEC_CALL LightSample(
    const pt_scene_t* scene,
    prng_t* rng,
    float4 ro,
    i32 iLight);
pim_inline float VEC_CALL LightEvalPdf(
    const pt_scene_t* scene,
    i32 iLight,
    float4 ro,
    float4 rd,
    float4* wuvOut);
pim_inline float4 VEC_CALL EstimateDirect(
    const pt_scene_t* scene,
    prng_t* rng,
    const surfhit_t* surf,
    const rayhit_t* hit,
    i32 iLight,
    float4 I);
pim_inline float4 VEC_CALL SampleLights(
    const pt_scene_t* scene,
    prng_t* rng,
    const surfhit_t* surf,
    const rayhit_t* hit,
    float4 I);
pim_inline bool VEC_CALL EvaluateLight(
    const pt_scene_t* scene,
    prng_t* rng,
    float4 P,
    float4* lightOut,
    float4* dirOut);
static void media_desc_new(media_desc_t* desc);
static void media_desc_update(media_desc_t* desc);
static void media_desc_gui(media_desc_t* desc);
static void media_desc_load(media_desc_t* desc, const char* name);
static void media_desc_save(const media_desc_t* desc, const char* name);
pim_inline float2 VEC_CALL Media_Density(const media_desc_t* desc, float4 P);
pim_inline media_t VEC_CALL Media_Sample(const media_desc_t* desc, float4 P);
pim_inline float4 VEC_CALL Media_Albedo(const media_desc_t* desc, float4 P);
pim_inline float4 VEC_CALL Media_Normal(const media_desc_t* desc, float4 P);
pim_inline media_t VEC_CALL Media_Lerp(media_t lhs, media_t rhs, float t);
pim_inline i32 VEC_CALL CalcStepCount(float rayLen);
pim_inline float4 VEC_CALL CalcTransmittance(
    const pt_scene_t* scene,
    prng_t* rng,
    float4 ro,
    float4 rd,
    float rayLen);
pim_inline scatter_t VEC_CALL ScatterRay(
    const pt_scene_t* scene,
    prng_t* rng,
    float4 ro,
    float4 rd,
    float rayLen);
static void TraceFn(task_t* pbase, i32 begin, i32 end);
static void RayGenFn(task_t* pBase, i32 begin, i32 end);

// ----------------------------------------------------------------------------

static cvar_t* cv_pt_lgrid_mpc;
static cvar_t* cv_r_sun_az;
static cvar_t* cv_r_sun_ze;
static cvar_t* cv_r_sun_rad;

static RTCDevice ms_device;

// ----------------------------------------------------------------------------

static void OnRtcError(void* user, RTCError error, const char* msg)
{
    if (error != RTC_ERROR_NONE)
    {
        con_logf(LogSev_Error, "rtc", "%s", msg);
        ASSERT(false);
    }
}

static bool InitRTC(void)
{
    if (!rtc_init())
    {
        return false;
    }
    ms_device = rtc.NewDevice(NULL);
    if (!ms_device)
    {
        OnRtcError(NULL, rtc.GetDeviceError(NULL), "Failed to create device");
        return false;
    }
    rtc.SetDeviceErrorFunction(ms_device, OnRtcError, NULL);
    return true;
}

void pt_sys_init(void)
{
    cv_pt_lgrid_mpc = cvar_find("pt_lgrid_mpc");
    cv_r_sun_az = cvar_find("r_sun_az");
    cv_r_sun_ze = cvar_find("r_sun_ze");
    cv_r_sun_rad = cvar_find("r_sun_rad");

    InitRTC();
}

void pt_sys_update(void)
{

}

void pt_sys_shutdown(void)
{
    rtc.ReleaseDevice(ms_device);
    ms_device = NULL;
}

pim_inline RTCRay VEC_CALL RtcNewRay(ray_t ray, float tNear, float tFar)
{
    ASSERT(tFar > tNear);
    ASSERT(tNear >= 0.0f);
    RTCRay rtcRay = { 0 };
    rtcRay.org_x = ray.ro.x;
    rtcRay.org_y = ray.ro.y;
    rtcRay.org_z = ray.ro.z;
    rtcRay.tnear = tNear;
    rtcRay.dir_x = ray.rd.x;
    rtcRay.dir_y = ray.rd.y;
    rtcRay.dir_z = ray.rd.z;
    rtcRay.tfar = tFar;
    rtcRay.mask = -1;
    rtcRay.flags = 0;
    return rtcRay;
}

pim_inline RTCRayHit VEC_CALL RtcIntersect(
    RTCScene scene,
    ray_t ray,
    float tNear,
    float tFar)
{
    RTCIntersectContext ctx;
    rtcInitIntersectContext(&ctx);
    RTCRayHit rayHit = { 0 };
    rayHit.ray = RtcNewRay(ray, tNear, tFar);
    rayHit.hit.primID = RTC_INVALID_GEOMETRY_ID; // triangle index
    rayHit.hit.geomID = RTC_INVALID_GEOMETRY_ID; // object id
    rayHit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID; // instance id
    rtc.Intersect1(scene, &ctx, &rayHit);
    return rayHit;
}

static RTCScene RtcNewScene(const pt_scene_t* scene)
{
    RTCScene rtcScene = rtc.NewScene(ms_device);
    ASSERT(rtcScene);
    if (!rtcScene)
    {
        return NULL;
    }

    RTCGeometry geom = rtc.NewGeometry(ms_device, RTC_GEOMETRY_TYPE_TRIANGLE);
    ASSERT(geom);

    const i32 vertCount = scene->vertCount;
    const i32 triCount = vertCount / 3;

    float3* pim_noalias dstPositions = rtc.SetNewGeometryBuffer(
        geom,
        RTC_BUFFER_TYPE_VERTEX,
        0,
        RTC_FORMAT_FLOAT3,
        sizeof(float3),
        vertCount);
    if (!dstPositions)
    {
        ASSERT(false);
        rtc.ReleaseGeometry(geom);
        rtc.ReleaseScene(rtcScene);
        return NULL;
    }
    const float4* pim_noalias srcPositions = scene->positions;
    for (i32 i = 0; i < vertCount; ++i)
    {
        dstPositions[i] = f4_f3(srcPositions[i]);
    }

    // kind of wasteful
    i32* pim_noalias dstIndices = rtc.SetNewGeometryBuffer(
        geom,
        RTC_BUFFER_TYPE_INDEX,
        0,
        RTC_FORMAT_UINT3,
        sizeof(int3),
        triCount);
    if (!dstIndices)
    {
        ASSERT(false);
        rtc.ReleaseGeometry(geom);
        rtc.ReleaseScene(rtcScene);
        return NULL;
    }
    for (i32 i = 0; i < vertCount; ++i)
    {
        dstIndices[i] = i;
    }

    rtc.CommitGeometry(geom);
    rtc.AttachGeometry(rtcScene, geom);
    rtc.ReleaseGeometry(geom);
    geom = NULL;

    rtc.CommitScene(rtcScene);

    return rtcScene;
}

static void FlattenDrawables(pt_scene_t* scene)
{
    const drawables_t* drawTable = drawables_get();
    const i32 drawCount = drawTable->count;
    const meshid_t* meshes = drawTable->meshes;
    const float4x4* matrices = drawTable->matrices;
    const material_t* materials = drawTable->materials;

    i32 vertCount = 0;
    float4* positions = NULL;
    float4* normals = NULL;
    float2* uvs = NULL;
    i32* matIds = NULL;

    i32 matCount = 0;
    material_t* sceneMats = NULL;

    for (i32 i = 0; i < drawCount; ++i)
    {
        mesh_t mesh;
        if (mesh_get(meshes[i], &mesh))
        {
            const i32 vertBack = vertCount;
            const i32 matBack = matCount;
            vertCount += mesh.length;
            matCount += 1;

            const float4x4 M = matrices[i];
            const float3x3 IM = f3x3_IM(M);
            const material_t material = materials[i];

            PermReserve(positions, vertCount);
            PermReserve(normals, vertCount);
            PermReserve(uvs, vertCount);
            PermReserve(matIds, vertCount);
            PermReserve(sceneMats, matCount);

            sceneMats[matBack] = material;

            for (i32 j = 0; j < mesh.length; ++j)
            {
                positions[vertBack + j] = f4x4_mul_pt(M, mesh.positions[j]);
            }

            for (i32 j = 0; j < mesh.length; ++j)
            {
                normals[vertBack + j] = f4_normalize3(f3x3_mul_col(IM, mesh.normals[j]));
            }

            for (i32 j = 0; (j + 3) <= mesh.length; j += 3)
            {
                float2 UA = TransformUv(mesh.uvs[j + 0], material.st);
                float2 UB = TransformUv(mesh.uvs[j + 1], material.st);
                float2 UC = TransformUv(mesh.uvs[j + 2], material.st);
                uvs[vertBack + j + 0] = UA;
                uvs[vertBack + j + 1] = UB;
                uvs[vertBack + j + 2] = UC;
            }

            for (i32 j = 0; j < mesh.length; ++j)
            {
                matIds[vertBack + j] = matBack;
            }
        }
    }

    scene->vertCount = vertCount;
    scene->positions = positions;
    scene->normals = normals;
    scene->uvs = uvs;
    scene->matIds = matIds;

    scene->matCount = matCount;
    scene->materials = sceneMats;
}

static float EmissionPdf(
    const pt_scene_t* scene,
    prng_t* rng,
    i32 iVert,
    i32 attempts)
{
    const i32 iMat = scene->matIds[iVert];
    const material_t* mat = scene->materials + iMat;

    if (mat->flags & matflag_sky)
    {
        return 1.0f;
    }

    const float kThreshold = 0.01f;
    float4 rome = ColorToLinear(mat->flatRome);
    if (rome.w > kThreshold)
    {
        texture_t romeMap = { 0 };
        if (texture_get(mat->rome, &romeMap))
        {
            const float2* pim_noalias uvs = scene->uvs;
            const float2 UA = uvs[iVert + 0];
            const float2 UB = uvs[iVert + 1];
            const float2 UC = uvs[iVert + 2];

            i32 hits = 0;
            for (i32 i = 0; i < attempts; ++i)
            {
                float4 wuv = SampleBaryCoord(f2_rand(rng));
                float2 uv = f2_blend(UA, UB, UC, wuv);
                float4 sample = UvWrap_c32(romeMap.texels, romeMap.size, uv);
                float em = sample.w * rome.w;
                if (em > kThreshold)
                {
                    ++hits;
                }
            }
            return (float)hits / (float)attempts;
        }
        else
        {
            return 1.0f;
        }
    }
    else
    {
        return 0.0f;
    }
}

typedef struct task_CalcEmissionPdf
{
    task_t task;
    const pt_scene_t* scene;
    float* pdfs;
    i32 attempts;
} task_CalcEmissionPdf;

static void CalcEmissionPdfFn(task_t* pbase, i32 begin, i32 end)
{
    task_CalcEmissionPdf* task = (task_CalcEmissionPdf*)pbase;
    const pt_scene_t* scene = task->scene;
    const i32 attempts = task->attempts;
    float* pim_noalias pdfs = task->pdfs;

    prng_t rng = prng_get();
    for (i32 i = begin; i < end; ++i)
    {
        pdfs[i] = EmissionPdf(scene, &rng, i * 3, attempts);
    }
    prng_set(rng);
}

static void SetupEmissives(pt_scene_t* scene)
{
    const i32 vertCount = scene->vertCount;
    const i32 triCount = vertCount / 3;

    task_CalcEmissionPdf* task = tmp_calloc(sizeof(*task));
    task->scene = scene;
    task->pdfs = tmp_malloc(sizeof(task->pdfs[0]) * triCount);
    task->attempts = 100000;

    task_run(&task->task, CalcEmissionPdfFn, triCount);

    i32 emissiveCount = 0;
    i32* emissives = NULL;
    float* pim_noalias emPdfs = NULL;

    const float* pim_noalias taskPdfs = task->pdfs;
    for (i32 iTri = 0; iTri < triCount; ++iTri)
    {
        float pdf = taskPdfs[iTri];
        if (pdf > 0.01f)
        {
            ++emissiveCount;
            PermReserve(emissives, emissiveCount);
            PermReserve(emPdfs, emissiveCount);
            emissives[emissiveCount - 1] = iTri * 3;
            emPdfs[emissiveCount - 1] = pdf;
        }
    }

    scene->emissiveCount = emissiveCount;
    scene->emissives = emissives;
    scene->emPdfs = emPdfs;
}

typedef struct task_SetupLightGrid
{
    task_t task;
    pt_scene_t* scene;
} task_SetupLightGrid;

static void SetupLightGridFn(task_t* pbase, i32 begin, i32 end)
{
    task_SetupLightGrid* task = (task_SetupLightGrid*)pbase;

    pt_scene_t* scene = task->scene;

    const grid_t grid = scene->lightGrid;
    dist1d_t* dists = scene->lightDists;

    const float4* pim_noalias positions = scene->positions;
    const i32* pim_noalias matIds = scene->matIds;
    const material_t* pim_noalias materials = scene->materials;

    const i32 emissiveCount = scene->emissiveCount;
    const i32* pim_noalias emissives = scene->emissives;

    const float weight = 1.0f / emissiveCount;
    const float metersPerCell = cv_pt_lgrid_mpc->asFloat;
    const float minDist = f1_max(metersPerCell * 0.5f, kMinLightDist);
    const float minDistSq = minDist * minDist;
    const float surfIrradiance = UnpackEmission(f4_1, 1.0f).x;
    const float skyIrradiance = 1365.0f;

    for (i32 i = begin; i < end; ++i)
    {
        dist1d_t dist;
        dist1d_new(&dist, emissiveCount);

        float4 position = grid_position(&grid, i);
        for (i32 iList = 0; iList < emissiveCount; ++iList)
        {
            i32 iVert = emissives[iList];
            float4 A = positions[iVert + 0];
            float4 B = positions[iVert + 1];
            float4 C = positions[iVert + 2];
            float4 mid = f4_divvs(f4_add(f4_add(A, B), C), 3.0f);

            float distA = f4_distancesq3(A, position);
            float distB = f4_distancesq3(B, position);
            float distC = f4_distancesq3(C, position);
            float distMid = f4_distancesq3(mid, position);
            float distSq = f1_min(distA, f1_min(distB, f1_min(distC, distMid)));
            distSq = f1_max(minDistSq, distSq - metersPerCell * 0.5f);

            float irradiance = surfIrradiance;

            const material_t* material = materials + matIds[iVert];
            if (material->flags & matflag_sky)
            {
                irradiance = skyIrradiance;
            }
            float power = irradiance / distSq;

            dist.pdf[iList] = power * weight;
        }

        dist1d_bake(&dist);
        dists[i] = dist;
    }
}

static void SetupLightGrid(pt_scene_t* scene)
{
    box_t bounds = box_from_pts(scene->positions, scene->vertCount);
    grid_t grid;
    const float metersPerCell = cv_pt_lgrid_mpc->asFloat;
    grid_new(&grid, bounds, 1.0f / metersPerCell);
    const i32 len = grid_len(&grid);
    scene->lightGrid = grid;
    scene->lightDists = perm_calloc(sizeof(scene->lightDists[0]) * len);

    task_SetupLightGrid* task = tmp_calloc(sizeof(*task));
    task->scene = scene;

    task_run(&task->task, SetupLightGridFn, len);
}

static void UpdateScene(pt_scene_t* scene)
{
    float azimuth = cv_r_sun_az ? f1_sat(cv_r_sun_az->asFloat) : 0.0f;
    float zenith = cv_r_sun_ze ? f1_sat(cv_r_sun_ze->asFloat) : 0.5f;
    float irradiance = cv_r_sun_rad ? cv_r_sun_rad->asFloat : 100.0f;
    const float4 kUp = { 0.0f, 1.0f, 0.0f, 0.0f };

    float4 sunDir = TanToWorld(kUp, SampleUnitHemisphere(f2_v(azimuth, zenith)));
    scene->sunDirection = f4_f3(sunDir);
    scene->sunIntensity = irradiance;

    const u32 kSkyName = 1;
    i32 iSky = Cubemaps_Find(kSkyName);
    if (iSky != -1)
    {
        scene->sky = Cubemaps_Get()->cubemaps + iSky;
    }
    else
    {
        scene->sky = NULL;
    }
}

pt_scene_t* pt_scene_new(void)
{
    ASSERT(ms_device);
    if (!ms_device)
    {
        return NULL;
    }

    pt_scene_t* scene = perm_calloc(sizeof(*scene));
    UpdateScene(scene);

    FlattenDrawables(scene);
    SetupEmissives(scene);
    SetupLightGrid(scene);
    media_desc_new(&scene->mediaDesc);
    scene->rtcScene = RtcNewScene(scene);

    return scene;
}

void pt_scene_del(pt_scene_t* scene)
{
    if (scene)
    {
        if (scene->rtcScene)
        {
            RTCScene rtcScene = scene->rtcScene;
            rtc.ReleaseScene(rtcScene);
            scene->rtcScene = NULL;
        }

        pim_free(scene->positions);
        pim_free(scene->normals);
        pim_free(scene->uvs);
        pim_free(scene->matIds);

        pim_free(scene->materials);

        pim_free(scene->emissives);
        pim_free(scene->emPdfs);

        {
            const i32 gridLen = grid_len(&scene->lightGrid);
            dist1d_t* lightDists = scene->lightDists;
            for (i32 i = 0; i < gridLen; ++i)
            {
                dist1d_del(lightDists + i);
            }
            pim_free(scene->lightDists);
        }

        memset(scene, 0, sizeof(*scene));
        pim_free(scene);
    }
}

/*
    i32 vertCount;
    i32 matCount;
    i32 emissiveCount;
    // hyperparameters
    float3 sunDirection;
    float sunIntensity;
    media_desc_t mediaDesc;
*/
void pt_scene_gui(pt_scene_t* scene)
{
    if (scene && igCollapsingHeader1("pt scene"))
    {
        igIndent(0.0f);
        igText("Vertex Count: %d", scene->vertCount);
        igText("Material Count: %d", scene->matCount);
        igText("Emissive Count: %d", scene->emissiveCount);
        igText("Sun Direction: %f %f %f", scene->sunDirection.x, scene->sunDirection.y, scene->sunDirection.z);
        igText("Sun Intensity: %f", scene->sunIntensity);
        media_desc_gui(&scene->mediaDesc);
        igUnindent(0.0f);
    }
}

pim_inline float4 VEC_CALL GetVert4(
    const float4* vertices,
    i32 iVert,
    float4 wuv)
{
    return f4_blend(
        vertices[iVert + 0],
        vertices[iVert + 1],
        vertices[iVert + 2],
        wuv);
}

pim_inline float2 VEC_CALL GetVert2(
    const float2* vertices,
    i32 iVert,
    float4 wuv)
{
    return f2_blend(
        vertices[iVert + 0],
        vertices[iVert + 1],
        vertices[iVert + 2],
        wuv);
}

pim_inline float VEC_CALL GetArea(const pt_scene_t* scene, i32 iLight)
{
    const float4* pim_noalias positions = scene->positions;
    return TriArea3D(positions[iLight + 0], positions[iLight + 1], positions[iLight + 2]);
}

pim_inline const material_t* VEC_CALL GetMaterial(
    const pt_scene_t* scene,
    rayhit_t hit)
{
    i32 iVert = hit.index;
    ASSERT(iVert >= 0);
    ASSERT(iVert < scene->vertCount);
    i32 matIndex = scene->matIds[iVert];
    ASSERT(matIndex >= 0);
    ASSERT(matIndex < scene->matCount);
    return scene->materials + matIndex;
}

pim_inline float4 VEC_CALL GetSky(
    const pt_scene_t* scene,
    float4 ro,
    float4 rd)
{
    if (scene->sky)
    {
        return f3_f4(Cubemap_ReadColor(scene->sky, rd), 1.0f);
    }
    return f4_0;
}

pim_inline surfhit_t VEC_CALL GetSurface(
    const pt_scene_t* scene,
    ray_t rin,
    rayhit_t hit,
    i32 bounce)
{
    surfhit_t surf = { 0 };

    const material_t* mat = GetMaterial(scene, hit);

    float2 uv = GetVert2(scene->uvs, hit.index, hit.wuvt);
    surf.N = f4_normalize3(GetVert4(scene->normals, hit.index, hit.wuvt));
    surf.P = f4_add(rin.ro, f4_mulvs(rin.rd, hit.wuvt.w));
    surf.P = f4_add(surf.P, f4_mulvs(surf.N, kMilli));

    texture_t tex;
    if (bounce == 0)
    {
        if (texture_get(mat->normal, &tex))
        {
            float4 Nts = UvBilinearWrap_dir8(tex.texels, tex.size, uv);
            surf.N = TanToWorld(surf.N, Nts);
        }
    }

    surf.albedo = ColorToLinear(mat->flatAlbedo);
    if (texture_get(mat->albedo, &tex))
    {
        float4 sample;
        if (bounce == 0)
        {
            sample = UvBilinearWrap_c32(tex.texels, tex.size, uv);
        }
        else
        {
            sample = UvWrap_c32(tex.texels, tex.size, uv);
        }
        surf.albedo = f4_mul(surf.albedo, sample);
    }

    float4 rome = ColorToLinear(mat->flatRome);
    if (texture_get(mat->rome, &tex))
    {
        float4 sample;
        if (bounce == 0)
        {
            sample = UvBilinearWrap_c32(tex.texels, tex.size, uv);
        }
        else
        {
            sample = UvWrap_c32(tex.texels, tex.size, uv);
        }
        rome = f4_mul(rome, sample);
    }
    surf.roughness = rome.x;
    surf.occlusion = rome.y;
    surf.metallic = rome.z;
    surf.emission = UnpackEmission(surf.albedo, rome.w);

    return surf;
}

pim_inline rayhit_t VEC_CALL pt_intersect_local(
    const pt_scene_t* scene,
    ray_t ray,
    float tNear,
    float tFar)
{
    rayhit_t hit = { 0 };
    hit.wuvt.w = -1.0f;
    hit.index = -1;

    RTCRayHit rtcHit = RtcIntersect(scene->rtcScene, ray, tNear, tFar);
    hit.normal = f4_v(rtcHit.hit.Ng_x, rtcHit.hit.Ng_y, rtcHit.hit.Ng_z, 0.0f);
    bool hitNothing =
        (rtcHit.hit.geomID == RTC_INVALID_GEOMETRY_ID) ||
        (rtcHit.ray.tfar <= 0.0f);
    if (hitNothing)
    {
        hit.type = hit_nothing;
        return hit;
    }
    if (f4_dot3(hit.normal, ray.rd) > 0.0f)
    {
        hit.type = hit_backface;
        return hit;
    }
    ASSERT(rtcHit.hit.primID != RTC_INVALID_GEOMETRY_ID);
    i32 iVert = rtcHit.hit.primID * 3;
    ASSERT(iVert >= 0);
    ASSERT(iVert < scene->vertCount);
    float u = f1_sat(rtcHit.hit.u);
    float v = f1_sat(rtcHit.hit.v);
    float w = f1_sat(1.0f - (u + v));
    float t = rtcHit.ray.tfar;

    hit.type = hit_triangle;
    hit.index = iVert;
    hit.wuvt = f4_v(w, u, v, t);

    if (GetMaterial(scene, hit)->flags & matflag_sky)
    {
        hit.type = hit_sky;
    }

    return hit;
}

rayhit_t VEC_CALL pt_intersect(
    const pt_scene_t* scene,
    ray_t ray,
    float tNear,
    float tFar)
{
    return pt_intersect_local(scene, ray, tNear, tFar);
}

pim_inline float4 VEC_CALL SampleSpecular(
    prng_t* rng,
    float4 I,
    float4 N,
    float alpha)
{
    float2 Xi = f2_rand(rng);
    float4 H = TanToWorld(N, SampleGGXMicrofacet(Xi, alpha));
    float4 L = f4_normalize3(f4_reflect3(I, H));
    return L;
}

pim_inline float4 VEC_CALL SampleDiffuse(prng_t* rng, float4 N)
{
    float2 Xi = f2_rand(rng);
    float4 L = TanToWorld(N, SampleCosineHemisphere(Xi));
    return L;
}

// samples a light direction of the brdf
pim_inline float4 VEC_CALL BrdfSample(
    prng_t* rng,
    const surfhit_t* surf,
    float4 I)
{
    float4 N = surf->N;
    float metallic = surf->metallic;
    float alpha = BrdfAlpha(surf->roughness);

    // metals are only specular
    const float amtDiffuse = 1.0f - metallic;
    const float amtSpecular = 1.0f;
    const float specProb = 1.0f / (amtSpecular + amtDiffuse);

    float4 L;
    if (prng_f32(rng) < specProb)
    {
        L = SampleSpecular(rng, I, N, alpha);
    }
    else
    {
        L = SampleDiffuse(rng, N);
    }

    return L;
}

// returns attenuation value for the given interaction
pim_inline float4 VEC_CALL BrdfEval(
    float4 I,
    const surfhit_t* surf,
    float4 L)
{
    float4 N = surf->N;
    float NoL = f4_dot3(N, L);
    if (NoL <= 0.0f)
    {
        return f4_0;
    }

    float4 V = f4_neg(I);
    float4 albedo = surf->albedo;
    float metallic = surf->metallic;
    float roughness = surf->roughness;
    float alpha = BrdfAlpha(roughness);
    float4 H = f4_normalize3(f4_add(V, L));
    float NoH = f4_dotsat(N, H);
    float HoV = f4_dotsat(H, V);
    float NoV = f4_dotsat(N, V);

    // metals are only specular
    const float amtDiffuse = 1.0f - metallic;
    const float amtSpecular = 1.0f;
    const float rcpProb = 1.0f / (amtSpecular + amtDiffuse);

    float4 brdf = f4_0;
    {
        float4 F = F_SchlickEx(albedo, metallic, HoV);
        float D = D_GTR(NoH, alpha);
        float G = G_SmithGGX(NoL, NoV, alpha);
        float4 Fr = f4_mulvs(F, D * G);
        brdf = f4_add(brdf, f4_mulvs(Fr, amtSpecular));
    }
    {
        float4 Fd = f4_mulvs(albedo, Fd_Lambert());
        brdf = f4_add(brdf, f4_mulvs(Fd, amtDiffuse));
    }

    brdf = f4_mulvs(brdf, NoL);

    float diffusePdf = amtDiffuse * LambertPdf(NoL);
    float specularPdf = amtSpecular * GGXPdf(NoH, HoV, alpha);
    float pdf = (diffusePdf + specularPdf) * rcpProb;

    brdf.w = pdf;
    return brdf;
}

pim_inline scatter_t VEC_CALL BrdfScatter(
    prng_t* rng,
    const surfhit_t* surf,
    float4 I)
{
    scatter_t result = { 0 };

    float4 N = surf->N;
    float4 V = f4_neg(I);
    float4 albedo = surf->albedo;
    float metallic = surf->metallic;
    float roughness = surf->roughness;
    float alpha = BrdfAlpha(roughness);

    // metals are only specular
    const float amtDiffuse = 1.0f - metallic;
    const float amtSpecular = 1.0f;
    const float rcpProb = 1.0f / (amtSpecular + amtDiffuse);

    float4 L;
    if (prng_f32(rng) < rcpProb)
    {
        L = SampleSpecular(rng, I, N, alpha);
    }
    else
    {
        L = SampleDiffuse(rng, N);
    }

    float NoL = f4_dot3(N, L);
    if (NoL <= 0.0f)
    {
        return result;
    }

    float4 H = f4_normalize3(f4_add(V, L));
    float NoH = f4_dotsat(N, H);
    float HoV = f4_dotsat(H, V);
    float NoV = f4_dotsat(N, V);

    float specularPdf = amtSpecular * GGXPdf(NoH, HoV, alpha);
    float diffusePdf = amtDiffuse * LambertPdf(NoL);
    float pdf = rcpProb * (diffusePdf + specularPdf);

    if (pdf <= 0.0f)
    {
        return result;
    }

    float4 brdf = f4_0;
    {
        float4 F = F_SchlickEx(albedo, metallic, HoV);
        float D = D_GTR(NoH, alpha);
        float G = G_SmithGGX(NoL, NoV, alpha);
        float4 Fr = f4_mulvs(F, D * G);
        brdf = f4_add(brdf, f4_mulvs(Fr, amtSpecular));
    }
    {
        float4 Fd = f4_mulvs(albedo, Fd_Lambert());
        brdf = f4_add(brdf, f4_mulvs(Fd, amtDiffuse));
    }

    result.dir = L;
    result.pdf = pdf;
    result.attenuation = f4_mulvs(brdf, NoL);

    return result;
}

pim_inline bool VEC_CALL LightSelect(
    const pt_scene_t* scene,
    prng_t* rng,
    float4 position,
    i32* iVertOut,
    float* pdfOut)
{
    if (scene->emissiveCount == 0)
    {
        *iVertOut = -1;
        *pdfOut = 0.0f;
        return false;
    }

    i32 iCell = grid_index(&scene->lightGrid, position);
    const dist1d_t* dist = scene->lightDists + iCell;

    i32 iList = dist1d_sampled(dist, prng_f32(rng));
    float pdf = dist1d_pdfd(dist, iList);

    i32 iVert = scene->emissives[iList];
    //float emPdf = scene->emPdfs[iList];

    *iVertOut = iVert;
    *pdfOut = pdf;// * emPdf;
    return true;
}

pim_inline float4 VEC_CALL LightRad(
    const pt_scene_t* scene,
    i32 iLight,
    float4 wuv,
    float4 ro,
    float4 rd)
{
    i32 iMat = scene->matIds[iLight];
    const material_t* mat = scene->materials + iMat;
    if (mat->flags & matflag_sky)
    {
        return GetSky(scene, ro, rd);
    }
    float2 uv = GetVert2(scene->uvs, iLight, wuv);
    float4 albedo = ColorToLinear(mat->flatAlbedo);
    texture_t tex;
    if (texture_get(mat->albedo, &tex))
    {
        albedo = f4_mul(albedo, UvWrap_c32(tex.texels, tex.size, uv));
    }
    float4 rome = ColorToLinear(mat->flatRome);
    if (texture_get(mat->rome, &tex))
    {
        rome = f4_mul(rome, UvWrap_c32(tex.texels, tex.size, uv));
    }
    return UnpackEmission(albedo, rome.w);
}

pim_inline lightsample_t VEC_CALL LightSample(
    const pt_scene_t* scene,
    prng_t* rng,
    float4 ro,
    i32 iLight)
{
    lightsample_t sample = { 0 };

    float4 wuv = SampleBaryCoord(f2_rand(rng));

    const float4* pim_noalias positions = scene->positions;
    float4 A = positions[iLight + 0];
    float4 B = positions[iLight + 1];
    float4 C = positions[iLight + 2];
    float4 pt = f4_blend(A, B, C, wuv);
    float area = TriArea3D(A, B, C);

    float4 rd = f4_sub(pt, ro);
    float distSq = f4_dot3(rd, rd);
    float distance = sqrtf(distSq);
    wuv.w = distance;
    rd = f4_divvs(rd, distance);

    sample.direction = rd;
    sample.wuvt = wuv;

    float4 N = f4_normalize3(GetVert4(scene->normals, iLight, wuv));
    float VoNl = f4_dot3(f4_neg(rd), N);
    if (VoNl > kEpsilon)
    {
        ray_t ray = { ro, rd };
        rayhit_t hit = pt_intersect_local(scene, ray, 0.0f, 1 << 20);
        if ((hit.type != hit_nothing) && (hit.index == iLight))
        {
            sample.pdf = LightPdf(area, VoNl, distSq);
            sample.irradiance = LightRad(scene, iLight, wuv, ro, rd);
            float4 Tr = CalcTransmittance(scene, rng, ro, rd, hit.wuvt.w);
            sample.irradiance = f4_mul(sample.irradiance, Tr);
        }
    }

    return sample;
}

pim_inline float VEC_CALL LightEvalPdf(
    const pt_scene_t* scene,
    i32 iLight,
    float4 ro,
    float4 rd,
    float4* wuvOut)
{
    ray_t ray = { ro, rd };
    rayhit_t hit = pt_intersect_local(scene, ray, 0.0f, 1 << 20);
    if ((hit.type != hit_nothing) && (hit.index == iLight))
    {
        *wuvOut = hit.wuvt;
        float distance = hit.wuvt.w;
        if (distance > kEpsilon)
        {
            float4 N = f4_normalize3(GetVert4(scene->normals, iLight, hit.wuvt));
            float cosTheta = f4_dot3(f4_neg(rd), N);
            if (cosTheta > kEpsilon)
            {
                float area = GetArea(scene, iLight);
                return LightPdf(area, cosTheta, distance * distance);
            }
        }
    }
    return 0.0f;
}

pim_inline float4 VEC_CALL EstimateDirect(
    const pt_scene_t* scene,
    prng_t* rng,
    const surfhit_t* surf,
    const rayhit_t* hit,
    i32 iLight,
    float4 I)
{
    float4 result = f4_0;
    {
        lightsample_t sample = LightSample(scene, rng, surf->P, iLight);
        if (sample.pdf > kEpsilon)
        {
            float4 brdf = BrdfEval(I, surf, sample.direction);
            float brdfPdf = brdf.w;
            if (brdf.w > kEpsilon)
            {
                float4 Li = f4_divvs(f4_mul(sample.irradiance, brdf), sample.pdf);
                result = f4_add(result, Li);
            }
        }
    }
    return result;
}

pim_inline float4 VEC_CALL SampleLights(
    const pt_scene_t* scene,
    prng_t* rng,
    const surfhit_t* surf,
    const rayhit_t* hit,
    float4 I)
{
    float4 sum = f4_0;
    i32 iLight;
    float lightPdf;
    if (LightSelect(scene, rng, surf->P, &iLight, &lightPdf))
    {
        if (hit->index != iLight)
        {
            float4 direct = EstimateDirect(scene, rng, surf, hit, iLight, I);
            direct = f4_divvs(direct, lightPdf);
            sum = f4_add(sum, direct);
        }
    }
    return sum;
}

pim_inline bool VEC_CALL EvaluateLight(
    const pt_scene_t* scene,
    prng_t* rng,
    float4 P,
    float4* lightOut,
    float4* dirOut)
{
    i32 iLight;
    float lightPdf;
    if (LightSelect(scene, rng, P, &iLight, &lightPdf))
    {
        lightsample_t sample = LightSample(scene, rng, P, iLight);
        if (sample.pdf > 0.0f)
        {
            *lightOut = f4_divvs(sample.irradiance, sample.pdf * lightPdf);
            *dirOut = sample.direction;
            return true;
        }
    }
    return false;
}

static void media_desc_new(media_desc_t* desc)
{
    desc->constantAlbedo = f4_v(0.941f, 0.782f, 0.720f, -0.5f);
    desc->noiseAlbedo = f4_v(0.579f, 0.628f, 0.691f, 0.758f);
    desc->absorption = exp2f(0.0f);
    desc->constantAmt = exp2f(-7.0f);
    desc->noiseAmt = exp2f(1.5f);
    desc->noiseOctaves = 2;
    desc->noiseGain = 0.5f;
    desc->noiseLacunarity = 2.03f;
    desc->noiseFreq = exp2f(-1.0f);
    desc->noiseHeight = 6.0f;
    desc->noiseScale = exp2f(0.0f);
    media_desc_update(desc);
}

static void media_desc_update(media_desc_t* desc)
{
    const i32 noiseOctaves = desc->noiseOctaves;
    const float noiseGain = desc->noiseGain;
    const float noiseScale = desc->noiseScale;

    float range = 0.0f;
    float amplitude = noiseScale;
    for (i32 i = 0; i < noiseOctaves; ++i)
    {
        range += amplitude;
        amplitude *= noiseGain;
    }

    desc->noiseRange = range;
}

static void media_desc_load(media_desc_t* desc, const char* name)
{
    if (!desc || !name)
    {
        return;
    }
    char filename[PIM_PATH];
    SPrintf(ARGS(filename), "%s_media_desc", name);
    fd_t fd = fd_open(filename, false);
    if (!fd_isopen(fd))
    {
        con_logf(LogSev_Error, "pt", "Failed to load media desc '%s'", filename);
        return;
    }
    memset(desc, 0, sizeof(*desc));
    fd_read(fd, desc, sizeof(*desc));
    fd_close(&fd);
}

static void media_desc_save(const media_desc_t* desc, const char* name)
{
    if (!desc || !name)
    {
        return;
    }
    char filename[PIM_PATH];
    SPrintf(ARGS(filename), "%s_media_desc", name);
    fd_t fd = fd_open(filename, true);
    if (!fd_isopen(fd))
    {
        fd = fd_create(filename);
    }
    if (!fd_isopen(fd))
    {
        con_logf(LogSev_Error, "pt", "Failed to save media desc '%s'", filename);
        return;
    }
    fd_write(fd, desc, sizeof(*desc));
    fd_close(&fd);
}

static void igLog2SliderFloat(const char* label, float* pLinear, float log2Lo, float log2Hi)
{
    float asLog2 = log2f(*pLinear);
    igSliderFloat(label, &asLog2, log2Lo, log2Hi);
    *pLinear = exp2f(asLog2);
}

static char gui_mediadesc_name[PIM_PATH];

static void media_desc_gui(media_desc_t* desc)
{
    const u32 hdrPicker = ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_InputRGB;
    const u32 ldrPicker = ImGuiColorEditFlags_Float | ImGuiColorEditFlags_InputRGB;

    if (desc && igCollapsingHeader1("MediaDesc"))
    {
        igIndent(0.0f);
        igInputText("Preset Name", gui_mediadesc_name, sizeof(gui_mediadesc_name), 0, NULL, NULL);
        if (igButton("Load Preset"))
        {
            media_desc_load(desc, gui_mediadesc_name);
        }
        if (igButton("Save Preset"))
        {
            media_desc_save(desc, gui_mediadesc_name);
        }
        igColorEdit3("Constant Albedo", &desc->constantAlbedo.x, ldrPicker);
        igSliderFloat("Constant Scatter Dir", &desc->constantAlbedo.w, -1.0f, 1.0f);
        igColorEdit3("Noise Albedo", &desc->noiseAlbedo.x, ldrPicker);
        igSliderFloat("Noise Scatter Dir", &desc->noiseAlbedo.w, -1.0f, 1.0f);
        igLog2SliderFloat("Log2 Absorption", &desc->absorption, -10.0f, 10.0f);
        igLog2SliderFloat("Log2 Constant Amount", &desc->constantAmt, -10.0f, 10.0f);
        igLog2SliderFloat("Log2 Noise Amount", &desc->noiseAmt, -10.0f, 10.0f);
        igSliderInt("Noise Octaves", &desc->noiseOctaves, 1, 10, "%d");
        igSliderFloat("Noise Gain", &desc->noiseGain, 0.0f, 1.0f);
        igSliderFloat("Noise Lacunarity", &desc->noiseLacunarity, 1.0f, 3.0f);
        igLog2SliderFloat("Log2 Noise Frequency", &desc->noiseFreq, -10.0f, 10.0f);
        igSliderFloat("Noise Height", &desc->noiseHeight, -20.0f, 20.0f);
        igLog2SliderFloat("Log2 Noise Scale", &desc->noiseScale, -5.0f, 5.0f);
        igUnindent(0.0f);

        media_desc_update(desc);
    }
}

pim_inline float2 VEC_CALL Media_Density(const media_desc_t* desc, float4 P)
{
    float heightFog = 0.0f;
    if (f1_distance(P.y, desc->noiseHeight) <= desc->noiseRange)
    {
        float noiseFreq = desc->noiseFreq;
        float noise = stb_perlin_fbm_noise3(
            P.x * noiseFreq,
            P.y * noiseFreq,
            P.z * noiseFreq,
            desc->noiseLacunarity,
            desc->noiseGain,
            desc->noiseOctaves);
        float height = desc->noiseHeight + desc->noiseScale * noise;
        float heightDensity = f1_sat(1.0f - f1_distance(height, P.y));
        heightFog = desc->noiseAmt * heightDensity;
    }

    float totalFog = desc->constantAmt + heightFog;
    float t = heightFog / totalFog;
    return f2_v(totalFog, t);
}

pim_inline media_t VEC_CALL Media_Sample(const media_desc_t* desc, float4 P)
{
    float2 density = Media_Density(desc, P);
    float totalScattering = density.x;
    float totalExtinction = totalScattering * (1.0f + desc->absorption);
    float4 albedo = f4_lerpvs(desc->constantAlbedo, desc->noiseAlbedo, density.y);

    media_t result;
    result.albedo = albedo;
    result.scattering = totalScattering;
    result.extinction = totalExtinction;

    return result;
}

pim_inline float4 VEC_CALL Media_Albedo(const media_desc_t* desc, float4 P)
{
    return Media_Sample(desc, P).albedo;
}

pim_inline float4 VEC_CALL Media_Normal(const media_desc_t* desc, float4 P)
{
    const float e = kMicro;
    float dx =
        Media_Density(desc, f4_add(P, f4_v(e, 0.0f, 0.0f, 0.0f))).x -
        Media_Density(desc, f4_add(P, f4_v(-e, 0.0f, 0.0f, 0.0f))).x;
    float dy =
        Media_Density(desc, f4_add(P, f4_v(0.0f, e, 0.0f, 0.0f))).x -
        Media_Density(desc, f4_add(P, f4_v(0.0f, -e, 0.0f, 0.0f))).x;
    float dz =
        Media_Density(desc, f4_add(P, f4_v(0.0f, 0.0f, e, 0.0f))).x -
        Media_Density(desc, f4_add(P, f4_v(0.0f, 0.0f, -e, 0.0f))).x;
    return f4_normalize3(f4_v(-dx, -dy, -dz, 0.0f));
}

pim_inline media_t VEC_CALL Media_Lerp(media_t lhs, media_t rhs, float t)
{
    media_t result;
    result.albedo = f4_lerpvs(lhs.albedo, rhs.albedo, t);
    result.scattering = f1_lerp(lhs.scattering, rhs.scattering, t);
    result.extinction = f1_lerp(lhs.extinction, rhs.extinction, t);
    return result;
}

pim_inline i32 VEC_CALL CalcStepCount(float rayLen)
{
    return i1_max(kMinSteps, (i32)(rayLen * kStepsPerMeter + 0.5f));
}

pim_inline float4 VEC_CALL CalcTransmittance(
    const pt_scene_t* scene,
    prng_t* rng,
    float4 ro,
    float4 rd,
    float rayLen)
{
    float4 transmittance = f4_1;
    media_t prevMedia = Media_Sample(&scene->mediaDesc, ro);
    const i32 kSteps = CalcStepCount(rayLen);
    const float dt0 = rayLen / kSteps;
    float t = 0.0f;
    while (t < rayLen)
    {
        float dt = prng_f32(rng) * dt0;
        t = f1_min(t + dt, rayLen);

        float4 P = f4_add(ro, f4_mulvs(rd, t));

        media_t newMedia = Media_Sample(&scene->mediaDesc, P);
        media_t media = Media_Lerp(prevMedia, newMedia, 0.5f);
        prevMedia = newMedia;

        float4 extinction = f4_mulvs(f4_inv(media.albedo), media.extinction * -dt);
        transmittance = f4_mul(transmittance, f4_exp3(extinction));
    }
    return transmittance;
}

pim_inline scatter_t VEC_CALL ScatterRay(
    const pt_scene_t* scene,
    prng_t* rng,
    float4 ro,
    float4 rd,
    float rayLen)
{
    scatter_t result;
    result.dir = rd;
    result.pos = f4_add(ro, f4_mulvs(rd, rayLen));
    result.pdf = 0.0f;

    float4 transmittance = f4_1;
    float4 scatteredLight = f4_0;
    float pScatter = prng_f32(rng);
    float scattering = 1.0f;
    media_t prevMedia = Media_Sample(&scene->mediaDesc, ro);

    const i32 kSteps = CalcStepCount(rayLen);
    const float dt0 = rayLen / kSteps;
    float t = 0.0f;
    while (t < rayLen)
    {
        float dt = prng_f32(rng) * dt0;
        t = f1_min(t + dt, rayLen);

        float4 P = f4_add(ro, f4_mulvs(rd, t));

        media_t newMedia = Media_Sample(&scene->mediaDesc, P);
        media_t media = Media_Lerp(prevMedia, newMedia, 0.5f);
        prevMedia = newMedia;

        float4 extinction = f4_mulvs(f4_inv(media.albedo), media.extinction * -dt);
        transmittance = f4_mul(transmittance, f4_exp3(extinction));
        scattering *= expf(media.scattering * -dt);

        if (pScatter > scattering)
        {
            float g = media.albedo.w;
            float4 V = f4_neg(rd);

            float4 light, L;
            if (EvaluateLight(scene, rng, P, &light, &L))
            {
                light = f4_mulvs(light, MiePhase(f4_dot3(V, L), g));
                light = f4_mul(light, transmittance);
                scatteredLight = f4_add(scatteredLight, light);
            }

            result.pos = P;
            result.dir = SampleUnitSphere(f2_rand(rng));
            result.pdf = 1.0f / (4.0f * kPi);
            transmittance = f4_mulvs(transmittance, MiePhase(f4_dot3(V, result.dir), g));
            break;
        }
    }

    result.attenuation = transmittance;
    result.irradiance = scatteredLight;

    return result;
}

pt_result_t VEC_CALL pt_trace_ray(
    prng_t* rng,
    const pt_scene_t* scene,
    ray_t ray)
{
    pt_result_t result = { 0 };

    float4 light = f4_0;
    float4 attenuation = f4_1;

    for (i32 b = 0; b < 666; ++b)
    {

        rayhit_t hit = pt_intersect_local(scene, ray, 0.0f, 1 << 20);
        if (hit.type == hit_nothing)
        {
            break;
        }
        if (hit.type == hit_backface)
        {
            break;
        }

        {
            scatter_t scatter = ScatterRay(scene, rng, ray.ro, ray.rd, hit.wuvt.w);
            light = f4_add(light, f4_mul(scatter.irradiance, attenuation));
            attenuation = f4_mul(attenuation, scatter.attenuation);
            if (scatter.pdf > 0.0f)
            {
                // scatter event occurred before reaching surface
                if (b == 0)
                {
                    result.albedo = f4_f3(Media_Albedo(&scene->mediaDesc, scatter.pos));
                    result.normal = f4_f3(Media_Normal(&scene->mediaDesc, scatter.pos));
                }
                attenuation = f4_divvs(attenuation, scatter.pdf);
                ray.ro = scatter.pos;
                ray.rd = scatter.dir;
                goto roulette;
            }
        }

        if (hit.type == hit_sky)
        {
            light = f4_add(light, f4_mul(GetSky(scene, ray.ro, ray.rd), attenuation));
            break;
        }

        surfhit_t surf = GetSurface(scene, ray, hit, b);
        if (b == 0)
        {
            result.albedo = f4_f3(surf.albedo);
            result.normal = f4_f3(surf.N);
        }

        if (b == 0)
        {
            light = f4_add(light, f4_mul(surf.emission, attenuation));
        }
        {
            float4 direct = SampleLights(scene, rng, &surf, &hit, ray.rd);
            light = f4_add(light, f4_mul(direct, attenuation));
        }

        scatter_t scatter = BrdfScatter(rng, &surf, ray.rd);
        if (scatter.pdf <= 0.0f)
        {
            break;
        }
        ray.ro = surf.P;
        ray.rd = scatter.dir;

        attenuation = f4_mul(attenuation, f4_divvs(scatter.attenuation, scatter.pdf));

    roulette:
        {
            float p = f1_clamp(f4_hmax3(attenuation), 0.0f, 0.95f);
            if (prng_f32(rng) < p)
            {
                attenuation = f4_divvs(attenuation, p);
            }
            else
            {
                break;
            }
        }
    }

    result.color = f4_f3(light);
    return result;
}

typedef struct trace_task_s
{
    task_t task;
    pt_trace_t* trace;
    camera_t camera;
} trace_task_t;

static void TraceFn(task_t* pbase, i32 begin, i32 end)
{
    trace_task_t* task = (trace_task_t*)pbase;

    pt_trace_t* trace = task->trace;
    const camera_t camera = task->camera;

    const pt_scene_t* scene = trace->scene;
    float3* pim_noalias color = trace->color;
    float3* pim_noalias albedo = trace->albedo;
    float3* pim_noalias normal = trace->normal;

    const int2 size = trace->imageSize;
    const float sampleWeight = trace->sampleWeight;

    const quat rot = camera.rotation;
    const float4 ro = camera.position;
    const float4 right = quat_right(rot);
    const float4 up = quat_up(rot);
    const float4 fwd = quat_fwd(rot);
    const float2 slope = proj_slope(f1_radians(camera.fovy), (float)size.x / (float)size.y);
    const float2 rcpSize = f2_rcp(i2_f2(size));

    prng_t rng = prng_get();
    for (i32 i = begin; i < end; ++i)
    {
        int2 coord = { i % size.x, i / size.x };

        float2 Xi = f2_mul(f2_tent(f2_rand(&rng)), rcpSize);
        float2 uv = f2_snorm(f2_add(CoordToUv(size, coord), Xi));

        float4 rd = proj_dir(right, up, fwd, slope, uv);
        ray_t ray = { ro, rd };
        pt_result_t result = pt_trace_ray(&rng, scene, ray);
        color[i] = f3_lerp(color[i], result.color, sampleWeight);
        albedo[i] = f3_lerp(albedo[i], result.albedo, sampleWeight);
        normal[i] = f3_lerp(normal[i], result.normal, sampleWeight);
    }
    prng_set(rng);
}

ProfileMark(pm_trace, pt_trace)
void pt_trace(pt_trace_t* desc)
{
    ProfileBegin(pm_trace);

    ASSERT(desc);
    ASSERT(desc->scene);
    ASSERT(desc->camera);
    ASSERT(desc->color);
    ASSERT(desc->albedo);
    ASSERT(desc->normal);

    UpdateScene(desc->scene);

    trace_task_t* task = tmp_calloc(sizeof(*task));
    task->trace = desc;
    task->camera = desc->camera[0];

    const i32 workSize = desc->imageSize.x * desc->imageSize.y;
    task_run(&task->task, TraceFn, workSize);

    ProfileEnd(pm_trace);
}

typedef struct pt_raygen_s
{
    task_t task;
    const pt_scene_t* scene;
    float4 origin;
    float4* colors;
    float4* directions;
} pt_raygen_t;

static void RayGenFn(task_t* pBase, i32 begin, i32 end)
{
    pt_raygen_t* task = (pt_raygen_t*)pBase;
    const pt_scene_t* scene = task->scene;
    const float4 ro = task->origin;

    float4* pim_noalias colors = task->colors;
    float4* pim_noalias directions = task->directions;

    prng_t rng = prng_get();
    for (i32 i = begin; i < end; ++i)
    {
        float2 Xi = f2_rand(&rng);
        float4 rd = SampleUnitSphere(Xi);
        directions[i] = rd;
        pt_result_t result = pt_trace_ray(&rng, scene, (ray_t) { ro, rd });
        colors[i] = f3_f4(result.color, 1.0f);
    }
    prng_set(rng);
}

ProfileMark(pm_raygen, pt_raygen)
pt_results_t pt_raygen(
    pt_scene_t* scene,
    float4 origin,
    i32 count)
{
    ProfileBegin(pm_raygen);

    ASSERT(scene);
    ASSERT(count >= 0);

    UpdateScene(scene);

    pt_raygen_t* task = tmp_calloc(sizeof(*task));
    task->scene = scene;
    task->origin = origin;
    task->colors = tmp_malloc(sizeof(task->colors[0]) * count);
    task->directions = tmp_malloc(sizeof(task->directions[0]) * count);
    task_run(&task->task, RayGenFn, count);

    pt_results_t results =
    {
        .colors = task->colors,
        .directions = task->directions,
    };

    ProfileEnd(pm_raygen);

    return results;
}
