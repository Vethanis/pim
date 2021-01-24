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
#include "common/stringutil.h"
#include "common/serialize.h"
#include "common/atomics.h"
#include "common/time.h"
#include "ui/cimgui.h"

#include "stb/stb_perlin_fork.h"
#include <string.h>

// ----------------------------------------------------------------------------

typedef enum LdsSlot
{
    LdsSlot_Brdf,
    LdsSlot_MeanFreePath,

    LdsSlot_COUNT
} LdsSlot;

typedef struct surfhit_s
{
    float4 P;
    float4 M; // macro normal
    float4 N; // micro normal
    float4 albedo;
    float4 emission;
    float roughness;
    float occlusion;
    float metallic;
    float ior;
    matflag_t flags;
    hittype_t type;
} surfhit_t;

typedef struct scatter_s
{
    float4 pos;
    float4 dir;
    float4 attenuation;
    float4 luminance;
    float pdf;
} scatter_t;

typedef struct lightsample_s
{
    float4 direction;
    float4 luminance;
    float4 wuvt;
    float pdf;
} lightsample_t;

typedef struct media_desc_s
{
    float4 constantAlbedo;
    float4 noiseAlbedo;
    float4 logConstantAlbedo;
    float4 logNoiseAlbedo;
    float rcpMajorant;
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
    float phaseDirA;
    float phaseDirB;
    float phaseBlend;
} media_desc_t;

typedef struct media_s
{
    float4 logAlbedo;
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
    // emissive index
    // [vertCount]
    i32* pim_noalias vertToEmit;

    // emissive triangle indices
    // [emissiveCount]
    i32* pim_noalias emissives;

    // grid of discrete light distributions
    grid_t lightGrid;
    // [lightGrid.size]
    dist1d_t* pim_noalias lightDists;

    // surface description, indexed by matIds
    // [matCount]
    material_t* pim_noalias materials;

    cubemap_t* pim_noalias sky;

    // array lengths
    i32 vertCount;
    i32 matCount;
    i32 emissiveCount;
    // parameters
    media_desc_t mediaDesc;
} pt_scene_t;

// ----------------------------------------------------------------------------

static void OnRtcError(void* user, RTCError error, const char* msg);
static bool InitRTC(void);
static void InitSamplers(void);
static void InitPixelDist(void);
static void ShutdownPixelDist(void);
static RTCScene RtcNewScene(const pt_scene_t*const pim_noalias scene);
static void FlattenDrawables(pt_scene_t*const pim_noalias scene);
static float EmissionPdf(
    pt_sampler_t*const pim_noalias sampler,
    const pt_scene_t*const pim_noalias scene,
    i32 iVert,
    i32 attempts);
static void CalcEmissionPdfFn(task_t* pbase, i32 begin, i32 end);
static void SetupEmissives(pt_scene_t*const pim_noalias scene);
static void SetupLightGridFn(task_t* pbase, i32 begin, i32 end);
static void SetupLightGrid(pt_scene_t*const pim_noalias scene);

static void media_desc_new(media_desc_t *const desc);
static void media_desc_update(media_desc_t *const desc);
static void media_desc_gui(media_desc_t *const desc);
static void media_desc_load(media_desc_t *const desc, const char* name);
static void media_desc_save(media_desc_t const *const desc, const char* name);

// ----------------------------------------------------------------------------

pim_inline RTCRay VEC_CALL RtcNewRay(float4 ro, float4 rd, float tNear, float tFar);
pim_inline RTCRayHit VEC_CALL RtcIntersect(
    RTCScene scene,
    float4 ro,
    float4 rd,
    float tNear,
    float tFar);
pim_inline float4 VEC_CALL GetPosition(
    const pt_scene_t *const pim_noalias scene,
    rayhit_t hit);
pim_inline float4 VEC_CALL GetNormal(
    const pt_scene_t *const pim_noalias scene,
    rayhit_t hit);
pim_inline float2 VEC_CALL GetUV(
    const pt_scene_t *const pim_noalias scene,
    rayhit_t hit);
pim_inline float VEC_CALL GetArea(const pt_scene_t*const pim_noalias scene, i32 iVert);
pim_inline material_t const *const pim_noalias VEC_CALL GetMaterial(
    const pt_scene_t*const pim_noalias scene,
    rayhit_t hit);
pim_inline float4 VEC_CALL GetSky(
    const pt_scene_t*const pim_noalias scene,
    float4 ro,
    float4 rd);
pim_inline float4 VEC_CALL GetEmission(
    const pt_scene_t*const pim_noalias scene,
    float4 ro,
    float4 rd,
    rayhit_t hit,
    i32 bounce);
pim_inline surfhit_t VEC_CALL GetSurface(
    const pt_scene_t*const pim_noalias scene,
    float4 ro,
    float4 rd,
    rayhit_t hit,
    i32 bounce);
pim_inline rayhit_t VEC_CALL pt_intersect_local(
    const pt_scene_t*const pim_noalias scene,
    float4 ro,
    float4 rd,
    float tNear,
    float tFar);

// ----------------------------------------------------------------------------

pim_inline float4 VEC_CALL RefractBrdfEval(
    pt_sampler_t*const pim_noalias sampler,
    float4 I,
    const surfhit_t* surf,
    float4 L);
pim_inline float4 VEC_CALL BrdfEval(
    pt_sampler_t*const pim_noalias sampler,
    float4 I,
    const surfhit_t* surf,
    float4 L);
pim_inline scatter_t VEC_CALL RefractScatter(
    pt_sampler_t*const pim_noalias sampler,
    const surfhit_t* surf,
    float4 I);
pim_inline scatter_t VEC_CALL BrdfScatter(
    pt_sampler_t*const pim_noalias sampler,
    const surfhit_t* surf,
    float4 I);

// ----------------------------------------------------------------------------

pim_inline bool VEC_CALL LightSelect(
    pt_sampler_t*const pim_noalias sampler,
    pt_scene_t*const pim_noalias scene,
    float4 position,
    i32* iVertOut,
    float* pdfOut);
pim_inline lightsample_t VEC_CALL LightSample(
    pt_sampler_t *const pim_noalias sampler,
    pt_scene_t *const pim_noalias scene,
    float4 ro,
    i32 iVert,
    i32 bounce);
pim_inline float VEC_CALL LightSelectPdf(
    pt_scene_t *const pim_noalias scene,
    i32 iVert,
    float4 ro);
pim_inline float VEC_CALL LightEvalPdf(
    pt_sampler_t *const pim_noalias sampler,
    pt_scene_t *const pim_noalias scene,
    float4 ro,
    float4 rd,
    rayhit_t *const pim_noalias hitOut);

// ----------------------------------------------------------------------------

pim_inline float4 VEC_CALL EstimateDirect(
    pt_sampler_t *const pim_noalias sampler,
    pt_scene_t *const pim_noalias scene,
    surfhit_t const *const pim_noalias surf,
    rayhit_t const *const pim_noalias srcHit,
    float4 I,
    i32 bounce);
pim_inline bool VEC_CALL EvaluateLight(
    pt_sampler_t *const pim_noalias sampler,
    pt_scene_t *const pim_noalias scene,
    float4 P,
    float4 *const pim_noalias lightOut,
    float4 *const pim_noalias dirOut,
    i32 bounce);

// ----------------------------------------------------------------------------

pim_inline float4 VEC_CALL AlbedoToLogAlbedo(float4 albedo);
pim_inline float2 VEC_CALL Media_Density(media_desc_t const *const desc, float4 P);
pim_inline media_t VEC_CALL Media_Sample(media_desc_t const *const desc, float4 P);
pim_inline float4 VEC_CALL Media_Albedo(media_desc_t const *const desc, float4 P);
pim_inline float4 VEC_CALL Media_Normal(media_desc_t const *const desc, float4 P);
pim_inline media_t VEC_CALL Media_Lerp(media_t lhs, media_t rhs, float t);
pim_inline float VEC_CALL SampleFreePath(float Xi, float rcpU);
pim_inline float VEC_CALL FreePathPdf(float t, float u);
pim_inline float4 VEC_CALL CalcMajorant(media_desc_t const *const desc);
pim_inline float4 VEC_CALL CalcControl(media_desc_t const *const desc);
pim_inline float4 VEC_CALL CalcResidual(float4 control, float4 majorant);
pim_inline float VEC_CALL CalcPhase(
    media_desc_t const *const desc,
    float cosTheta);
pim_inline float4 VEC_CALL SamplePhaseDir(
    pt_sampler_t*const pim_noalias sampler,
    media_desc_t const *const pim_noalias desc,
    float4 rd);
pim_inline float4 VEC_CALL CalcTransmittance(
    pt_sampler_t*const pim_noalias sampler,
    const pt_scene_t*const pim_noalias scene,
    float4 ro,
    float4 rd,
    float rayLen);
pim_inline scatter_t VEC_CALL ScatterRay(
    pt_sampler_t*const pim_noalias sampler,
    pt_scene_t*const pim_noalias scene,
    float4 ro,
    float4 rd,
    float rayLen,
    i32 bounce);

// ----------------------------------------------------------------------------

static void TraceFn(void* pbase, i32 begin, i32 end);
static void RayGenFn(void* pBase, i32 begin, i32 end);
pim_inline void VEC_CALL LightOnHit(
    pt_sampler_t*const pim_noalias sampler,
    pt_scene_t*const pim_noalias scene,
    float4 ro,
    float4 lum,
    i32 iVert);
static void UpdateDists(pt_scene_t *const pim_noalias scene);

// ----------------------------------------------------------------------------

pim_inline pt_sampler_t VEC_CALL GetSampler(void);
pim_inline void VEC_CALL SetSampler(pt_sampler_t sampler);

pim_inline float VEC_CALL Sample1D(pt_sampler_t*const pim_noalias sampler);
pim_inline float2 VEC_CALL Sample2D(pt_sampler_t*const pim_noalias sampler);

pim_inline float VEC_CALL RoulettePrng(pt_sampler_t *const pim_noalias sampler)
{
    float Xi = sampler->Xi[0];
    Xi += (kGoldenRatio - 1.0f);
    Xi = (Xi >= 1.0f) ? (Xi - 1.0f) : Xi;
    sampler->Xi[0] = Xi;
    return Xi;
}
pim_inline float VEC_CALL LightSelectPrng(pt_sampler_t *const pim_noalias sampler)
{
    float Xi = sampler->Xi[1];
    Xi += (kSqrt2 - 1.0f);
    Xi = (Xi >= 1.0f) ? (Xi - 1.0f) : Xi;
    sampler->Xi[1] = Xi;
    return Xi;
}
pim_inline float VEC_CALL BrdfPrng(pt_sampler_t*const pim_noalias sampler)
{
    float Xi = sampler->Xi[2];
    Xi += (kSqrt3 - 1.0f);
    Xi = (Xi >= 1.0f) ? (Xi - 1.0f) : Xi;
    sampler->Xi[2] = Xi;
    return Xi;
}
pim_inline float VEC_CALL ScatterPrng(pt_sampler_t *const pim_noalias sampler)
{
    float Xi = sampler->Xi[3];
    Xi += (kSqrt5 - 2.0f);
    Xi = (Xi >= 1.0f) ? (Xi - 1.0f) : Xi;
    sampler->Xi[3] = Xi;
    return Xi;
}
pim_inline float VEC_CALL PhaseDirPrng(pt_sampler_t *const pim_noalias sampler)
{
    float Xi = sampler->Xi[4];
    Xi += (kSqrt7 - 2.0f);
    Xi = (Xi >= 1.0f) ? (Xi - 1.0f) : Xi;
    sampler->Xi[4] = Xi;
    return Xi;
}
pim_inline float2 VEC_CALL UvPrng(pt_sampler_t *const pim_noalias sampler)
{
    float Xi = sampler->Xi[5];
    Xi += (kSqrt11 - 3.0f);
    Xi = (Xi >= 1.0f) ? (Xi - 1.0f) : Xi;
    sampler->Xi[5] = Xi;
    float Yi = sampler->Xi[6];
    Yi += (kSqrt17 - 4.0f);
    Yi = (Yi >= 1.0f) ? (Yi - 1.0f) : Yi;
    sampler->Xi[6] = Yi;
    return f2_v(Xi, Yi);
}
pim_inline float2 VEC_CALL DofPrng(pt_sampler_t *const pim_noalias sampler)
{
    float Xi = sampler->Xi[7];
    Xi += (kSqrt13 - 3.0f);
    Xi = (Xi >= 1.0f) ? (Xi - 1.0f) : Xi;
    sampler->Xi[7] = Xi;
    return f2_v(Xi, Sample1D(sampler));
}
// ----------------------------------------------------------------------------

static cvar_t cv_pt_dist_meters =
{
    .type = cvart_float,
    .name = "pt_dist_meters",
    .value = "1.5",
    .minFloat = 0.1f,
    .maxFloat = 20.0f,
    .desc = "path tracer light distribution meters per cell"
};

static cvar_t cv_pt_dist_alpha =
{
    .type = cvart_float,
    .name = "pt_dist_alpha",
    .value = "0.5",
    .minFloat = 0.0f,
    .maxFloat = 1.0f,
    .desc = "path tracer light distribution update amount",
};

static cvar_t cv_pt_dist_samples =
{
    .type = cvart_int,
    .name = "pt_dist_samples",
    .value = "1000",
    .minInt = 30,
    .maxInt = 1 << 20,
    .desc = "path tracer light distribution minimum samples per update",
};

static RTCDevice ms_device;
static dist1d_t ms_pixeldist;
static pt_sampler_t ms_samplers[kMaxThreads];

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

static void InitPixelDist(void)
{
    const i32 kSamples = 16;
    const float dt = 1.0f / kSamples;
    dist1d_t dist;
    dist1d_new(&dist, kSamples);
    for (i32 i = 0; i < kSamples; ++i)
    {
        float x = f1_lerp(0.0f, 2.0f, (i + 0.5f) * dt);
        dist.pdf[i] = f1_gauss(x, 0.0f, 0.5f);
    }
    dist1d_bake(&dist);
    ms_pixeldist = dist;
}

static void ShutdownPixelDist(void)
{
    dist1d_del(&ms_pixeldist);
}

static void InitSamplers(void)
{
    const i32 numthreads = task_thread_ct();

    prng_t rng = prng_get();
    for (i32 i = 0; i < numthreads; ++i)
    {
        ms_samplers[i].rng.state = prng_u64(&rng);
        for (i32 j = 0; j < NELEM(ms_samplers[i].Xi); ++j)
        {
            ms_samplers[i].Xi[j] = prng_f32(&rng);
        }
    }
    prng_set(rng);
}

void pt_sys_init(void)
{
    cvar_reg(&cv_pt_dist_meters);
    cvar_reg(&cv_pt_dist_alpha);
    cvar_reg(&cv_pt_dist_samples);

    InitRTC();
    InitSamplers();
    InitPixelDist();
}

void pt_sys_update(void)
{

}

void pt_sys_shutdown(void)
{
    rtc.ReleaseDevice(ms_device);
    ms_device = NULL;
    ShutdownPixelDist();
}

pt_sampler_t VEC_CALL pt_sampler_get(void) { return GetSampler(); }
void VEC_CALL pt_sampler_set(pt_sampler_t sampler) { SetSampler(sampler); }
float2 VEC_CALL pt_sample_2d(pt_sampler_t*const pim_noalias sampler) { return Sample2D(sampler); }
float VEC_CALL pt_sample_1d(pt_sampler_t*const pim_noalias sampler) { return Sample1D(sampler); }

pim_inline RTCRay VEC_CALL RtcNewRay(
    float4 ro,
    float4 rd,
    float tNear,
    float tFar)
{
    ASSERT(tFar > tNear);
    ASSERT(tNear >= 0.0f);
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

pim_inline RTCRayHit VEC_CALL RtcIntersect(
    RTCScene scene,
    float4 ro,
    float4 rd,
    float tNear,
    float tFar)
{
    RTCIntersectContext ctx = { 0 };
    rtcInitIntersectContext(&ctx);
    RTCRayHit rayHit = { 0 };
    rayHit.ray = RtcNewRay(ro, rd, tNear, tFar);
    rayHit.hit.primID = RTC_INVALID_GEOMETRY_ID; // triangle index
    rayHit.hit.geomID = RTC_INVALID_GEOMETRY_ID; // object id
    rayHit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID; // instance id
    rtc.Intersect1(scene, &ctx, &rayHit);
    return rayHit;
}

// ros[i].w = tNear
// rds[i].w = tFar
pim_inline RTCRayHit16 VEC_CALL RtcIntersect16(
    RTCScene scene,
    float4 const *const pim_noalias ros,
    float4 const *const pim_noalias rds)
{
    RTCRayHit16 rayHit = { 0 };
    RTCIntersectContext ctx = { 0 };
    rtcInitIntersectContext(&ctx);
    i32 valid[16] = { 0 };
    for (i32 i = 0; i < 16; ++i)
    {
        rayHit.ray.org_x[i] = ros[i].x;
        rayHit.ray.org_y[i] = ros[i].y;
        rayHit.ray.org_z[i] = ros[i].z;
        rayHit.ray.tnear[i] = ros[i].w;
        rayHit.ray.dir_x[i] = rds[i].x;
        rayHit.ray.dir_y[i] = rds[i].y;
        rayHit.ray.dir_z[i] = rds[i].z;
        rayHit.ray.tfar[i] = rds[i].w;
        rayHit.ray.mask[i] = -1;
        rayHit.ray.flags[i] = 0;
        rayHit.hit.primID[i] = RTC_INVALID_GEOMETRY_ID;
        rayHit.hit.geomID[i] = RTC_INVALID_GEOMETRY_ID;
        rayHit.hit.instID[0][i] = RTC_INVALID_GEOMETRY_ID;
        valid[i] = -1;
    }
    rtc.Intersect16(valid, scene, &ctx, &rayHit);
    return rayHit;
}

// ros[i].w = tNear
// rds[i].w = tFar (place at least 1 millimeter before surface)
// on miss (visible): tFar unchanged
// on hit (occluded): tFar < 0
pim_inline void VEC_CALL RtcOccluded16(
    RTCScene scene,
    const float4* pim_noalias ros,
    const float4* pim_noalias rds,
    bool* pim_noalias visibles)
{
    RTCRay16 rayHit = { 0 };
    RTCIntersectContext ctx = { 0 };
    rtcInitIntersectContext(&ctx);
    i32 valid[16] = { 0 };
    for (i32 i = 0; i < 16; ++i)
    {
        rayHit.org_x[i] = ros[i].x;
        rayHit.org_y[i] = ros[i].y;
        rayHit.org_z[i] = ros[i].z;
        rayHit.tnear[i] = ros[i].w;
        rayHit.dir_x[i] = rds[i].x;
        rayHit.dir_y[i] = rds[i].y;
        rayHit.dir_z[i] = rds[i].z;
        rayHit.tfar[i] = rds[i].w;
        rayHit.mask[i] = -1;
        rayHit.flags[i] = 0;
        valid[i] = -1;
    }
    rtc.Occluded16(valid, scene, &ctx, &rayHit);
    for (i32 i = 0; i < 16; ++i)
    {
        visibles[i] = rayHit.tfar[i] > 0.0f;
    }
}

typedef pim_alignas(16) struct PointQueryUserData
{
    const float4* pim_noalias positions;
    float distance;
    u32 primID;
    u32 geomID;
    bool frontFace;
} PointQueryUserData;

pim_inline bool RtcPointQueryFn(RTCPointQueryFunctionArguments* args)
{
    PointQueryUserData* pim_noalias usr = args->userPtr;
    const u32 primID = args->primID;
    if (primID != RTC_INVALID_GEOMETRY_ID)
    {
        RTCPointQuery* pim_noalias query = args->query;
        const float4* pim_noalias positions = usr->positions;
        const u32 iVert = primID * 3;
        float4 A = positions[iVert + 0];
        float4 B = positions[iVert + 1];
        float4 C = positions[iVert + 2];
        float4 P = { query->x, query->y, query->z, query->radius };
        float distance = sdTriangle3D(A, B, C, P);
        bool frontFace = distance > 0.0f;
        distance = f1_abs(distance);
        if (distance < P.w)
        {
            if (distance < usr->distance)
            {
                usr->distance = distance;
                usr->primID = primID;
                usr->geomID = args->geomID;
                usr->frontFace = frontFace;
                query->radius = f1_min(query->radius, distance);
                return true;
            }
        }
    }
    return false;
}

pim_inline PointQueryUserData VEC_CALL RtcPointQuery(const pt_scene_t*const pim_noalias scene, float4 pt)
{
    PointQueryUserData usr = { 0 };
    usr.positions = scene->positions;
    usr.distance = 1 << 20;
    usr.primID = RTC_INVALID_GEOMETRY_ID;
    usr.geomID = RTC_INVALID_GEOMETRY_ID;
    RTCPointQuery query = { 0 };
    RTCPointQueryContext ctx;
    rtcInitPointQueryContext(&ctx);
    query.x = pt.x;
    query.y = pt.y;
    query.z = pt.z;
    query.radius = pt.w;
    query.time = 0.0f;
    rtc.PointQuery(scene->rtcScene, &query, &ctx, RtcPointQueryFn, &usr);
    return usr;
}

static RTCScene RtcNewScene(const pt_scene_t*const pim_noalias scene)
{
    RTCScene rtcScene = rtc.NewScene(ms_device);
    ASSERT(rtcScene);
    if (!rtcScene)
    {
        return NULL;
    }

    rtc.SetSceneBuildQuality(rtcScene, RTC_BUILD_QUALITY_HIGH);

    RTCGeometry geom = rtc.NewGeometry(ms_device, RTC_GEOMETRY_TYPE_TRIANGLE);
    ASSERT(geom);

    const i32 vertCount = scene->vertCount;
    const i32 triCount = vertCount / 3;

    if (vertCount > 0)
    {
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
    }

    if (triCount > 0)
    {
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
    }

    rtc.CommitGeometry(geom);
    rtc.AttachGeometry(rtcScene, geom);
    rtc.ReleaseGeometry(geom);
    geom = NULL;

    rtc.CommitScene(rtcScene);

    return rtcScene;
}

static void FlattenDrawables(pt_scene_t*const pim_noalias scene)
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
        mesh_t const *const mesh = mesh_get(meshes[i]);
        if (!mesh)
        {
            continue;
        }

        const i32 meshLen = mesh->length;
        float4 const *const pim_noalias meshPositions = mesh->positions;
        float4 const *const pim_noalias meshNormals = mesh->normals;
        float4 const *const pim_noalias meshUvs = mesh->uvs;

        const i32 vertBack = vertCount;
        const i32 matBack = matCount;
        vertCount += meshLen;
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

        for (i32 j = 0; j < meshLen; ++j)
        {
            positions[vertBack + j] = f4x4_mul_pt(M, meshPositions[j]);
        }

        for (i32 j = 0; j < meshLen; ++j)
        {
            normals[vertBack + j] = f4_normalize3(f3x3_mul_col(IM, meshNormals[j]));
        }

        for (i32 j = 0; (j + 3) <= meshLen; j += 3)
        {
            float4 uva = meshUvs[j + 0];
            float4 uvb = meshUvs[j + 1];
            float4 uvc = meshUvs[j + 2];
            float2 UA = f2_v(uva.x, uva.y);
            float2 UB = f2_v(uvb.x, uvb.y);
            float2 UC = f2_v(uvc.x, uvc.y);
            uvs[vertBack + j + 0] = UA;
            uvs[vertBack + j + 1] = UB;
            uvs[vertBack + j + 2] = UC;
        }

        for (i32 j = 0; j < meshLen; ++j)
        {
            matIds[vertBack + j] = matBack;
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
    pt_sampler_t*const pim_noalias sampler,
    const pt_scene_t*const pim_noalias scene,
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
    {
        texture_t const *const romeMap = texture_get(mat->rome);
        if (romeMap)
        {
            u32 const *const pim_noalias texels = romeMap->texels;
            const int2 texSize = romeMap->size;

            const float2* pim_noalias uvs = scene->uvs;
            const float2 UA = uvs[iVert + 0];
            const float2 UB = uvs[iVert + 1];
            const float2 UC = uvs[iVert + 2];

            i32 hits = 0;
            for (i32 i = 0; i < attempts; ++i)
            {
                float4 wuv = SampleBaryCoord(Sample2D(sampler));
                float2 uv = f2_blend(UA, UB, UC, wuv);
                float sample = UvWrapPow2_c32(texels, texSize, uv).w;
                if (sample > kThreshold)
                {
                    ++hits;
                }
            }
            return (float)hits / (float)attempts;
        }
        else
        {
            return 0.0f;
        }
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
    const pt_scene_t*const pim_noalias scene = task->scene;
    const i32 attempts = task->attempts;
    float* pim_noalias pdfs = task->pdfs;

    pt_sampler_t sampler = pt_sampler_get();
    for (i32 i = begin; i < end; ++i)
    {
        pdfs[i] = EmissionPdf(&sampler, scene, i * 3, attempts);
    }
    pt_sampler_set(sampler);
}

static void SetupEmissives(pt_scene_t*const pim_noalias scene)
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
    i32* pim_noalias vertToEmit = perm_malloc(sizeof(vertToEmit[0]) * vertCount);

    const float* pim_noalias taskPdfs = task->pdfs;
    for (i32 iTri = 0; iTri < triCount; ++iTri)
    {
        i32 iVert = iTri * 3;
        vertToEmit[iVert + 0] = -1;
        vertToEmit[iVert + 1] = -1;
        vertToEmit[iVert + 2] = -1;
        float pdf = taskPdfs[iTri];
        if (pdf > 0.01f)
        {
            vertToEmit[iVert + 0] = emissiveCount;
            vertToEmit[iVert + 1] = emissiveCount;
            vertToEmit[iVert + 2] = emissiveCount;
            ++emissiveCount;
            PermReserve(emissives, emissiveCount);
            emissives[emissiveCount - 1] = iVert;
        }
    }

    scene->vertToEmit = vertToEmit;
    scene->emissiveCount = emissiveCount;
    scene->emissives = emissives;
}

typedef struct task_SetupLightGrid
{
    task_t task;
    pt_scene_t* scene;
} task_SetupLightGrid;

static void SetupLightGridFn(task_t* pbase, i32 begin, i32 end)
{
    task_SetupLightGrid* task = (task_SetupLightGrid*)pbase;

    pt_scene_t*const pim_noalias scene = task->scene;

    const grid_t grid = scene->lightGrid;
    dist1d_t *const pim_noalias dists = scene->lightDists;

    float4 const *const pim_noalias positions = scene->positions;
    i32 const *const pim_noalias matIds = scene->matIds;
    material_t const *const pim_noalias materials = scene->materials;

    const i32 emissiveCount = scene->emissiveCount;
    i32 const *const pim_noalias emissives = scene->emissives;

    const float metersPerCell = cvar_get_float(&cv_pt_dist_meters);
    const float radius = metersPerCell * 0.666f;
    pt_sampler_t sampler = GetSampler();

    float4 hamm[16];
    for (i32 i = 0; i < NELEM(hamm); ++i)
    {
        float2 Xi = Hammersley2D(i, NELEM(hamm));
        hamm[i] = SampleUnitSphere(Xi);
    }

    RTCScene rtScene = scene->rtcScene;

    for (i32 i = begin; i < end; ++i)
    {
        float4 position = grid_position(&grid, i);
        position.w = radius + 0.01f * kMilli;
        {
            PointQueryUserData query = RtcPointQuery(scene, position);
            if (query.distance > radius)
            {
                // far from a surface. might be inside or outside the map
                // toss some rays and see if they are backfaces
                float hitcount = 0.0f;
                for (i32 j = 0; j < NELEM(hamm); ++j)
                {
                    rayhit_t hit = pt_intersect_local(scene, position, hamm[j], 0.0f, 1 << 20);
                    if (hit.type == hit_triangle)
                    {
                        ++hitcount;
                    }
                }
                float hitratio = hitcount / NELEM(hamm);
                if (hitratio < 0.5f)
                {
                    continue;
                }
            }
        }

        dist1d_t dist = { 0 };
        dist1d_new(&dist, emissiveCount);

        for (i32 iList = 0; iList < emissiveCount; ++iList)
        {
            i32 iVert = emissives[iList];
            i32 iMat = matIds[iVert];
            float4 A = positions[iVert + 0];
            float4 B = positions[iVert + 1];
            float4 C = positions[iVert + 2];

            i32 hits = 0;
            float4 ros[16];
            float4 rds[16];
            bool visibles[16];
            const i32 hitAttempts = 32;
            const i32 loopIterations = hitAttempts / NELEM(ros);
            for (i32 j = 0; j < loopIterations; ++j)
            {
                for (i32 k = 0; k < NELEM(ros); ++k)
                {
                    float4 t = f4_mulvs(f4_lerpsv(-1.5f, 1.5f, f4_rand(&sampler.rng)), radius);
                    float4 ro = f4_add(position, t);
                    ro.w = 0.0f;
                    float4 at = f4_blend(A, B, C, SampleBaryCoord(Sample2D(&sampler)));
                    float4 rd = f4_sub(at, ro);
                    float dist = f4_length3(rd);
                    rd = f4_divvs(rd, dist);
                    rd.w = dist - 0.01f * kMilli;
                    ros[k] = ro;
                    rds[k] = rd;
                }
                RtcOccluded16(rtScene, ros, rds, visibles);
                for (i32 k = 0; k < NELEM(ros); ++k)
                {
                    hits += visibles[k] ? 1 : 0;
                }
            }
            float hitPdf = (float)hits / (float)hitAttempts;

            dist.pdf[iList] = hitPdf;
        }

        dist1d_bake(&dist);
        dists[i] = dist;
    }
    SetSampler(sampler);
}

static void SetupLightGrid(pt_scene_t*const pim_noalias scene)
{
    if (scene->vertCount > 0)
    {
        box_t bounds = box_from_pts(scene->positions, scene->vertCount);
        float metersPerCell = cvar_get_float(&cv_pt_dist_meters);
        grid_t grid;
        grid_new(&grid, bounds, 1.0f / metersPerCell);
        const i32 len = grid_len(&grid);
        scene->lightGrid = grid;
        scene->lightDists = tex_calloc(sizeof(scene->lightDists[0]) * len);

        task_SetupLightGrid* task = tmp_calloc(sizeof(*task));
        task->scene = scene;

        task_run(task, SetupLightGridFn, len);
    }
}

void pt_scene_update(pt_scene_t*const pim_noalias scene)
{
    guid_t skyname = guid_str("sky");
    cubemaps_t* maps = Cubemaps_Get();
    i32 iSky = Cubemaps_Find(maps, skyname);
    if (iSky != -1)
    {
        scene->sky = maps->cubemaps + iSky;
    }
    else
    {
        scene->sky = NULL;
    }
    UpdateDists(scene);
}

pt_scene_t* pt_scene_new(void)
{
    ASSERT(ms_device);
    if (!ms_device)
    {
        return NULL;
    }

    pt_scene_t*const pim_noalias scene = perm_calloc(sizeof(*scene));
    pt_scene_update(scene);

    FlattenDrawables(scene);
    SetupEmissives(scene);
    media_desc_new(&scene->mediaDesc);
    scene->rtcScene = RtcNewScene(scene);
    SetupLightGrid(scene);

    return scene;
}

void pt_scene_del(pt_scene_t*const pim_noalias scene)
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
        pim_free(scene->vertToEmit);

        pim_free(scene->materials);

        pim_free(scene->emissives);

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

void pt_scene_gui(pt_scene_t*const pim_noalias scene)
{
    if (scene && igCollapsingHeader1("pt scene"))
    {
        igIndent(0.0f);
        igText("Vertex Count: %d", scene->vertCount);
        igText("Material Count: %d", scene->matCount);
        igText("Emissive Count: %d", scene->emissiveCount);
        media_desc_gui(&scene->mediaDesc);
        igUnindent(0.0f);
    }
}

void pt_trace_new(
    pt_trace_t* trace,
    pt_scene_t*const pim_noalias scene,
    int2 imageSize)
{
    ASSERT(trace);
    ASSERT(scene);
    if (trace)
    {
        const i32 texelCount = imageSize.x * imageSize.y;
        ASSERT(texelCount > 0);

        trace->imageSize = imageSize;
        trace->scene = scene;
        trace->sampleWeight = 1.0f;
        trace->color = tex_calloc(sizeof(trace->color[0]) * texelCount);
        trace->albedo = tex_calloc(sizeof(trace->albedo[0]) * texelCount);
        trace->normal = tex_calloc(sizeof(trace->normal[0]) * texelCount);
        trace->denoised = tex_calloc(sizeof(trace->denoised[0]) * texelCount);
        dofinfo_new(&trace->dofinfo);
    }
}

void pt_trace_del(pt_trace_t* trace)
{
    if (trace)
    {
        pim_free(trace->color);
        pim_free(trace->albedo);
        pim_free(trace->normal);
        pim_free(trace->denoised);
        memset(trace, 0, sizeof(*trace));
    }
}

void pt_trace_gui(pt_trace_t* trace)
{
    if (trace && igCollapsingHeader1("pt trace"))
    {
        igIndent(0.0f);
        dofinfo_gui(&trace->dofinfo);
        pt_scene_gui(trace->scene);
        igUnindent(0.0f);
    }
}

void dofinfo_new(dofinfo_t* dof)
{
    if (dof)
    {
        dof->aperture = 5.0f * kMilli;
        dof->focalLength = 6.0f;
        dof->bladeCount = 5;
        dof->bladeRot = kPi / 10.0f;
        dof->focalPlaneCurvature = 0.05f;
    }
}

void dofinfo_gui(dofinfo_t* dof)
{
    if (dof && igCollapsingHeader1("dofinfo"))
    {
        float aperture = dof->aperture / kMilli;
        igSliderFloat("Aperture, millimeters", &aperture, 0.1f, 100.0f);
        dof->aperture = aperture * kMilli;
        float focalLength = log2f(dof->focalLength);
        igSliderFloat("Focal Length, log2 meters", &focalLength, -5.0f, 10.0f);
        dof->focalLength = exp2f(focalLength);
        igSliderFloat("Focal Plane Curvature", &dof->focalPlaneCurvature, 0.0f, 1.0f);
        igSliderInt("Blade Count", &dof->bladeCount, 3, 666, "%d");
        igSliderFloat("Blade Rotation", &dof->bladeRot, 0.0f, kTau);
    }
}

pim_inline float4 VEC_CALL GetPosition(
    const pt_scene_t *const pim_noalias scene,
    rayhit_t hit)
{
    float4 const *const pim_noalias positions = scene->positions;
    return f4_blend(
        positions[hit.index + 0],
        positions[hit.index + 1],
        positions[hit.index + 2],
        hit.wuvt);
}

pim_inline float4 VEC_CALL GetNormal(
    const pt_scene_t *const pim_noalias scene,
    rayhit_t hit)
{
    float4 const *const pim_noalias normals = scene->normals;
    float4 N = f4_blend(
        normals[hit.index + 0],
        normals[hit.index + 1],
        normals[hit.index + 2],
        hit.wuvt);
    N = (f4_dot3(hit.normal, N) > 0.0f) ? N : f4_neg(N);
    return f4_normalize3(N);
}

pim_inline float2 VEC_CALL GetUV(
    const pt_scene_t *const pim_noalias scene,
    rayhit_t hit)
{
    float2 const *const pim_noalias uvs = scene->uvs;
    return f2_blend(
        uvs[hit.index + 0],
        uvs[hit.index + 1],
        uvs[hit.index + 2],
        hit.wuvt);
}

pim_inline float VEC_CALL GetArea(const pt_scene_t *const pim_noalias scene, i32 iVert)
{
    float4 const *const pim_noalias positions = scene->positions;
    ASSERT(iVert >= 0);
    ASSERT((iVert + 2) < scene->vertCount);
    return TriArea3D(positions[iVert + 0], positions[iVert + 1], positions[iVert + 2]);
}

pim_inline material_t const *const pim_noalias VEC_CALL GetMaterial(
    const pt_scene_t *const pim_noalias scene,
    rayhit_t hit)
{
    i32 iVert = hit.index;
    ASSERT(iVert >= 0);
    ASSERT(iVert < scene->vertCount);
    i32 matIndex = scene->matIds[iVert];
    ASSERT(matIndex >= 0);
    ASSERT(matIndex < scene->matCount);
    return &scene->materials[matIndex];
}

pim_inline float4 VEC_CALL GetSky(
    const pt_scene_t*const pim_noalias scene,
    float4 ro,
    float4 rd)
{
    if (scene->sky)
    {
        return f3_f4(Cubemap_ReadColor(scene->sky, rd), 1.0f);
    }
    return f4_0;
}

pim_inline float4 VEC_CALL GetEmission(
    const pt_scene_t*const pim_noalias scene,
    float4 ro,
    float4 rd,
    rayhit_t hit,
    i32 bounce)
{
    material_t const *const pim_noalias mat = GetMaterial(scene, hit);
    if (hit.flags & matflag_sky)
    {
        return GetSky(scene, ro, rd);
    }
    else
    {
        float2 uv = GetUV(scene, hit);
        float4 albedo = f4_1;
        {
            texture_t const *const tex = texture_get(mat->albedo);
            if (tex)
            {
                albedo = UvBilinearWrapPow2_c32_fast(tex->texels, tex->size, uv);
            }
        }
        float e = 0.0f;
        {
            texture_t const *const tex = texture_get(mat->rome);
            if (tex)
            {
                e = UvBilinearWrapPow2_c32_fast(tex->texels, tex->size, uv).w;
            }
        }
        return UnpackEmission(albedo, e);
    }
}

pim_inline surfhit_t VEC_CALL GetSurface(
    const pt_scene_t *const pim_noalias scene,
    float4 ro,
    float4 rd,
    rayhit_t hit,
    i32 bounce)
{
    surfhit_t surf;
    surf.type = hit.type;

    material_t const *const pim_noalias mat = GetMaterial(scene, hit);
    surf.flags = mat->flags;
    surf.ior = mat->ior;
    float2 uv = GetUV(scene, hit);
    surf.M = GetNormal(scene, hit);
    surf.N = surf.M;
    surf.P = GetPosition(scene, hit);
    surf.P = f4_add(surf.P, f4_mulvs(surf.M, 0.01f * kMilli));

    if (hit.flags & matflag_sky)
    {
        surf.albedo = f4_0;
        surf.roughness = 1.0f;
        surf.occlusion = 0.0f;
        surf.metallic = 0.0f;
        surf.emission = GetSky(scene, ro, rd);
    }
    else
    {
        {
            texture_t const *const pim_noalias tex = texture_get(mat->normal);
            if (tex)
            {
                float4 Nts = UvBilinearWrapPow2_xy16(tex->texels, tex->size, uv);
                surf.N = TanToWorld(surf.N, Nts);
            }
        }

        surf.albedo = f4_1;
        {
            texture_t const *const pim_noalias tex = texture_get(mat->albedo);
            if (tex)
            {
                surf.albedo = UvBilinearWrapPow2_c32_fast(tex->texels, tex->size, uv);
            }
        }

        float4 rome = f4_v(0.5f, 1.0f, 0.0f, 0.0f);
        {
            texture_t const *const pim_noalias tex = texture_get(mat->rome);
            if (tex)
            {
                rome = UvBilinearWrapPow2_c32_fast(tex->texels, tex->size, uv);
            }
        }
        surf.emission = UnpackEmission(surf.albedo, rome.w);
        surf.roughness = rome.x;
        surf.occlusion = rome.y;
        surf.metallic = rome.z;
    }

    return surf;
}

pim_inline rayhit_t VEC_CALL pt_intersect_local(
    const pt_scene_t *const pim_noalias scene,
    float4 ro,
    float4 rd,
    float tNear,
    float tFar)
{
    rayhit_t hit = { 0 };
    hit.wuvt.w = -1.0f;
    hit.index = -1;

    RTCRayHit rtcHit = RtcIntersect(scene->rtcScene, ro, rd, tNear, tFar);
    hit.normal = f4_v(rtcHit.hit.Ng_x, rtcHit.hit.Ng_y, rtcHit.hit.Ng_z, 0.0f);
    bool hitNothing =
        (rtcHit.hit.geomID == RTC_INVALID_GEOMETRY_ID) ||
        (rtcHit.ray.tfar <= 0.0f);
    if (hitNothing)
    {
        hit.type = hit_nothing;
        return hit;
    }
    hit.type = hit_triangle;
    if (f4_dot3(hit.normal, rd) > 0.0f)
    {
        hit.normal = f4_neg(hit.normal);
        hit.type = hit_backface;
    }
    hit.normal = f4_normalize3(hit.normal);

    ASSERT(rtcHit.hit.primID != RTC_INVALID_GEOMETRY_ID);
    i32 iVert = rtcHit.hit.primID * 3;
    ASSERT(iVert >= 0);
    ASSERT(iVert < scene->vertCount);
    float u = f1_sat(rtcHit.hit.u);
    float v = f1_sat(rtcHit.hit.v);
    float w = f1_sat(1.0f - (u + v));
    float t = rtcHit.ray.tfar;

    hit.index = iVert;
    hit.wuvt = f4_v(w, u, v, t);
    hit.flags = GetMaterial(scene, hit)->flags;

    return hit;
}

rayhit_t VEC_CALL pt_intersect(
    pt_scene_t *const pim_noalias scene,
    float4 ro,
    float4 rd,
    float tNear,
    float tFar)
{
    return pt_intersect_local(scene, ro, rd, tNear, tFar);
}

// returns attenuation value for the given interaction
pim_inline float4 VEC_CALL BrdfEval(
    pt_sampler_t*const pim_noalias sampler,
    float4 I,
    const surfhit_t* surf,
    float4 L)
{
    ASSERT(IsUnitLength(I));
    ASSERT(IsUnitLength(L));
    float4 N = surf->N;
    ASSERT(IsUnitLength(N));
    if (surf->flags & matflag_refractive)
    {
        return RefractBrdfEval(sampler, I, surf, L);
    }
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

    const float amtDiffuse = 1.0f - metallic;
    const float amtSpecular = 1.0f;

    float4 brdf = f4_0;
    float4 F = F_SchlickEx(albedo, metallic, HoV);
    {
        float D = D_GTR(NoH, alpha);
        float G = G_SmithGGX(NoL, NoV, alpha);
        float4 Fr = f4_mulvs(F, D * G);
        brdf = f4_add(brdf, Fr);
    }
    {
        float4 Fd = f4_mulvs(albedo, Fd_Burley(NoL, NoV, HoV, roughness));
        Fd = f4_mulvs(Fd, amtDiffuse);
        brdf = f4_add(brdf, Fd);
    }

    brdf = f4_mulvs(brdf, NoL);

    float diffusePdf = amtDiffuse * LambertPdf(NoL);
    float specularPdf = amtSpecular * GGXPdf(NoH, HoV, alpha);
    float pdf = diffusePdf + specularPdf;

    brdf.w = pdf;
    return brdf;
}

pim_inline float VEC_CALL Schlick(float cosTheta, float k)
{
    float r0 = (1.0f - k) / (1.0f + k);
    r0 = f1_sat(r0 * r0);
    float t = 1.0f - cosTheta;
    t = t * t * t * t * t;
    t = f1_sat(t);
    return f1_lerp(r0, 1.0f, t);
}

pim_inline float4 VEC_CALL RefractBrdfEval(
    pt_sampler_t *const pim_noalias sampler,
    float4 I,
    const surfhit_t* surf,
    float4 L)
{
    return f4_0;
}

pim_inline scatter_t VEC_CALL RefractScatter(
    pt_sampler_t*const pim_noalias sampler,
    const surfhit_t* surf,
    float4 I)
{
    const float kAir = 1.000277f;
    const float matIor = f1_max(1.0f, surf->ior);
    const float alpha = BrdfAlpha(surf->roughness);

    float4 V = f4_neg(I);
    float4 M = surf->M;
    float4 m = TanToWorld(surf->N, SampleGGXMicrofacet(Sample2D(sampler), alpha));
    m = f4_dot3(m, M) > 0.0f ? m : f4_reflect3(m, M);
    m = f4_normalize3(m);
    bool entering = surf->type != hit_backface;
    float k = entering ? (kAir / matIor) : (matIor / kAir);

    float cosTheta = f1_min(1.0f, f4_dot3(V, m));
    float sinTheta = sqrtf(1.0f - cosTheta * cosTheta);
    float pdf = Schlick(cosTheta, k);
    float4 L = f4_normalize3(f4_reflect3(I, m));
    bool tir = (k * sinTheta) > 1.0f;
    pdf = tir ? 1.0f : pdf;

    float4 P = surf->P;
    float4 F = F_SchlickEx(surf->albedo, surf->metallic, f1_sat(cosTheta));
    if (BrdfPrng(sampler) > pdf)
    {
        L = f4_normalize3(f4_refract3(I, m, k));
        pdf = 1.0f - pdf;
        F = f4_inv(F);
        P = f4_sub(P, f4_mulvs(M, 0.02f * kMilli));
    }

    scatter_t result = { 0 };
    result.pos = P;
    result.dir = L;
    result.attenuation = tir ? f4_1 : F;
    result.pdf = pdf;

    return result;
}

pim_inline scatter_t VEC_CALL BrdfScatter(
    pt_sampler_t *const pim_noalias sampler,
    const surfhit_t* surf,
    float4 I)
{
    ASSERT(IsUnitLength(I));
    ASSERT(IsUnitLength(surf->N));
    if (surf->flags & matflag_refractive)
    {
        return RefractScatter(sampler, surf, I);
    }

    scatter_t result = { 0 };
    result.pos = surf->P;

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
    {
        float2 Xi = Sample2D(sampler);
        float3x3 TBN = NormalToTBN(N);
        if (BrdfPrng(sampler) < rcpProb)
        {
            float4 m = TbnToWorld(TBN, SampleGGXMicrofacet(Xi, alpha));
            L = f4_reflect3(I, m);
        }
        else
        {
            L = TbnToWorld(TBN, SampleCosineHemisphere(Xi));
        }
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
    float LoH = f4_dotsat(L, H);

    float specularPdf = amtSpecular * GGXPdf(NoH, HoV, alpha);
    float diffusePdf = amtDiffuse * LambertPdf(NoL);
    float pdf = diffusePdf + specularPdf;

    if (pdf <= 0.0f)
    {
        return result;
    }

    float4 brdf = f4_0;
    float4 F = F_SchlickEx(albedo, metallic, HoV);
    {
        float D = D_GTR(NoH, alpha);
        float G = G_SmithGGX(NoL, NoV, alpha);
        float4 Fr = f4_mulvs(F, D * G);
        brdf = f4_add(brdf, Fr);
    }
    {
        float4 Fd = f4_mulvs(albedo, Fd_Burley(NoL, NoV, LoH, roughness));
        Fd = f4_mulvs(Fd, amtDiffuse);
        brdf = f4_add(brdf, Fd);
    }

    result.dir = L;
    result.pdf = pdf;
    result.attenuation = f4_mulvs(brdf, NoL);

    return result;
}

pim_inline void VEC_CALL LightOnHit(
    pt_sampler_t*const pim_noalias sampler,
    pt_scene_t*const pim_noalias scene,
    float4 ro,
    float4 lum4,
    i32 iVert)
{
    i32 iList = scene->vertToEmit[iVert];
    if (iList >= 0)
    {
        i32 iCell = grid_index(&scene->lightGrid, ro);
        dist1d_t* dist = &scene->lightDists[iCell];
        if (dist->length)
        {
            u32 amt = (u32)(f4_avglum(lum4) * 64.0f + 0.5f);
            fetch_add_u32(dist->live + iList, amt, MO_Relaxed);
        }
    }
}

pim_inline bool VEC_CALL LightSelect(
    pt_sampler_t*const pim_noalias sampler,
    pt_scene_t*const pim_noalias scene,
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
    if (!dist->length)
    {
        return false;
    }

    i32 iList = dist1d_sampled(dist, LightSelectPrng(sampler));
    float pdf = dist1d_pdfd(dist, iList);

    i32 iVert = scene->emissives[iList];

    *iVertOut = iVert;
    *pdfOut = pdf;
    return pdf > kEpsilon;
}

pim_inline float VEC_CALL LightSelectPdf(
    pt_scene_t*const pim_noalias scene,
    i32 iVert,
    float4 ro)
{
    float selectPdf = 1.0f;
    i32 iCell = grid_index(&scene->lightGrid, ro);
    i32 iList = scene->vertToEmit[iVert];
    if (iList >= 0)
    {
        const dist1d_t* dist = scene->lightDists + iCell;
        if (dist->length)
        {
            selectPdf = dist1d_pdfd(dist, iList);
        }
    }
    return selectPdf;
}

pim_inline lightsample_t VEC_CALL LightSample(
    pt_sampler_t *const pim_noalias sampler,
    pt_scene_t *const pim_noalias scene,
    float4 ro,
    i32 iVert,
    i32 bounce)
{
    lightsample_t sample = { 0 };

    float4 wuv = SampleBaryCoord(Sample2D(sampler));

    const float4* pim_noalias positions = scene->positions;
    float4 A = positions[iVert + 0];
    float4 B = positions[iVert + 1];
    float4 C = positions[iVert + 2];
    float4 pt = f4_blend(A, B, C, wuv);
    float area = TriArea3D(A, B, C);

    float4 rd = f4_sub(pt, ro);
    float distSq = f4_dot3(rd, rd);
    float distance = sqrtf(distSq);
    wuv.w = distance;
    rd = f4_divvs(rd, distance);

    sample.direction = rd;
    sample.wuvt = wuv;

    rayhit_t hit = pt_intersect_local(scene, ro, rd, 0.0f, distance + 0.01f * kMilli);
    if ((hit.type != hit_nothing) && (hit.index == iVert))
    {
        float cosTheta = f1_abs(f4_dot3(rd, hit.normal));
        sample.pdf = LightPdf(area, cosTheta, distSq);
        sample.luminance = GetEmission(scene, ro, rd, hit, bounce);
        if (f4_hmax3(sample.luminance) > kEpsilon)
        {
            float4 Tr = CalcTransmittance(sampler, scene, ro, rd, hit.wuvt.w);
            sample.luminance = f4_mul(sample.luminance, Tr);
        }
    }

    return sample;
}

pim_inline float VEC_CALL LightEvalPdf(
    pt_sampler_t *const pim_noalias sampler,
    pt_scene_t *const pim_noalias scene,
    float4 ro,
    float4 rd,
    rayhit_t *const pim_noalias hitOut)
{
    ASSERT(IsUnitLength(rd));
    rayhit_t hit = pt_intersect_local(scene, ro, rd, 0.0f, 1 << 20);
    float pdf = 0.0f;
    if (hit.type != hit_nothing)
    {
        float cosTheta = f1_abs(f4_dot3(rd, hit.normal));
        float area = GetArea(scene, hit.index);
        float distSq = f1_max(kEpsilon, hit.wuvt.w * hit.wuvt.w);
        pdf = LightPdf(area, cosTheta, distSq);
    }
    *hitOut = hit;
    return pdf;
}

pim_inline float4 VEC_CALL EstimateDirect(
    pt_sampler_t *const pim_noalias sampler,
    pt_scene_t *const pim_noalias scene,
    surfhit_t const *const pim_noalias surf,
    rayhit_t const *const pim_noalias srcHit,
    float4 I,
    i32 bounce)
{
    float4 result = f4_0;
    if (surf->flags & matflag_refractive)
    {
        return result;
    }

    const float4 ro = surf->P;
    const float pRough = f1_lerp(0.05f, 0.95f, surf->roughness);
    const float pSmooth = 1.0f - pRough;
    if (Sample1D(sampler) < pRough)
    {
        i32 iVert;
        float selectPdf;
        if (LightSelect(sampler, scene, surf->P, &iVert, &selectPdf))
        {
            if (srcHit->index != iVert)
            {
                // already has CalcTransmittance applied
                lightsample_t sample = LightSample(sampler, scene, ro, iVert, bounce);
                float4 rd = sample.direction;
                float4 Li = sample.luminance;
                float lightPdf = sample.pdf * selectPdf * pRough;
                if ((lightPdf > kEpsilon) && (f4_hmax3(Li) > kEpsilon))
                {
                    float4 brdf = BrdfEval(sampler, I, surf, rd);
                    Li = f4_mul(Li, brdf);
                    float brdfPdf = brdf.w * pSmooth;
                    if (brdfPdf > kEpsilon)
                    {
                        Li = f4_mulvs(Li, PowerHeuristic(lightPdf, brdfPdf) * 0.5f / lightPdf);
                        result = f4_add(result, Li);
                    }
                }
            }
        }
    }
    else
    {
        scatter_t sample = BrdfScatter(sampler, surf, I);
        float4 rd = sample.dir;
        float brdfPdf = sample.pdf * pSmooth;
        if (brdfPdf > kEpsilon)
        {
            rayhit_t hit;
            float lightPdf = LightEvalPdf(sampler, scene, ro, rd, &hit) * pRough;
            if (lightPdf > kEpsilon)
            {
                lightPdf *= LightSelectPdf(scene, hit.index, ro);
                float4 Li = f4_mul(GetEmission(scene, ro, rd, hit, bounce), sample.attenuation);
                if (f4_hmax3(Li) > kEpsilon)
                {
                    Li = f4_mulvs(Li, PowerHeuristic(brdfPdf, lightPdf) * 0.5f / brdfPdf);
                    Li = f4_mul(Li, CalcTransmittance(sampler, scene, ro, rd, hit.wuvt.w));
                    result = f4_add(result, Li);
                }
            }
        }
    }

    return result;
}

pim_inline bool VEC_CALL EvaluateLight(
    pt_sampler_t*const pim_noalias sampler,
    pt_scene_t*const pim_noalias scene,
    float4 P,
    float4 *const pim_noalias lightOut,
    float4 *const pim_noalias dirOut,
    i32 bounce)
{
    i32 iVert;
    float selectPdf;
    if (LightSelect(sampler, scene, P, &iVert, &selectPdf))
    {
        lightsample_t sample = LightSample(sampler, scene, P, iVert, bounce);
        if (sample.pdf > kEpsilon)
        {
            *lightOut = f4_divvs(sample.luminance, sample.pdf * selectPdf);
            *dirOut = sample.direction;
            return true;
        }
    }
    return false;
}

static void media_desc_new(media_desc_t *const desc)
{
    desc->constantAlbedo = f4_v(0.75f, 0.75f, 0.75f, 0.5f);
    desc->noiseAlbedo = f4_v(0.75f, 0.75f, 0.75f, -0.5f);
    desc->absorption = 0.1f;
    desc->constantAmt = exp2f(-20.0f);
    desc->noiseAmt = exp2f(-20.0f);
    desc->noiseOctaves = 2;
    desc->noiseGain = 0.5f;
    desc->noiseLacunarity = 2.0666f;
    desc->noiseFreq = 1.0f;
    desc->noiseHeight = 20.0f;
    desc->noiseScale = exp2f(-5.0f);
    desc->phaseDirA = 0.5f;
    desc->phaseDirB = -0.5f;
    desc->phaseBlend = 0.5f;
    media_desc_update(desc);
}

static void media_desc_update(media_desc_t *const desc)
{
    desc->logConstantAlbedo = AlbedoToLogAlbedo(desc->constantAlbedo);
    desc->logNoiseAlbedo = AlbedoToLogAlbedo(desc->noiseAlbedo);
    desc->phaseDirA = f1_clamp(desc->phaseDirA, -0.99f, 0.99f);
    desc->phaseDirB = f1_clamp(desc->phaseDirB, -0.99f, 0.99f);
    desc->phaseBlend = f1_sat(desc->phaseBlend);

    const i32 noiseOctaves = desc->noiseOctaves;
    const float noiseGain = desc->noiseGain;
    const float noiseScale = desc->noiseScale;

    float sum = 0.0f;
    float amplitude = 1.0f;
    for (i32 i = 0; i < noiseOctaves; ++i)
    {
        sum += amplitude;
        amplitude *= noiseGain;
    }

    desc->noiseRange = sum * noiseScale * 1.5f;

    desc->rcpMajorant = 1.0f / f4_hmax3(CalcMajorant(desc));
}

static void media_desc_load(media_desc_t *const desc, const char* name)
{
    if (!desc || !name)
    {
        return;
    }
    memset(desc, 0, sizeof(*desc));
    media_desc_new(desc);
    char filename[PIM_PATH];
    SPrintf(ARGS(filename), "%s.json", name);
    ser_obj_t* root = ser_fromfile(filename);
    if (root)
    {
        ser_load_f4(desc, root, constantAlbedo);
        ser_load_f4(desc, root, noiseAlbedo);
        ser_load_f32(desc, root, absorption);
        ser_load_f32(desc, root, constantAmt);
        ser_load_f32(desc, root, noiseAmt);
        ser_load_i32(desc, root, noiseOctaves);
        ser_load_f32(desc, root, noiseGain);
        ser_load_f32(desc, root, noiseLacunarity);
        ser_load_f32(desc, root, noiseFreq);
        ser_load_f32(desc, root, noiseScale);
        ser_load_f32(desc, root, noiseHeight);
        ser_load_f32(desc, root, phaseDirA);
        ser_load_f32(desc, root, phaseDirB);
        ser_load_f32(desc, root, phaseBlend);
    }
    else
    {
        con_logf(LogSev_Error, "pt", "Failed to load media desc '%s'", filename);
        media_desc_new(desc);
    }
    ser_obj_del(root);
}

static void media_desc_save(media_desc_t const *const desc, const char* name)
{
    if (!desc || !name)
    {
        return;
    }
    char filename[PIM_PATH];
    SPrintf(ARGS(filename), "%s.json", name);
    ser_obj_t* root = ser_obj_dict();
    if (root)
    {
        ser_save_f4(desc, root, constantAlbedo);
        ser_save_f4(desc, root, noiseAlbedo);
        ser_save_f32(desc, root, absorption);
        ser_save_f32(desc, root, constantAmt);
        ser_save_f32(desc, root, noiseAmt);
        ser_save_i32(desc, root, noiseOctaves);
        ser_save_f32(desc, root, noiseGain);
        ser_save_f32(desc, root, noiseLacunarity);
        ser_save_f32(desc, root, noiseFreq);
        ser_save_f32(desc, root, noiseScale);
        ser_save_f32(desc, root, noiseHeight);
        ser_save_f32(desc, root, phaseDirA);
        ser_save_f32(desc, root, phaseDirB);
        ser_save_f32(desc, root, phaseBlend);
        if (!ser_tofile(filename, root))
        {
            con_logf(LogSev_Error, "pt", "Failed to save media desc '%s'", filename);
        }
        ser_obj_del(root);
    }
}

static void igLog2SliderFloat(const char* label, float* pLinear, float log2Lo, float log2Hi)
{
    float asLog2 = log2f(*pLinear);
    igSliderFloat(label, &asLog2, log2Lo, log2Hi);
    *pLinear = exp2f(asLog2);
}

static char gui_mediadesc_name[PIM_PATH];

static void media_desc_gui(media_desc_t *const desc)
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
        igSliderFloat("Phase Dir A", &desc->phaseDirA, -0.99f, 0.99f);
        igSliderFloat("Phase Dir B", &desc->phaseDirB, -0.99f, 0.99f);
        igSliderFloat("Phase Blend", &desc->phaseBlend, 0.0f, 1.0f);
        igLog2SliderFloat("Log2 Absorption", &desc->absorption, -20.0f, 5.0f);
        igColorEdit3("Constant Albedo", &desc->constantAlbedo.x, ldrPicker);
        igLog2SliderFloat("Log2 Constant Amount", &desc->constantAmt, -20.0f, 5.0f);
        igColorEdit3("Noise Albedo", &desc->noiseAlbedo.x, ldrPicker);
        igLog2SliderFloat("Log2 Noise Amount", &desc->noiseAmt, -20.0f, 5.0f);
        igSliderInt("Noise Octaves", &desc->noiseOctaves, 1, 10, "%d");
        igSliderFloat("Noise Gain", &desc->noiseGain, 0.0f, 1.0f);
        igSliderFloat("Noise Lacunarity", &desc->noiseLacunarity, 1.0f, 3.0f);
        igLog2SliderFloat("Log2 Noise Frequency", &desc->noiseFreq, -5.0f, 5.0f);
        igSliderFloat("Noise Height", &desc->noiseHeight, -20.0f, 20.0f);
        igLog2SliderFloat("Log2 Noise Scale", &desc->noiseScale, -5.0f, 5.0f);
        igUnindent(0.0f);

        media_desc_update(desc);
    }
}

pim_inline float2 VEC_CALL Media_Density(media_desc_t const *const desc, float4 P)
{
    float heightFog = 0.0f;
    if (f1_distance(P.y, desc->noiseHeight) <= desc->noiseRange)
    {
        float noiseFreq = desc->noiseFreq;
        float noise = stb_perlinf_fbm_noise3(
            P.x * noiseFreq,
            P.y * noiseFreq,
            P.z * noiseFreq,
            desc->noiseLacunarity,
            desc->noiseGain,
            desc->noiseOctaves);
        float noiseScale = desc->noiseScale;
        float height = desc->noiseHeight + noiseScale * noise;
        float dist = f1_distance(P.y, height) / noiseScale;
        float heightDensity = f1_sat(1.0f - dist);
        heightFog = desc->noiseAmt * heightDensity;
    }

    float totalFog = desc->constantAmt + heightFog;
    float t = heightFog / totalFog;
    return f2_v(totalFog, t);
}

pim_inline media_t VEC_CALL Media_Sample(media_desc_t const *const desc, float4 P)
{
    float2 density = Media_Density(desc, P);
    float totalScattering = density.x;
    float totalExtinction = totalScattering * (1.0f + desc->absorption);
    float4 logAlbedo = f4_lerpvs(desc->logConstantAlbedo, desc->logNoiseAlbedo, density.y);

    media_t result;
    result.logAlbedo = logAlbedo;
    result.scattering = totalScattering;
    result.extinction = totalExtinction;

    return result;
}

pim_inline float4 VEC_CALL AlbedoToLogAlbedo(float4 albedo)
{
    // https://www.desmos.com/calculator/rqrl5xhtea
    const float a = 0.3679f;
    const float b = 0.6f;
    float4 x = f4_addvs(f4_mulvs(albedo, b), a);
    float4 y = f4_neg(f4_log(x));
    return y;
}

pim_inline float4 VEC_CALL Media_Extinction(media_t media)
{
    return f4_mulvs(media.logAlbedo, media.extinction);
}

pim_inline float VEC_CALL Media_Scattering(media_t media)
{
    return f1_lerp(0.5f, 1.0f, f4_hmax3(media.logAlbedo)) * media.scattering;
}

pim_inline float4 VEC_CALL Media_Albedo(media_desc_t const *const desc, float4 P)
{
    return f4_inv(Media_Sample(desc, P).logAlbedo);
}

pim_inline float4 VEC_CALL Media_Normal(media_desc_t const *const desc, float4 P)
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
    result.logAlbedo = f4_lerpvs(lhs.logAlbedo, rhs.logAlbedo, t);
    result.scattering = f1_lerp(lhs.scattering, rhs.scattering, t);
    result.extinction = f1_lerp(lhs.extinction, rhs.extinction, t);
    return result;
}

// samples a free path in a given media
// Xi: random variable in [0, 1)
// u: extinction coefficient, or majorant in woodcock sampling
pim_inline float VEC_CALL SampleFreePath(float Xi, float rcpU)
{
    return -logf(1.0f - Xi) * rcpU;
}

// returns probability of a free path at t
// t: ray time
// u: extinction coefficient, or majorant in woodcock sampling
pim_inline float VEC_CALL FreePathPdf(float t, float u)
{
    return u * expf(-t * u);
}

// maximum extinction coefficient of the media
pim_inline float4 VEC_CALL CalcMajorant(media_desc_t const *const desc)
{
    i32 octaves = desc->noiseOctaves;
    float gain = desc->noiseGain;
    float sum = 0.0f;
    float amplitude = 1.0f;
    for (i32 i = 0; i < octaves; ++i)
    {
        sum += amplitude;
        amplitude *= gain;
    }
    float a = 1.0f + desc->absorption;
    float4 ca = f4_mulvs(desc->logConstantAlbedo, desc->constantAmt * a);
    float4 na = f4_mulvs(desc->logNoiseAlbedo, desc->noiseAmt * a * sum);
    return f4_add(ca, na);
}

pim_inline float4 VEC_CALL CalcControl(media_desc_t const *const desc)
{
    i32 octaves = desc->noiseOctaves;
    float gain = desc->noiseGain;
    float sum = 0.0f;
    float amplitude = 1.0f;
    for (i32 i = 0; i < octaves; ++i)
    {
        sum += amplitude;
        amplitude *= gain;
    }
    float a = 1.0f + desc->absorption;
    float4 ca = f4_mulvs(desc->logConstantAlbedo, desc->constantAmt * a);
    float4 na = f4_mulvs(desc->logNoiseAlbedo, desc->noiseAmt * a * sum * 0.5f);
    return f4_add(ca, na);
}

pim_inline float4 VEC_CALL CalcResidual(float4 control, float4 majorant)
{
    return f4_abs(f4_sub(majorant, control));
}

pim_inline float VEC_CALL CalcPhase(
    media_desc_t const *const desc,
    float cosTheta)
{
    return f1_lerp(MiePhase(cosTheta, desc->phaseDirA), MiePhase(cosTheta, desc->phaseDirB), desc->phaseBlend);
}

pim_inline float4 VEC_CALL SamplePhaseDir(
    pt_sampler_t*const pim_noalias sampler,
    media_desc_t const *const pim_noalias desc,
    float4 rd)
{
    float4 L;
    do
    {
        L = SampleUnitSphere(Sample2D(sampler));
        L.w = CalcPhase(desc, f4_dot3(rd, L));
    } while (PhaseDirPrng(sampler) > L.w);
    return L;
}

pim_inline float4 VEC_CALL CalcTransmittance(
    pt_sampler_t *const pim_noalias sampler,
    const pt_scene_t *const pim_noalias scene,
    float4 ro,
    float4 rd,
    float rayLen)
{
    media_desc_t const *const pim_noalias desc = &scene->mediaDesc;
    const float rcpU = desc->rcpMajorant;
    float t = 0.0f;
    float4 attenuation = f4_1;
    while (true)
    {
        float4 P = f4_add(ro, f4_mulvs(rd, t));
        float dt = SampleFreePath(Sample1D(sampler), rcpU);
        t += dt;
        if (t >= rayLen)
        {
            break;
        }
        media_t media = Media_Sample(desc, P);
        float4 uT = Media_Extinction(media);
        float4 ratio = f4_inv(f4_mulvs(uT, rcpU));
        attenuation = f4_mul(attenuation, ratio);
    }
    return attenuation;
}

pim_inline scatter_t VEC_CALL ScatterRay(
    pt_sampler_t *const pim_noalias sampler,
    pt_scene_t *const pim_noalias scene,
    float4 ro,
    float4 rd,
    float rayLen,
    i32 bounce)
{
    scatter_t result = { 0 };
    result.pdf = 0.0f;

    media_desc_t const *const pim_noalias desc = &scene->mediaDesc;
    const float rcpU = desc->rcpMajorant;

    float t = 0.0f;
    result.attenuation = f4_1;
    while (true)
    {
        float4 P = f4_add(ro, f4_mulvs(rd, t));
        float dt = SampleFreePath(Sample1D(sampler), rcpU);
        t += dt;
        if (t >= rayLen)
        {
            break;
        }

        media_t media = Media_Sample(desc, P);

        float uS = Media_Scattering(media);
        bool scattered = ScatterPrng(sampler) < (uS * rcpU);
        if (scattered)
        {
            float4 lum;
            float4 L;
            if (EvaluateLight(sampler, scene, P, &lum, &L, bounce))
            {
                float ph = CalcPhase(desc, f4_dot3(rd, L));
                lum = f4_mulvs(lum, ph * dt);
                lum = f4_mul(lum, result.attenuation);
                result.luminance = lum;
            }

            result.pos = f4_add(ro, f4_mulvs(rd, t));
            result.dir = SamplePhaseDir(sampler, desc, rd);
            result.pdf = result.dir.w;
            float ph = CalcPhase(desc, f4_dot3(rd, result.dir));
            result.attenuation = f4_mulvs(result.attenuation, ph);
        }

        float4 uT = Media_Extinction(media);
        float4 ratio = f4_inv(f4_mulvs(uT, rcpU));
        result.attenuation = f4_mul(result.attenuation, ratio);

        if (scattered)
        {
            break;
        }
    }

    return result;
}

pt_result_t VEC_CALL pt_trace_ray(
    pt_sampler_t *const pim_noalias sampler,
    pt_scene_t *const pim_noalias scene,
    float4 ro,
    float4 rd)
{
    pt_result_t result = { 0 };
    float4 luminance = f4_0;
    float4 attenuation = f4_1;
    u32 prevFlags = 0;

    for (i32 b = 0; b < 666; ++b)
    {
        {
            float p = f1_sat(f4_avglum(attenuation));
            if (RoulettePrng(sampler) < p)
            {
                attenuation = f4_divvs(attenuation, p);
            }
            else
            {
                break;
            }
        }

        rayhit_t hit = pt_intersect_local(scene, ro, rd, 0.0f, 1 << 20);
        if (hit.type == hit_nothing)
        {
            break;
        }
        //if (hit.type == hit_backface)
        //{
        //    if (!(hit.flags & matflag_refractive))
        //    {
        //        break;
        //    }
        //}

        surfhit_t surf = GetSurface(scene, ro, rd, hit, b);
        if (b > 0)
        {
            LightOnHit(sampler, scene, ro, surf.emission, hit.index);
        }

        {
            scatter_t scatter = ScatterRay(sampler, scene, ro, rd, hit.wuvt.w, b);
            luminance = f4_add(luminance, f4_mul(scatter.luminance, attenuation));
            if (scatter.pdf > kEpsilon)
            {
                if (b == 0)
                {
                    result.albedo = f4_f3(Media_Albedo(&scene->mediaDesc, scatter.pos));
                    result.normal = f4_f3(f4_neg(rd));
                }
                attenuation = f4_mul(attenuation, f4_divvs(scatter.attenuation, scatter.pdf));
                ro = scatter.pos;
                rd = scatter.dir;
                continue;
            }
            else
            {
                attenuation = f4_mul(attenuation, scatter.attenuation);
            }
        }

        if (b == 0)
        {
            float t = f4_avglum(attenuation);
            float4 N = f4_lerpvs(f4_neg(rd), surf.N, t);
            result.albedo = f4_f3(surf.albedo);
            result.normal = f4_f3(N);
        }
        if ((b == 0) || (prevFlags & matflag_refractive))
        {
            luminance = f4_add(luminance, f4_mul(surf.emission, attenuation));
        }
        if (hit.flags & matflag_sky)
        {
            break;
        }

        {
            float4 Li = EstimateDirect(sampler, scene, &surf, &hit, rd, b);
            luminance = f4_add(luminance, f4_mul(Li, attenuation));
        }

        scatter_t scatter = BrdfScatter(sampler, &surf, rd);
        if (scatter.pdf < kEpsilon)
        {
            break;
        }
        ro = scatter.pos;
        rd = scatter.dir;

        attenuation = f4_mul(attenuation, f4_divvs(scatter.attenuation, scatter.pdf));
        prevFlags = surf.flags;
    }

    result.color = f4_f3(luminance);
    return result;
}

pim_inline float2 VEC_CALL SampleUv(
    pt_sampler_t*const pim_noalias sampler,
    dist1d_t const *const pim_noalias dist)
{
    float2 Xi = UvPrng(sampler);
    float angle = Xi.x * kTau;
    float radius = dist1d_samplec(dist, Xi.y) * 2.0f;
    Xi.x = cosf(angle);
    Xi.y = sinf(angle);
    return f2_mulvs(Xi, radius);
}

pim_inline ray_t VEC_CALL CalculateDof(
    pt_sampler_t*const pim_noalias sampler,
    const dofinfo_t* dof,
    float4 right,
    float4 up,
    float4 fwd,
    ray_t ray)
{
    float2 offset;
    {
        i32 side = prng_i32(&sampler->rng);
        float2 Xi = DofPrng(sampler);
        if (dof->bladeCount == 666)
        {
            offset = SamplePentagram(Xi, side);
        }
        else
        {
            offset = SampleNGon(
                Xi, side,
                dof->bladeCount,
                dof->bladeRot);
        }
    }
    offset = f2_mulvs(offset, dof->aperture);
    float t = f1_lerp(
        dof->focalLength / f4_dot3(ray.rd, fwd),
        dof->focalLength,
        dof->focalPlaneCurvature);
    float4 focusPos = f4_add(ray.ro, f4_mulvs(ray.rd, t));
    float4 aperturePos = f4_add(ray.ro, f4_add(f4_mulvs(right, offset.x), f4_mulvs(up, offset.y)));
    ray.ro = aperturePos;
    ray.rd = f4_normalize3(f4_sub(focusPos, aperturePos));
    return ray;
}

typedef struct TaskUpdateDists
{
    task_t task;
    pt_scene_t* scene;
    float alpha;
    u32 minSamples;
} TaskUpdateDists;

static void UpdateDistsFn(void* pbase, i32 begin, i32 end)
{
    TaskUpdateDists*const pim_noalias task = pbase;
    pt_scene_t*const pim_noalias scene = task->scene;
    dist1d_t*const pim_noalias dists = scene->lightDists;
    float alpha = task->alpha;
    u32 minSamples = task->minSamples;
    for (i32 i = begin; i < end; ++i)
    {
        dist1d_livebake(&dists[i], alpha, minSamples);
    }
}

static void UpdateDists(pt_scene_t*const pim_noalias scene)
{
    TaskUpdateDists*const pim_noalias task = tmp_calloc(sizeof(*task));
    task->scene = scene;
    task->alpha = cvar_get_float(&cv_pt_dist_alpha);
    task->minSamples = cvar_get_int(&cv_pt_dist_samples);
    i32 worklen = grid_len(&scene->lightGrid);
    task_run(task, UpdateDistsFn, worklen);
}

typedef struct trace_task_s
{
    task_t task;
    pt_trace_t* trace;
    camera_t camera;
} trace_task_t;

static void TraceFn(void* pbase, i32 begin, i32 end)
{
    trace_task_t *const pim_noalias task = pbase;

    pt_trace_t *const pim_noalias trace = task->trace;
    const camera_t camera = task->camera;

    pt_scene_t *const pim_noalias scene = trace->scene;
    float3 *const pim_noalias color = trace->color;
    float3 *const pim_noalias albedo = trace->albedo;
    float3 *const pim_noalias normal = trace->normal;

    const int2 size = trace->imageSize;
    const float2 rcpSize = f2_rcp(i2_f2(size));
    const float sampleWeight = trace->sampleWeight;

    const quat rot = camera.rotation;
    const float4 eye = camera.position;
    const float4 right = quat_right(rot);
    const float4 up = quat_up(rot);
    const float4 fwd = quat_fwd(rot);
    const float2 slope = proj_slope(f1_radians(camera.fovy), (float)size.x / (float)size.y);
    const dofinfo_t dof = trace->dofinfo;
    const dist1d_t dist = ms_pixeldist;

    pt_sampler_t sampler = GetSampler();
    for (i32 i = begin; i < end; ++i)
    {
        int2 coord = { i % size.x, i / size.x };

        // gaussian AA filter
        float2 uv = { (coord.x + 0.5f), (coord.y + 0.5f) };
        float2 Xi = SampleUv(&sampler, &dist);
        uv = f2_snorm(f2_mul(f2_add(uv, Xi), rcpSize));

        ray_t ray = { eye, proj_dir(right, up, fwd, slope, uv) };
        ray = CalculateDof(&sampler, &dof, right, up, fwd, ray);

        pt_result_t result = pt_trace_ray(&sampler, scene, ray.ro, ray.rd);
        color[i] = f3_lerp(color[i], result.color, sampleWeight);
        albedo[i] = f3_lerp(albedo[i], result.albedo, sampleWeight);
        normal[i] = f3_lerp(normal[i], result.normal, sampleWeight);
    }
    SetSampler(sampler);
}

ProfileMark(pm_trace, pt_trace)
void pt_trace(pt_trace_t* desc, const camera_t* camera)
{
    ProfileBegin(pm_trace);

    ASSERT(desc);
    ASSERT(desc->scene);
    ASSERT(camera);
    ASSERT(desc->color);
    ASSERT(desc->albedo);
    ASSERT(desc->normal);

    pt_scene_update(desc->scene);

    trace_task_t*const pim_noalias task = tmp_calloc(sizeof(*task));
    task->trace = desc;
    task->camera = *camera;

    const i32 workSize = desc->imageSize.x * desc->imageSize.y;
    task_run(&task->task, TraceFn, workSize);

    ProfileEnd(pm_trace);
}

typedef struct pt_raygen_s
{
    task_t task;
    pt_scene_t* scene;
    float4 origin;
    float4* colors;
    float4* directions;
} pt_raygen_t;

static void RayGenFn(void* pBase, i32 begin, i32 end)
{
    pt_raygen_t*const pim_noalias task = pBase;
    pt_scene_t*const pim_noalias scene = task->scene;
    const float4 ro = task->origin;

    float4*const pim_noalias colors = task->colors;
    float4*const pim_noalias directions = task->directions;

    pt_sampler_t sampler = GetSampler();
    for (i32 i = begin; i < end; ++i)
    {
        float4 rd = SampleUnitSphere(Sample2D(&sampler));
        directions[i] = rd;
        pt_result_t result = pt_trace_ray(&sampler, scene, ro, rd);
        colors[i] = f3_f4(result.color, 1.0f);
    }
    SetSampler(sampler);
}

ProfileMark(pm_raygen, pt_raygen)
pt_results_t pt_raygen(
    pt_scene_t*const pim_noalias scene,
    float4 origin,
    i32 count)
{
    ProfileBegin(pm_raygen);

    ASSERT(scene);
    ASSERT(count >= 0);

    pt_scene_update(scene);

    pt_raygen_t*const pim_noalias task = tmp_calloc(sizeof(*task));
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

pim_inline float VEC_CALL Sample1D(pt_sampler_t*const pim_noalias sampler)
{
    return prng_f32(&sampler->rng);
}

pim_inline float2 VEC_CALL Sample2D(pt_sampler_t*const pim_noalias sampler)
{
    return f2_rand(&sampler->rng);
}

pim_inline pt_sampler_t VEC_CALL GetSampler(void)
{
    i32 tid = task_thread_id();
    return ms_samplers[tid];
}

pim_inline void VEC_CALL SetSampler(pt_sampler_t sampler)
{
    i32 tid = task_thread_id();
    ms_samplers[tid] = sampler;
}
