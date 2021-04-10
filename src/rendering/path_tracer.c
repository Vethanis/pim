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
#include "common/cvars.h"
#include "common/stringutil.h"
#include "common/serialize.h"
#include "common/atomics.h"
#include "common/time.h"
#include "ui/cimgui_ext.h"

#include "stb/stb_perlin_fork.h"
#include <string.h>

// ----------------------------------------------------------------------------

typedef struct SurfHit_s
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
    MatFlag flags;
    HitType type;
} SurfHit;

typedef struct Scatter_s
{
    float4 pos;
    float4 dir;
    float4 attenuation;
    float4 luminance;
    float pdf;
} Scatter;

typedef struct LightSample_s
{
    float4 direction;
    float4 luminance;
    float4 wuvt;
    float pdf;
} LightSample;

typedef struct MediaDesc_s
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
} MediaDesc;

typedef struct Media_s
{
    float4 logAlbedo;
    float scattering;
    float extinction;
} Media;

typedef struct PtScene_s
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
    i32* pim_noalias emitToVert;

    // portal triangle indices
    // [portalCount]
    i32* pim_noalias portals;

    // grid of discrete light distributions
    Grid lightGrid;
    // [lightGrid.size]
    Dist1D* pim_noalias lightDists;

    // surface description, indexed by matIds
    // [matCount]
    Material* pim_noalias materials;

    Cubemap* pim_noalias sky;

    // array lengths
    i32 vertCount;
    i32 matCount;
    i32 emissiveCount;
    i32 portalCount;
    // parameters
    MediaDesc mediaDesc;
    u64 modtime;
} PtScene;

// ----------------------------------------------------------------------------

static void PtScene_Init(PtScene* scene);
static void PtScene_Clear(PtScene* scene);
static void OnRtcError(void* user, RTCError error, const char* msg);
static bool InitRTC(void);
static void InitSamplers(void);
static void InitPixelDist(void);
static void ShutdownPixelDist(void);
static RTCScene RtcNewScene(const PtScene*const pim_noalias scene);
static void FlattenDrawables(PtScene*const pim_noalias scene);
static float EmissionPdf(
    PtSampler*const pim_noalias sampler,
    const PtScene*const pim_noalias scene,
    i32 iVert,
    i32 attempts);
static void CalcEmissionPdfFn(Task* pbase, i32 begin, i32 end);
static void SetupEmissives(PtScene*const pim_noalias scene);
static void SetupPortals(PtScene* scene);
static void SetupLightGridFn(Task* pbase, i32 begin, i32 end);
static void SetupLightGrid(PtScene*const pim_noalias scene);

static void media_desc_new(MediaDesc *const desc);
static void media_desc_update(MediaDesc *const desc);
static void media_desc_gui(MediaDesc *const desc);
static void media_desc_load(MediaDesc *const desc, const char* name);
static void media_desc_save(MediaDesc const *const desc, const char* name);

// ----------------------------------------------------------------------------

pim_inline RTCRay VEC_CALL RtcNewRay(float4 ro, float4 rd, float tNear, float tFar);
pim_inline RTCRayHit VEC_CALL RtcIntersect(
    RTCScene scene,
    float4 ro,
    float4 rd,
    float tNear,
    float tFar);
pim_inline float4 VEC_CALL GetPosition(
    const PtScene *const pim_noalias scene,
    RayHit hit);
pim_inline float4 VEC_CALL GetNormal(
    const PtScene *const pim_noalias scene,
    RayHit hit);
pim_inline float2 VEC_CALL GetUV(
    const PtScene *const pim_noalias scene,
    RayHit hit);
pim_inline float VEC_CALL GetArea(const PtScene*const pim_noalias scene, i32 iVert);
pim_inline Material const *const pim_noalias VEC_CALL GetMaterial(
    const PtScene*const pim_noalias scene,
    RayHit hit);
pim_inline float4 VEC_CALL GetSky(
    const PtScene*const pim_noalias scene,
    float4 ro,
    float4 rd,
    RayHit hit);
pim_inline float4 VEC_CALL GetEmission(
    const PtScene*const pim_noalias scene,
    float4 ro,
    float4 rd,
    RayHit hit,
    i32 bounce);
pim_inline SurfHit VEC_CALL GetSurface(
    const PtScene*const pim_noalias scene,
    float4 ro,
    float4 rd,
    RayHit hit,
    i32 bounce);
pim_inline RayHit VEC_CALL pt_intersect_local(
    const PtScene*const pim_noalias scene,
    float4 ro,
    float4 rd,
    float tNear,
    float tFar);

// ----------------------------------------------------------------------------

pim_inline float4 VEC_CALL RefractBrdfEval(
    PtSampler*const pim_noalias sampler,
    float4 I,
    const SurfHit* surf,
    float4 L);
pim_inline float4 VEC_CALL BrdfEval(
    PtSampler*const pim_noalias sampler,
    float4 I,
    const SurfHit* surf,
    float4 L);
pim_inline Scatter VEC_CALL RefractScatter(
    PtSampler*const pim_noalias sampler,
    const SurfHit* surf,
    float4 I);
pim_inline Scatter VEC_CALL BrdfScatter(
    PtSampler *const pim_noalias sampler,
    PtScene *const pim_noalias scene,
    const SurfHit* surf,
    float4 I);

// ----------------------------------------------------------------------------

pim_inline bool VEC_CALL LightSelect(
    PtSampler*const pim_noalias sampler,
    PtScene*const pim_noalias scene,
    float4 position,
    i32* iVertOut,
    float* pdfOut);
pim_inline LightSample VEC_CALL SampleLight(
    PtSampler *const pim_noalias sampler,
    PtScene *const pim_noalias scene,
    float4 ro,
    i32 iVert,
    i32 bounce);
pim_inline float VEC_CALL LightSelectPdf(
    PtScene *const pim_noalias scene,
    i32 iVert,
    float4 ro);
pim_inline float VEC_CALL LightEvalPdf(
    PtSampler *const pim_noalias sampler,
    PtScene *const pim_noalias scene,
    float4 ro,
    float4 rd,
    RayHit *const pim_noalias hitOut);

// ----------------------------------------------------------------------------

pim_inline float4 VEC_CALL EstimateDirect(
    PtSampler *const pim_noalias sampler,
    PtScene *const pim_noalias scene,
    SurfHit const *const pim_noalias surf,
    RayHit const *const pim_noalias srcHit,
    float4 I,
    i32 bounce);
pim_inline bool VEC_CALL EvaluateLight(
    PtSampler *const pim_noalias sampler,
    PtScene *const pim_noalias scene,
    float4 P,
    float4 *const pim_noalias lightOut,
    float4 *const pim_noalias dirOut,
    i32 bounce);

// ----------------------------------------------------------------------------

pim_inline float4 VEC_CALL AlbedoToLogAlbedo(float4 albedo);
pim_inline float2 VEC_CALL Media_Density(MediaDesc const *const desc, float4 P);
pim_inline Media VEC_CALL Media_Sample(MediaDesc const *const desc, float4 P);
pim_inline float4 VEC_CALL Media_Albedo(MediaDesc const *const desc, float4 P);
pim_inline float4 VEC_CALL Media_Normal(MediaDesc const *const desc, float4 P);
pim_inline Media VEC_CALL Media_Lerp(Media lhs, Media rhs, float t);
pim_inline float VEC_CALL SampleFreePath(float Xi, float rcpU);
pim_inline float VEC_CALL FreePathPdf(float t, float u);
pim_inline float4 VEC_CALL CalcMajorant(MediaDesc const *const desc);
pim_inline float4 VEC_CALL CalcControl(MediaDesc const *const desc);
pim_inline float4 VEC_CALL CalcResidual(float4 control, float4 majorant);
pim_inline float VEC_CALL CalcPhase(
    MediaDesc const *const desc,
    float cosTheta);
pim_inline float4 VEC_CALL SamplePhaseDir(
    PtSampler*const pim_noalias sampler,
    MediaDesc const *const pim_noalias desc,
    float4 rd);
pim_inline float4 VEC_CALL CalcTransmittance(
    PtSampler*const pim_noalias sampler,
    const PtScene*const pim_noalias scene,
    float4 ro,
    float4 rd,
    float rayLen);
pim_inline Scatter VEC_CALL ScatterRay(
    PtSampler*const pim_noalias sampler,
    PtScene*const pim_noalias scene,
    float4 ro,
    float4 rd,
    float rayLen,
    i32 bounce);

// ----------------------------------------------------------------------------

static void TraceFn(void* pbase, i32 begin, i32 end);
static void RayGenFn(void* pBase, i32 begin, i32 end);
pim_inline void VEC_CALL LightOnHit(
    PtSampler*const pim_noalias sampler,
    PtScene*const pim_noalias scene,
    float4 ro,
    float4 lum,
    i32 iVert);
static void UpdateDists(PtScene *const pim_noalias scene);
static void DofUpdate(PtTrace* trace, const Camera* camera);

// ----------------------------------------------------------------------------

pim_inline PtSampler VEC_CALL GetSampler(void);
pim_inline void VEC_CALL SetSampler(PtSampler sampler);

pim_inline float VEC_CALL Sample1D(PtSampler*const pim_noalias sampler);
pim_inline float2 VEC_CALL Sample2D(PtSampler*const pim_noalias sampler);

pim_inline float VEC_CALL RoulettePrng(PtSampler *const pim_noalias sampler)
{
    float Xi = f1_wrap(sampler->Xi[0] + kGoldenConj);
    sampler->Xi[0] = Xi;
    return Xi;
}
pim_inline float VEC_CALL LightSelectPrng(PtSampler *const pim_noalias sampler)
{
    float Xi = f1_wrap(sampler->Xi[1] + kSqrt2Conj);
    sampler->Xi[1] = Xi;
    return Xi;
}
pim_inline float VEC_CALL BrdfPrng(PtSampler*const pim_noalias sampler)
{
    float Xi = f1_wrap(sampler->Xi[2] + kSqrt3Conj);
    sampler->Xi[2] = Xi;
    return Xi;
}
pim_inline float VEC_CALL ScatterPrng(PtSampler *const pim_noalias sampler)
{
    float Xi = f1_wrap(sampler->Xi[3] + kSqrt5Conj);
    sampler->Xi[3] = Xi;
    return Xi;
}
pim_inline float VEC_CALL PhaseDirPrng(PtSampler *const pim_noalias sampler)
{
    float Xi = f1_wrap(sampler->Xi[4] + kSqrt7Conj);
    sampler->Xi[4] = Xi;
    return Xi;
}
pim_inline float2 VEC_CALL UvPrng(PtSampler *const pim_noalias sampler)
{
    float Xi = f1_wrap(sampler->Xi[5] + kSqrt11Conj);
    sampler->Xi[5] = Xi;
    float Yi = f1_wrap(sampler->Xi[6] + kSqrt17Conj);
    sampler->Xi[6] = Yi;
    return f2_v(Xi, Yi);
}
pim_inline float2 VEC_CALL DofPrng(PtSampler *const pim_noalias sampler)
{
    float Xi = f1_wrap(sampler->Xi[7] + kSqrt13Conj);
    sampler->Xi[7] = Xi;
    float Yi = f1_wrap(sampler->Xi[8] + kSqrt19Conj);
    sampler->Xi[8] = Yi;
    return f2_v(Xi, Yi);
}
pim_inline float VEC_CALL MisPrng(PtSampler *const pim_noalias sampler)
{
    float Xi = f1_wrap(sampler->Xi[9] + kSqrt23Conj);
    sampler->Xi[9] = Xi;
    return Xi;
}
// ----------------------------------------------------------------------------

static RTCDevice ms_device;
static PtSampler ms_samplers[kMaxThreads];

// ----------------------------------------------------------------------------

static void OnRtcError(void* user, RTCError error, const char* msg)
{
    if (error != RTC_ERROR_NONE)
    {
        Con_Logf(LogSev_Error, "rtc", "%s", msg);
        ASSERT(false);
    }
}

static bool InitRTC(void)
{
    if (!LibRtc_Init())
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
}

static void ShutdownPixelDist(void)
{
}

static void InitSamplers(void)
{
    const i32 numthreads = Task_ThreadCount();

    Prng rng = Prng_Get();
    for (i32 i = 0; i < numthreads; ++i)
    {
        ms_samplers[i].rng.state = Prng_u64(&rng);
        for (i32 j = 0; j < NELEM(ms_samplers[i].Xi); ++j)
        {
            ms_samplers[i].Xi[j] = Prng_f32(&rng);
        }
    }
    Prng_Set(rng);
}

void PtSys_Init(void)
{
    InitRTC();
    InitSamplers();
    InitPixelDist();
}

void PtSys_Update(void)
{

}

void PtSys_Shutdown(void)
{
    if (ms_device)
    {
        rtc.ReleaseDevice(ms_device);
        ms_device = NULL;
    }
    ShutdownPixelDist();
}

PtSampler VEC_CALL PtSampler_Get(void) { return GetSampler(); }
void VEC_CALL PtSampler_Set(PtSampler sampler) { SetSampler(sampler); }
float2 VEC_CALL Pt_Sample2D(PtSampler*const pim_noalias sampler) { return Sample2D(sampler); }
float VEC_CALL Pt_Sample1D(PtSampler*const pim_noalias sampler) { return Sample1D(sampler); }

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

pim_inline PointQueryUserData VEC_CALL RtcPointQuery(const PtScene*const pim_noalias scene, float4 pt)
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

static RTCScene RtcNewScene(const PtScene*const pim_noalias scene)
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

static void FlattenDrawables(PtScene*const pim_noalias scene)
{
    const Entities* drawTable = Entities_Get();
    const i32 drawCount = drawTable->count;
    const MeshId* meshes = drawTable->meshes;
    const float4x4* matrices = drawTable->matrices;
    const Material* materials = drawTable->materials;

    i32 vertCount = 0;
    float4* positions = NULL;
    float4* normals = NULL;
    float2* uvs = NULL;
    i32* matIds = NULL;

    i32 matCount = 0;
    Material* sceneMats = NULL;

    for (i32 i = 0; i < drawCount; ++i)
    {
        Mesh const *const mesh = Mesh_Get(meshes[i]);
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
        const Material material = materials[i];

        Perm_Reserve(positions, vertCount);
        Perm_Reserve(normals, vertCount);
        Perm_Reserve(uvs, vertCount);
        Perm_Reserve(matIds, vertCount);

        Perm_Reserve(sceneMats, matCount);
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
    PtSampler*const pim_noalias sampler,
    const PtScene*const pim_noalias scene,
    i32 iVert,
    i32 attempts)
{
    const i32 iMat = scene->matIds[iVert];
    const Material* mat = scene->materials + iMat;

    if (mat->flags & MatFlag_Sky)
    {
        return 1.0f;
    }

    Texture const *const romeMap = Texture_Get(mat->rome);
    if (!romeMap)
    {
        return 0.0f;
    }
    u32 const *const pim_noalias texels = romeMap->texels;
    const int2 texSize = romeMap->size;

    const float2* pim_noalias uvs = scene->uvs;
    const float2 UA = uvs[iVert + 0];
    const float2 UB = uvs[iVert + 1];
    const float2 UC = uvs[iVert + 2];

    i32 hits = 0;
    for (i32 i = 0; i < attempts; ++i)
    {
        i32 iTexel = UvWrapPow2(texSize,
            f2_blend(UA, UB, UC,
                SampleBaryCoord(Sample2D(sampler))));
        u32 sample = texels[iTexel] >> 24;
        hits += sample ? 1 : 0;
    }
    return (float)hits / (float)attempts;
}

typedef struct task_CalcEmissionPdf
{
    Task task;
    const PtScene* scene;
    float* pdfs;
    i32 attempts;
} task_CalcEmissionPdf;

static void CalcEmissionPdfFn(Task* pbase, i32 begin, i32 end)
{
    task_CalcEmissionPdf* task = (task_CalcEmissionPdf*)pbase;
    const PtScene*const pim_noalias scene = task->scene;
    const i32 attempts = task->attempts;
    float* pim_noalias pdfs = task->pdfs;

    PtSampler sampler = PtSampler_Get();
    for (i32 i = begin; i < end; ++i)
    {
        pdfs[i] = EmissionPdf(&sampler, scene, i * 3, attempts);
    }
    PtSampler_Set(sampler);
}

static void SetupEmissives(PtScene*const pim_noalias scene)
{
    const i32 vertCount = scene->vertCount;
    const i32 triCount = vertCount / 3;

    task_CalcEmissionPdf* task = Temp_Calloc(sizeof(*task));
    task->scene = scene;
    task->pdfs = Temp_Alloc(sizeof(task->pdfs[0]) * triCount);
    task->attempts = 1000;

    Task_Run(&task->task, CalcEmissionPdfFn, triCount);

    i32 emissiveCount = 0;
    i32* emitToVert = NULL;
    i32* pim_noalias vertToEmit = Perm_Alloc(sizeof(vertToEmit[0]) * vertCount);

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
            Perm_Reserve(emitToVert, emissiveCount);
            emitToVert[emissiveCount - 1] = iVert;
        }
    }

    scene->vertToEmit = vertToEmit;
    scene->emissiveCount = emissiveCount;
    scene->emitToVert = emitToVert;
}

static void SetupPortals(PtScene* scene)
{
    i32 portalCount = 0;
    i32* pim_noalias portals = NULL;
    const i32 matCount = scene->matCount;
    const Material* materials = scene->materials;
    const i32 vertCount = scene->vertCount;
    const i32* pim_noalias matIds = scene->matIds;
    for (i32 iVert = 0; iVert < vertCount; iVert += 3)
    {
        i32 iMat = matIds[iVert];
        ASSERT(iMat >= 0);
        ASSERT(iMat < matCount);
        const Material* material = &materials[iMat];
        if (material->flags & MatFlag_Portal)
        {
            ++portalCount;
            Perm_Reserve(portals, portalCount);
            portals[portalCount - 1] = iVert;
        }
    }
    scene->portalCount = portalCount;
    scene->portals = portals;
}

typedef struct task_SetupLightGrid
{
    Task task;
    PtScene* scene;
} task_SetupLightGrid;

static void SetupLightGridFn(Task* pbase, i32 begin, i32 end)
{
    task_SetupLightGrid* task = (task_SetupLightGrid*)pbase;

    PtScene*const pim_noalias scene = task->scene;

    const Grid grid = scene->lightGrid;
    Dist1D *const pim_noalias dists = scene->lightDists;

    float4 const *const pim_noalias positions = scene->positions;
    i32 const *const pim_noalias matIds = scene->matIds;
    Material const *const pim_noalias materials = scene->materials;

    const i32 emissiveCount = scene->emissiveCount;
    i32 const *const pim_noalias emitToVert = scene->emitToVert;

    const float metersPerCell = ConVar_GetFloat(&cv_pt_dist_meters);
    const float radius = metersPerCell * 0.666f;
    PtSampler sampler = GetSampler();

    float4 hamm[16];
    for (i32 i = 0; i < NELEM(hamm); ++i)
    {
        float2 Xi = Hammersley2D(i, NELEM(hamm));
        hamm[i] = SampleUnitSphere(Xi);
    }

    RTCScene rtScene = scene->rtcScene;

    for (i32 i = begin; i < end; ++i)
    {
        float4 position = Grid_Position(&grid, i);
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
                    RayHit hit = pt_intersect_local(scene, position, hamm[j], 0.0f, 1 << 20);
                    if (hit.type == Hit_Triangle)
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

        Dist1D dist = { 0 };
        Dist1D_New(&dist, emissiveCount);

        for (i32 iEmit = 0; iEmit < emissiveCount; ++iEmit)
        {
            i32 iVert = emitToVert[iEmit];
            i32 iMat = matIds[iVert];
            float4 A = positions[iVert + 0];
            float4 B = positions[iVert + 1];
            float4 C = positions[iVert + 2];

            i32 hits = 0;
            float4 ros[16];
            float4 rds[16];
            bool visibles[16];
            const i32 hitAttempts = 16;
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

            dist.pdf[iEmit] = hitPdf;
        }

        Dist1D_Bake(&dist);
        dists[i] = dist;
    }
    SetSampler(sampler);
}

static void SetupLightGrid(PtScene*const pim_noalias scene)
{
    if (scene->vertCount > 0)
    {
        Box3D bounds = box_from_pts(scene->positions, scene->vertCount);
        float metersPerCell = ConVar_GetFloat(&cv_pt_dist_meters);
        Grid grid;
        Grid_New(&grid, bounds, 1.0f / metersPerCell);
        const i32 len = Grid_Len(&grid);
        scene->lightGrid = grid;
        scene->lightDists = Tex_Calloc(sizeof(scene->lightDists[0]) * len);

        task_SetupLightGrid* task = Temp_Calloc(sizeof(*task));
        task->scene = scene;

        Task_Run(task, SetupLightGridFn, len);
    }
}

static void PtScene_FindSky(PtScene* scene)
{
    scene->sky = NULL;
    Guid skyname = Guid_FromStr("sky");
    Cubemaps* maps = Cubemaps_Get();
    i32 iSky = Cubemaps_Find(maps, skyname);
    if (iSky >= 0)
    {
        scene->sky = &maps->cubemaps[iSky];
    }
}

ProfileMark(pm_scene_update, PtScene_Update)
void PtScene_Update(PtScene *const pim_noalias scene)
{
    ProfileBegin(pm_scene_update);

    if (Entities_Get()->modtime != scene->modtime)
    {
        PtScene_Clear(scene);
        PtScene_Init(scene);
    }
    PtScene_FindSky(scene);
    UpdateDists(scene);

    ProfileEnd(pm_scene_update);
}

static void PtScene_Init(PtScene* scene)
{
    PtScene_FindSky(scene);
    FlattenDrawables(scene);
    SetupEmissives(scene);
    SetupPortals(scene);
    media_desc_new(&scene->mediaDesc);
    scene->rtcScene = RtcNewScene(scene);
    SetupLightGrid(scene);

    scene->modtime = Entities_Get()->modtime;
}

static void PtScene_Clear(PtScene* scene)
{
    if (scene->rtcScene)
    {
        RTCScene rtcScene = scene->rtcScene;
        rtc.ReleaseScene(rtcScene);
        scene->rtcScene = NULL;
    }

    Mem_Free(scene->positions);
    Mem_Free(scene->normals);
    Mem_Free(scene->uvs);
    Mem_Free(scene->matIds);
    Mem_Free(scene->vertToEmit);

    Mem_Free(scene->materials);

    Mem_Free(scene->emitToVert);
    Mem_Free(scene->portals);

    {
        const i32 gridLen = Grid_Len(&scene->lightGrid);
        Dist1D* lightDists = scene->lightDists;
        for (i32 i = 0; i < gridLen; ++i)
        {
            Dist1D_Del(lightDists + i);
        }
        Mem_Free(scene->lightDists);
    }

    memset(scene, 0, sizeof(*scene));
}

PtScene* PtScene_New(void)
{
    ASSERT(ms_device);
    if (!ms_device)
    {
        return NULL;
    }
    PtScene* scene = Perm_Calloc(sizeof(*scene));
    PtScene_Init(scene);
    return scene;
}

void PtScene_Del(PtScene*const pim_noalias scene)
{
    if (scene)
    {
        PtScene_Clear(scene);
        Mem_Free(scene);
    }
}

void PtScene_Gui(PtScene*const pim_noalias scene)
{
    if (scene && igExCollapsingHeader1("pt scene"))
    {
        igIndent(0.0f);
        igText("Vertex Count: %d", scene->vertCount);
        igText("Material Count: %d", scene->matCount);
        igText("Emissive Count: %d", scene->emissiveCount);
        media_desc_gui(&scene->mediaDesc);
        igUnindent(0.0f);
    }
}

void PtTrace_New(
    PtTrace* trace,
    PtScene*const pim_noalias scene,
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
        trace->color = Tex_Calloc(sizeof(trace->color[0]) * texelCount);
        trace->albedo = Tex_Calloc(sizeof(trace->albedo[0]) * texelCount);
        trace->normal = Tex_Calloc(sizeof(trace->normal[0]) * texelCount);
        trace->denoised = Tex_Calloc(sizeof(trace->denoised[0]) * texelCount);
        DofInfo_New(&trace->dofinfo);
    }
}

void PtTrace_Del(PtTrace* trace)
{
    if (trace)
    {
        Mem_Free(trace->color);
        Mem_Free(trace->albedo);
        Mem_Free(trace->normal);
        Mem_Free(trace->denoised);
        memset(trace, 0, sizeof(*trace));
    }
}

void PtTrace_Gui(PtTrace* trace)
{
    if (trace && igExCollapsingHeader1("pt trace"))
    {
        igIndent(0.0f);
        DofInfo_Gui(&trace->dofinfo);
        PtScene_Gui(trace->scene);
        igUnindent(0.0f);
    }
}

void DofInfo_New(DofInfo* dof)
{
    if (dof)
    {
        dof->aperture = 5.0f * kMilli;
        dof->focalLength = 6.0f;
        dof->bladeCount = 5;
        dof->bladeRot = kPi / 10.0f;
        dof->focalPlaneCurvature = 0.05f;
        dof->autoFocus = true;
        dof->autoFocusSpeed = 3.0f;
    }
}

void DofInfo_Gui(DofInfo* dof)
{
    if (dof && igExCollapsingHeader1("dofinfo"))
    {
        float aperture = dof->aperture / kMilli;
        igExSliderFloat("Aperture, millimeters", &aperture, 0.1f, 100.0f);
        dof->aperture = aperture * kMilli;
        igCheckbox("Autofocus", &dof->autoFocus);
        if (dof->autoFocus)
        {
            igExSliderFloat("Autofocus Rate", &dof->autoFocusSpeed, 0.01f, 10.0f);
        }
        else
        {
            float focalLength = log2f(dof->focalLength);
            igExSliderFloat("Focal Length, log2 meters", &focalLength, -5.0f, 10.0f);
            dof->focalLength = exp2f(focalLength);
        }
        igText("Focal Length, meters: %f", dof->focalLength);
        igExSliderFloat("Focal Plane Curvature", &dof->focalPlaneCurvature, 0.0f, 1.0f);
        igExSliderInt("Blade Count", &dof->bladeCount, 3, 666);
        igExSliderFloat("Blade Rotation", &dof->bladeRot, 0.0f, kTau);
    }
}

pim_inline float4 VEC_CALL GetPosition(
    const PtScene *const pim_noalias scene,
    RayHit hit)
{
    float4 const *const pim_noalias positions = scene->positions;
    return f4_blend(
        positions[hit.index + 0],
        positions[hit.index + 1],
        positions[hit.index + 2],
        hit.wuvt);
}

pim_inline float4 VEC_CALL GetNormal(
    const PtScene *const pim_noalias scene,
    RayHit hit)
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
    const PtScene *const pim_noalias scene,
    RayHit hit)
{
    float2 const *const pim_noalias uvs = scene->uvs;
    return f2_blend(
        uvs[hit.index + 0],
        uvs[hit.index + 1],
        uvs[hit.index + 2],
        hit.wuvt);
}

pim_inline float VEC_CALL GetArea(const PtScene *const pim_noalias scene, i32 iVert)
{
    float4 const *const pim_noalias positions = scene->positions;
    ASSERT(iVert >= 0);
    ASSERT((iVert + 2) < scene->vertCount);
    return TriArea3D(positions[iVert + 0], positions[iVert + 1], positions[iVert + 2]);
}

pim_inline Material const *const pim_noalias VEC_CALL GetMaterial(
    const PtScene *const pim_noalias scene,
    RayHit hit)
{
    i32 iVert = hit.index;
    ASSERT(iVert >= 0);
    ASSERT(iVert < scene->vertCount);
    i32 matIndex = scene->matIds[iVert];
    ASSERT(matIndex >= 0);
    ASSERT(matIndex < scene->matCount);
    return &scene->materials[matIndex];
}

pim_inline float4 VEC_CALL TriplaneBlending(float4 dir)
{
    float4 a = f4_abs(dir);
    return f4_divvs(a, a.x + a.y + a.z + kEpsilon);
}

pim_inline float4 VEC_CALL SampleSkyTex(const Texture* tex, float2 uv)
{
    float duration = kTau;// (float)Time_Sec(Time_FrameStart() - Time_AppStart());
    float2 topUv = uv;
    topUv.x += duration * 0.1f;
    topUv.y += duration * 0.1f;
    topUv.x = fmodf(topUv.x, 0.5f);
    float4 albedo = UvWrapPow2_c32(tex->texels, tex->size, topUv);
    if (f4_hmax3(albedo) <= (1.0f / 255.0f))
    {
        float2 bottomUv = uv;
        bottomUv.x += duration * 0.05f;
        bottomUv.y += duration * 0.05f;
        bottomUv.x = 0.5f + fmodf(bottomUv.x, 0.5f);
        albedo = UvWrapPow2_c32(tex->texels, tex->size, bottomUv);
    }
    return albedo;
}

pim_inline float4 VEC_CALL TriplaneSampleSkyTex(const Texture* tex, float4 dir, float4 pos)
{
    float4 b = TriplaneBlending(dir);
    float4 x = f4_mulvs(SampleSkyTex(tex, f2_v(pos.z, pos.y)), b.x);
    float4 y = f4_mulvs(SampleSkyTex(tex, f2_v(pos.x, pos.z)), b.y);
    float4 z = f4_mulvs(SampleSkyTex(tex, f2_v(pos.x, pos.y)), b.z);
    return f4_add(x, f4_add(y, z));
}

pim_inline float4 VEC_CALL GetSky(
    const PtScene*const pim_noalias scene,
    float4 ro,
    float4 rd,
    RayHit hit)
{
    if (ConVar_GetBool(&cv_pt_retro))
    {
        const Material* mat = GetMaterial(scene, hit);
        const Texture* tex = Texture_Get(mat->albedo);
        if (tex)
        {
            return SampleSkyTex(tex, GetUV(scene, hit));
        }
    }
    if (scene->sky)
    {
        return f3_f4(Cubemap_ReadColor(scene->sky, rd), 1.0f);
    }
    return f4_0;
}

pim_inline float4 VEC_CALL GetEmission(
    const PtScene*const pim_noalias scene,
    float4 ro,
    float4 rd,
    RayHit hit,
    i32 bounce)
{
    if (hit.flags & MatFlag_Sky)
    {
        return GetSky(scene, ro, rd, hit);
    }
    else
    {
        Material const *const pim_noalias mat = GetMaterial(scene, hit);
        float2 uv = GetUV(scene, hit);
        float4 albedo = f4_1;
        {
            Texture const *const tex = Texture_Get(mat->albedo);
            if (tex)
            {
                albedo = UvBilinearWrapPow2_c32_fast(tex->texels, tex->size, uv);
            }
        }
        float e = 0.0f;
        {
            Texture const *const tex = Texture_Get(mat->rome);
            if (tex)
            {
                e = UvBilinearWrapPow2_c32_fast(tex->texels, tex->size, uv).w;
            }
        }
        return UnpackEmission(albedo, e);
    }
}

pim_inline SurfHit VEC_CALL GetSurface(
    const PtScene *const pim_noalias scene,
    float4 ro,
    float4 rd,
    RayHit hit,
    i32 bounce)
{
    SurfHit surf;
    surf.type = hit.type;

    Material const *const pim_noalias mat = GetMaterial(scene, hit);
    surf.flags = mat->flags;
    surf.ior = mat->ior;
    float2 uv = GetUV(scene, hit);
    surf.M = GetNormal(scene, hit);
    surf.N = surf.M;
    surf.P = GetPosition(scene, hit);
    surf.P = f4_add(surf.P, f4_mulvs(surf.M, 0.01f * kMilli));

    if (hit.flags & MatFlag_Sky)
    {
        surf.albedo = f4_0;
        surf.roughness = 1.0f;
        surf.occlusion = 0.0f;
        surf.metallic = 0.0f;
        surf.emission = GetSky(scene, ro, rd, hit);
    }
    else
    {
        {
            Texture const *const pim_noalias tex = Texture_Get(mat->normal);
            if (tex)
            {
                float4 Nts = UvBilinearWrapPow2_xy16(tex->texels, tex->size, uv);
                surf.N = TanToWorld(surf.N, Nts);
            }
        }

        surf.albedo = f4_1;
        {
            Texture const *const pim_noalias tex = Texture_Get(mat->albedo);
            if (tex)
            {
                surf.albedo = UvBilinearWrapPow2_c32_fast(tex->texels, tex->size, uv);
            }
        }

        float4 rome = f4_v(0.5f, 1.0f, 0.0f, 0.0f);
        {
            Texture const *const pim_noalias tex = Texture_Get(mat->rome);
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

pim_inline SurfHit VEC_CALL GetSurfaceRetro(
    const PtScene *const pim_noalias scene,
    float4 ro,
    float4 rd,
    RayHit hit,
    i32 bounce)
{
    SurfHit surf;
    surf.type = hit.type;

    Material const *const pim_noalias mat = GetMaterial(scene, hit);
    surf.flags = mat->flags;
    surf.ior = mat->ior;
    float2 uv = GetUV(scene, hit);
    surf.M = GetNormal(scene, hit);
    surf.N = surf.M;
    surf.P = GetPosition(scene, hit);
    surf.P = f4_add(surf.P, f4_mulvs(surf.M, 0.01f * kMilli));

    if (hit.flags & MatFlag_Sky)
    {
        surf.albedo = f4_0;
        surf.roughness = 1.0f;
        surf.occlusion = 0.0f;
        surf.metallic = 0.0f;
        surf.emission = GetSky(scene, ro, rd, hit);
    }
    else
    {
        surf.albedo = f4_1;
        {
            Texture const *const pim_noalias tex = Texture_Get(mat->albedo);
            if (tex)
            {
                surf.albedo = UvWrapPow2_c32(tex->texels, tex->size, uv);
            }
        }

        float4 rome = f4_v(0.5f, 1.0f, 0.0f, 0.0f);
        {
            Texture const *const pim_noalias tex = Texture_Get(mat->rome);
            if (tex)
            {
                rome = UvWrapPow2_c32(tex->texels, tex->size, uv);
            }
        }
        surf.emission = UnpackEmission(surf.albedo, rome.w);
        surf.roughness = rome.x;
        surf.occlusion = rome.y;
        surf.metallic = rome.z;
    }

    return surf;
}

pim_inline RayHit VEC_CALL pt_intersect_local(
    const PtScene *const pim_noalias scene,
    float4 ro,
    float4 rd,
    float tNear,
    float tFar)
{
    RayHit hit = { 0 };
    hit.wuvt.w = -1.0f;
    hit.index = -1;

    RTCRayHit rtcHit = RtcIntersect(scene->rtcScene, ro, rd, tNear, tFar);
    hit.normal = f4_v(rtcHit.hit.Ng_x, rtcHit.hit.Ng_y, rtcHit.hit.Ng_z, 0.0f);
    bool hitNothing =
        (rtcHit.hit.geomID == RTC_INVALID_GEOMETRY_ID) ||
        (rtcHit.ray.tfar <= 0.0f);
    if (hitNothing)
    {
        hit.type = Hit_Nothing;
        return hit;
    }
    hit.type = Hit_Triangle;
    if (f4_dot3(hit.normal, rd) > 0.0f)
    {
        hit.normal = f4_neg(hit.normal);
        hit.type = Hit_Backface;
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

RayHit VEC_CALL Pt_Intersect(
    PtScene *const pim_noalias scene,
    float4 ro,
    float4 rd,
    float tNear,
    float tFar)
{
    return pt_intersect_local(scene, ro, rd, tNear, tFar);
}

static Scatter VEC_CALL PortalScatter(
    PtScene *const pim_noalias scene,
    PtSampler*const pim_noalias sampler,
    const SurfHit* surf,
    float4 I)
{
    Scatter scatter = { 0 };
    i32 portalCount = scene->portalCount;
    if (portalCount)
    {
        i32 iPortal = Prng_i32(&sampler->rng) % portalCount;
        i32 iVert = scene->portals[iPortal];
        float4 wuvt = SampleBaryCoord(Sample2D(sampler));
        float4 const *const pim_noalias positions = scene->positions;
        float4 const *const pim_noalias normals = scene->normals;
        float4 P = f4_blend(positions[iVert + 0], positions[iVert + 1], positions[iVert + 2], wuvt);
        float4 N = f4_normalize3(f4_blend(normals[iVert + 0], normals[iVert + 1], normals[iVert + 2], wuvt));
        P = f4_add(P, f4_mulvs(N, kMilli * 2.0f));
        float4 rd = I;
        if (f4_dot3(rd, N) < 0.0f)
        {
            rd = f4_neg(rd);
        }
        scatter.pos = P;
        scatter.dir = rd;
        scatter.attenuation = f4_s(0.5f);
        scatter.pdf = 1.0f;
    }
    return scatter;
}

// returns attenuation value for the given interaction
pim_inline float4 VEC_CALL BrdfEval(
    PtSampler*const pim_noalias sampler,
    float4 I,
    const SurfHit* surf,
    float4 L)
{
    ASSERT(IsUnitLength(I));
    ASSERT(IsUnitLength(L));
    float4 N = surf->N;
    ASSERT(IsUnitLength(N));
    if (surf->flags & (MatFlag_Refractive | MatFlag_Portal))
    {
        return f4_0;
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

    float diffusePdf = amtDiffuse * ImportanceSampleLambertPdf(NoL);
    float specularPdf = amtSpecular * GGXPdf(NoH, HoV, alpha);
    float pdf = diffusePdf + specularPdf;

    brdf.w = pdf;
    return brdf;
}

pim_inline float4 VEC_CALL BrdfEvalRetro(
    PtSampler*const pim_noalias sampler,
    float4 I,
    const SurfHit* surf,
    float4 L)
{
    if (surf->flags & (MatFlag_Refractive | MatFlag_Portal))
    {
        return f4_0;
    }
    float4 N = surf->N;
    float NoL = f4_dot3(N, L);
    if (NoL <= 0.0f)
    {
        return f4_0;
    }

    float4 V = f4_neg(I);
    float4 H = f4_normalize3(f4_add(V, L));
    float NoV = f4_dotsat(N, V);
    float HoV = f4_dotsat(H, V);
    float fd = Fd_Burley(NoL, NoV, HoV, 1.0f);
    float4 brdf = f4_mulvs(f4_mulvs(surf->albedo, fd), NoL);
    brdf.w = ImportanceSampleLambertPdf(NoL);
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
    PtSampler *const pim_noalias sampler,
    float4 I,
    const SurfHit* surf,
    float4 L)
{
    return f4_0;
}

pim_inline Scatter VEC_CALL RefractScatter(
    PtSampler*const pim_noalias sampler,
    const SurfHit* surf,
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
    bool entering = surf->type != Hit_Backface;
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

    Scatter result = { 0 };
    result.pos = P;
    result.dir = L;
    result.attenuation = tir ? f4_1 : F;
    result.pdf = pdf;

    return result;
}

pim_inline Scatter VEC_CALL RefractScatterRetro(
    PtSampler*const pim_noalias sampler,
    const SurfHit* surf,
    float4 I)
{
    const float kAir = 1.000277f;
    const float matIor = f1_max(1.0f, surf->ior);
    const float alpha = BrdfAlpha(0.01f);

    float4 V = f4_neg(I);
    float4 M = surf->M;
    float4 m = TanToWorld(surf->N, SampleGGXMicrofacet(Sample2D(sampler), alpha));
    m = f4_dot3(m, M) > 0.0f ? m : f4_reflect3(m, M);
    m = f4_normalize3(m);
    bool entering = surf->type != Hit_Backface;
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

    Scatter result = { 0 };
    result.pos = P;
    result.dir = L;
    result.attenuation = tir ? f4_1 : F;
    result.pdf = pdf;

    return result;
}

pim_inline Scatter VEC_CALL BrdfScatter(
    PtSampler *const pim_noalias sampler,
    PtScene *const pim_noalias scene,
    const SurfHit* surf,
    float4 I)
{
    ASSERT(IsUnitLength(I));
    ASSERT(IsUnitLength(surf->N));
    if (surf->flags & MatFlag_Portal)
    {
        return PortalScatter(scene, sampler, surf, I);
    }
    if (surf->flags & MatFlag_Refractive)
    {
        return RefractScatter(sampler, surf, I);
    }

    Scatter result = { 0 };
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
    float diffusePdf = amtDiffuse * ImportanceSampleLambertPdf(NoL);
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
        float4 Fd = f4_mulvs(albedo, Fd_Burley(NoL, NoV, HoV, roughness));
        Fd = f4_mulvs(Fd, amtDiffuse);
        brdf = f4_add(brdf, Fd);
    }

    result.dir = L;
    result.pdf = pdf;
    result.attenuation = f4_mulvs(brdf, NoL);

    return result;
}

pim_inline Scatter VEC_CALL BrdfScatterRetro(
    PtSampler *const pim_noalias sampler,
    PtScene *const pim_noalias scene,
    const SurfHit* surf,
    float4 I)
{
    ASSERT(IsUnitLength(I));
    ASSERT(IsUnitLength(surf->N));
    if (surf->flags & MatFlag_Portal)
    {
        return PortalScatter(scene, sampler, surf, I);
    }
    if (surf->flags & MatFlag_Refractive)
    {
        return RefractScatterRetro(sampler, surf, I);
    }

    Scatter result = { 0 };
    result.pos = surf->P;

    float4 N = surf->N;
    float4 L = TanToWorld(N, SampleCosineHemisphere(Sample2D(sampler)));
    float NoL = f4_dot3(N, L);
    float pdf = ImportanceSampleLambertPdf(NoL);
    if (pdf <= 0.0f)
    {
        return result;
    }

    float4 V = f4_neg(I);
    float4 H = f4_normalize3(f4_add(V, L));
    float NoV = f4_dotsat(N, V);
    float HoV = f4_dotsat(H, V);
    float fd = Fd_Burley(NoL, NoV, HoV, 1.0f);

    result.dir = L;
    result.pdf = pdf;
    result.attenuation = f4_mulvs(f4_mulvs(surf->albedo, fd), NoL);

    return result;
}

pim_inline void VEC_CALL LightOnHit(
    PtSampler *const pim_noalias sampler,
    PtScene *const pim_noalias scene,
    float4 ro,
    float4 lum4,
    i32 iVert)
{
    u32 amt = (u32)(f4_avglum(lum4) * 64.0f + 0.5f);
    i32 iGrid = Grid_Index(&scene->lightGrid, ro);
    i32 iEmit = scene->vertToEmit[iVert];
    if (iEmit >= 0)
    {
        Dist1D* pim_noalias dist = &scene->lightDists[iGrid];
        if (dist->length)
        {
            fetch_add_u32(&dist->live[iEmit], amt, MO_Relaxed);
        }
    }
}

pim_inline bool VEC_CALL LightSelect(
    PtSampler*const pim_noalias sampler,
    PtScene*const pim_noalias scene,
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

    i32 iCell = Grid_Index(&scene->lightGrid, position);
    const Dist1D* dist = scene->lightDists + iCell;
    if (!dist->length)
    {
        return false;
    }

    i32 iEmit = Dist1D_SampleD(dist, LightSelectPrng(sampler));
    float pdf = Dist1D_PdfD(dist, iEmit);

    i32 iVert = scene->emitToVert[iEmit];

    *iVertOut = iVert;
    *pdfOut = pdf;
    return pdf > kEpsilon;
}

pim_inline float VEC_CALL LightSelectPdf(
    PtScene *const pim_noalias scene,
    i32 iVert,
    float4 ro)
{
    float selectPdf = 1.0f;
    i32 iGrid = Grid_Index(&scene->lightGrid, ro);
    i32 iEmit = scene->vertToEmit[iVert];
    if (iEmit >= 0)
    {
        const Dist1D* pim_noalias dist = &scene->lightDists[iGrid];
        if (dist->length)
        {
            selectPdf = Dist1D_PdfD(dist, iEmit);
        }
    }
    return selectPdf;
}

pim_inline LightSample VEC_CALL SampleLight(
    PtSampler *const pim_noalias sampler,
    PtScene *const pim_noalias scene,
    float4 ro,
    i32 iVert,
    i32 bounce)
{
    LightSample sample = { 0 };

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

    RayHit hit = pt_intersect_local(scene, ro, rd, 0.0f, distance + 0.01f * kMilli);
    if ((hit.type != Hit_Nothing) && (hit.index == iVert))
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

pim_inline LightSample VEC_CALL LightSampleRetro(
    PtSampler *const pim_noalias sampler,
    PtScene *const pim_noalias scene,
    float4 ro,
    i32 iVert,
    i32 bounce)
{
    LightSample sample = { 0 };

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

    RayHit hit = pt_intersect_local(scene, ro, rd, 0.0f, distance + 0.01f * kMilli);
    if ((hit.type != Hit_Nothing) && (hit.index == iVert))
    {
        float cosTheta = f1_abs(f4_dot3(rd, hit.normal));
        sample.pdf = LightPdf(area, cosTheta, distSq);
        sample.luminance = GetEmission(scene, ro, rd, hit, bounce);
    }

    return sample;
}

pim_inline float VEC_CALL LightEvalPdf(
    PtSampler *const pim_noalias sampler,
    PtScene *const pim_noalias scene,
    float4 ro,
    float4 rd,
    RayHit *const pim_noalias hitOut)
{
    ASSERT(IsUnitLength(rd));
    RayHit hit = pt_intersect_local(scene, ro, rd, 0.0f, 1 << 20);
    float pdf = 0.0f;
    if (hit.type != Hit_Nothing)
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
    PtSampler *const pim_noalias sampler,
    PtScene *const pim_noalias scene,
    SurfHit const *const pim_noalias surf,
    RayHit const *const pim_noalias srcHit,
    float4 I,
    i32 bounce)
{
    float4 result = f4_0;
    if (surf->flags & (MatFlag_Refractive | MatFlag_Portal))
    {
        return result;
    }

    const float4 ro = surf->P;
    const float pRough = f1_lerp(0.05f, 0.95f, surf->roughness);
    const float pSmooth = 1.0f - pRough;
    if (MisPrng(sampler) < pRough)
    {
        i32 iVert;
        float selectPdf;
        if (LightSelect(sampler, scene, surf->P, &iVert, &selectPdf))
        {
            if (srcHit->index != iVert)
            {
                // already has CalcTransmittance applied
                LightSample sample = SampleLight(sampler, scene, ro, iVert, bounce);
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
        Scatter sample = BrdfScatter(sampler, scene, surf, I);
        float4 rd = sample.dir;
        float brdfPdf = sample.pdf * pSmooth;
        if (brdfPdf > kEpsilon)
        {
            RayHit hit;
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

pim_inline float4 VEC_CALL EstimateDirectRetro(
    PtSampler *const pim_noalias sampler,
    PtScene *const pim_noalias scene,
    SurfHit const *const pim_noalias surf,
    RayHit const *const pim_noalias srcHit,
    float4 I,
    i32 bounce)
{
    float4 result = f4_0;
    if (surf->flags & (MatFlag_Refractive | MatFlag_Portal))
    {
        return result;
    }

    {
        i32 iVert;
        float selectPdf;
        if (LightSelect(sampler, scene, surf->P, &iVert, &selectPdf))
        {
            if (srcHit->index != iVert)
            {
                LightSample sample = LightSampleRetro(sampler, scene, surf->P, iVert, bounce);
                float4 rd = sample.direction;
                float4 Li = sample.luminance;
                float lightPdf = sample.pdf * selectPdf;
                if (lightPdf > kEpsilon)
                {
                    float4 brdf = BrdfEvalRetro(sampler, I, surf, rd);
                    Li = f4_mul(Li, brdf);
                    float brdfPdf = brdf.w;
                    if (brdfPdf > kEpsilon)
                    {
                        Li = f4_mulvs(Li, 1.0f / lightPdf);
                        result = f4_add(result, Li);
                    }
                }
            }
        }
    }

    return result;
}

pim_inline bool VEC_CALL EvaluateLight(
    PtSampler*const pim_noalias sampler,
    PtScene*const pim_noalias scene,
    float4 P,
    float4 *const pim_noalias lightOut,
    float4 *const pim_noalias dirOut,
    i32 bounce)
{
    i32 iVert;
    float selectPdf;
    if (LightSelect(sampler, scene, P, &iVert, &selectPdf))
    {
        LightSample sample = SampleLight(sampler, scene, P, iVert, bounce);
        if (sample.pdf > kEpsilon)
        {
            *lightOut = f4_divvs(sample.luminance, sample.pdf * selectPdf);
            *dirOut = sample.direction;
            return true;
        }
    }
    return false;
}

static void media_desc_new(MediaDesc *const desc)
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

static void media_desc_update(MediaDesc *const desc)
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

static void media_desc_load(MediaDesc *const desc, const char* name)
{
    if (!desc || !name)
    {
        return;
    }
    memset(desc, 0, sizeof(*desc));
    media_desc_new(desc);
    char filename[PIM_PATH];
    SPrintf(ARGS(filename), "%s.json", name);
    SerObj* root = Ser_ReadFile(filename);
    if (root)
    {
        Ser_LoadVector(desc, root, constantAlbedo);
        Ser_LoadVector(desc, root, noiseAlbedo);
        Ser_LoadFloat(desc, root, absorption);
        Ser_LoadFloat(desc, root, constantAmt);
        Ser_LoadFloat(desc, root, noiseAmt);
        Ser_LoadInt(desc, root, noiseOctaves);
        Ser_LoadFloat(desc, root, noiseGain);
        Ser_LoadFloat(desc, root, noiseLacunarity);
        Ser_LoadFloat(desc, root, noiseFreq);
        Ser_LoadFloat(desc, root, noiseScale);
        Ser_LoadFloat(desc, root, noiseHeight);
        Ser_LoadFloat(desc, root, phaseDirA);
        Ser_LoadFloat(desc, root, phaseDirB);
        Ser_LoadFloat(desc, root, phaseBlend);
    }
    else
    {
        Con_Logf(LogSev_Error, "pt", "Failed to load media desc '%s'", filename);
        media_desc_new(desc);
    }
    SerObj_Del(root);
}

static void media_desc_save(MediaDesc const *const desc, const char* name)
{
    if (!desc || !name)
    {
        return;
    }
    char filename[PIM_PATH];
    SPrintf(ARGS(filename), "%s.json", name);
    SerObj* root = SerObj_Dict();
    if (root)
    {
        Ser_SaveVector(desc, root, constantAlbedo);
        Ser_SaveVector(desc, root, noiseAlbedo);
        Ser_SaveFloat(desc, root, absorption);
        Ser_SaveFloat(desc, root, constantAmt);
        Ser_SaveFloat(desc, root, noiseAmt);
        Ser_SaveInt(desc, root, noiseOctaves);
        Ser_SaveFloat(desc, root, noiseGain);
        Ser_SaveFloat(desc, root, noiseLacunarity);
        Ser_SaveFloat(desc, root, noiseFreq);
        Ser_SaveFloat(desc, root, noiseScale);
        Ser_SaveFloat(desc, root, noiseHeight);
        Ser_SaveFloat(desc, root, phaseDirA);
        Ser_SaveFloat(desc, root, phaseDirB);
        Ser_SaveFloat(desc, root, phaseBlend);
        if (!Ser_WriteFile(filename, root))
        {
            Con_Logf(LogSev_Error, "pt", "Failed to save media desc '%s'", filename);
        }
        SerObj_Del(root);
    }
}

static void igLog2SliderFloat(const char* label, float* pLinear, float log2Lo, float log2Hi)
{
    float asLog2 = log2f(*pLinear);
    igExSliderFloat(label, &asLog2, log2Lo, log2Hi);
    *pLinear = exp2f(asLog2);
}

static char gui_mediadesc_name[PIM_PATH];

static void media_desc_gui(MediaDesc *const desc)
{
    const u32 hdrPicker = ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_InputRGB;
    const u32 ldrPicker = ImGuiColorEditFlags_Float | ImGuiColorEditFlags_InputRGB;

    if (desc && igExCollapsingHeader1("MediaDesc"))
    {
        igIndent(0.0f);
        igInputText("Preset Name", gui_mediadesc_name, sizeof(gui_mediadesc_name), 0, NULL, NULL);
        if (igExButton("Load Preset"))
        {
            media_desc_load(desc, gui_mediadesc_name);
        }
        if (igExButton("Save Preset"))
        {
            media_desc_save(desc, gui_mediadesc_name);
        }
        igExSliderFloat("Phase Dir A", &desc->phaseDirA, -0.99f, 0.99f);
        igExSliderFloat("Phase Dir B", &desc->phaseDirB, -0.99f, 0.99f);
        igExSliderFloat("Phase Blend", &desc->phaseBlend, 0.0f, 1.0f);
        igLog2SliderFloat("Log2 Absorption", &desc->absorption, -20.0f, 5.0f);
        igColorEdit3("Constant Albedo", &desc->constantAlbedo.x, ldrPicker);
        igLog2SliderFloat("Log2 Constant Amount", &desc->constantAmt, -20.0f, 5.0f);
        igColorEdit3("Noise Albedo", &desc->noiseAlbedo.x, ldrPicker);
        igLog2SliderFloat("Log2 Noise Amount", &desc->noiseAmt, -20.0f, 5.0f);
        igExSliderInt("Noise Octaves", &desc->noiseOctaves, 1, 10);
        igExSliderFloat("Noise Gain", &desc->noiseGain, 0.0f, 1.0f);
        igExSliderFloat("Noise Lacunarity", &desc->noiseLacunarity, 1.0f, 3.0f);
        igLog2SliderFloat("Log2 Noise Frequency", &desc->noiseFreq, -5.0f, 5.0f);
        igExSliderFloat("Noise Height", &desc->noiseHeight, -20.0f, 20.0f);
        igLog2SliderFloat("Log2 Noise Scale", &desc->noiseScale, -5.0f, 5.0f);
        igUnindent(0.0f);

        media_desc_update(desc);
    }
}

pim_inline float2 VEC_CALL Media_Density(MediaDesc const *const desc, float4 P)
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

pim_inline Media VEC_CALL Media_Sample(MediaDesc const *const desc, float4 P)
{
    float2 density = Media_Density(desc, P);
    float totalScattering = density.x;
    float totalExtinction = totalScattering * (1.0f + desc->absorption);
    float4 logAlbedo = f4_lerpvs(desc->logConstantAlbedo, desc->logNoiseAlbedo, density.y);

    Media result;
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

pim_inline float4 VEC_CALL Media_Extinction(Media media)
{
    return f4_mulvs(media.logAlbedo, media.extinction);
}

pim_inline float VEC_CALL Media_Scattering(Media media)
{
    return f1_lerp(0.5f, 1.0f, f4_hmax3(media.logAlbedo)) * media.scattering;
}

pim_inline float4 VEC_CALL Media_Albedo(MediaDesc const *const desc, float4 P)
{
    return f4_inv(Media_Sample(desc, P).logAlbedo);
}

pim_inline float4 VEC_CALL Media_Normal(MediaDesc const *const desc, float4 P)
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

pim_inline Media VEC_CALL Media_Lerp(Media lhs, Media rhs, float t)
{
    Media result;
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
pim_inline float4 VEC_CALL CalcMajorant(MediaDesc const *const desc)
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

pim_inline float4 VEC_CALL CalcControl(MediaDesc const *const desc)
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
    MediaDesc const *const desc,
    float cosTheta)
{
    return f1_lerp(MiePhase(cosTheta, desc->phaseDirA), MiePhase(cosTheta, desc->phaseDirB), desc->phaseBlend);
}

pim_inline float4 VEC_CALL SamplePhaseDir(
    PtSampler*const pim_noalias sampler,
    MediaDesc const *const pim_noalias desc,
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
    PtSampler *const pim_noalias sampler,
    const PtScene *const pim_noalias scene,
    float4 ro,
    float4 rd,
    float rayLen)
{
    MediaDesc const *const pim_noalias desc = &scene->mediaDesc;
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
        Media media = Media_Sample(desc, P);
        float4 uT = Media_Extinction(media);
        float4 ratio = f4_inv(f4_mulvs(uT, rcpU));
        attenuation = f4_mul(attenuation, ratio);
    }
    return attenuation;
}

pim_inline Scatter VEC_CALL ScatterRay(
    PtSampler *const pim_noalias sampler,
    PtScene *const pim_noalias scene,
    float4 ro,
    float4 rd,
    float rayLen,
    i32 bounce)
{
    Scatter result = { 0 };
    result.pdf = 0.0f;

    MediaDesc const *const pim_noalias desc = &scene->mediaDesc;
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

        Media media = Media_Sample(desc, P);

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

PtResult VEC_CALL Pt_TraceRayRetro(
    PtSampler *const pim_noalias sampler,
    PtScene *const pim_noalias scene,
    float4 ro,
    float4 rd)
{
    PtResult result = { 0 };
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

        RayHit hit = pt_intersect_local(scene, ro, rd, 0.0f, 1 << 20);
        if (hit.type == Hit_Nothing)
        {
            break;
        }

        SurfHit surf = GetSurfaceRetro(scene, ro, rd, hit, b);
        if (hit.type == Hit_Backface && !(surf.flags & MatFlag_Refractive))
        {
            break;
        }

        if (b > 0)
        {
            LightOnHit(sampler, scene, ro, surf.emission, hit.index);
        }

        {
            Scatter scatter = ScatterRay(sampler, scene, ro, rd, hit.wuvt.w, b);
            luminance = f4_add(luminance, f4_mul(scatter.luminance, attenuation));
            if (scatter.pdf > kEpsilon)
            {
                {
                    float t = 1.0f / (b + 1);
                    float4 albedo = Media_Albedo(&scene->mediaDesc, scatter.pos);
                    result.albedo = f3_lerp(result.albedo, f4_f3(albedo), t);
                    result.normal = f3_lerp(result.normal, f4_f3(f4_neg(rd)), t);
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

        {
            float t = 1.0f / (b + 1);
            result.albedo = f3_lerp(result.albedo, f4_f3(surf.albedo), t);
            result.normal = f3_lerp(result.normal, f4_f3(surf.N), t);
        }
        if ((b == 0) || (prevFlags & MatFlag_Refractive))
        {
            if (!(hit.flags & MatFlag_Portal))
            {
                luminance = f4_add(luminance, f4_mul(surf.emission, attenuation));
            }
        }
        if (hit.flags & MatFlag_Sky)
        {
            break;
        }

        {
            float4 Li = EstimateDirectRetro(sampler, scene, &surf, &hit, rd, b);
            luminance = f4_add(luminance, f4_mul(Li, attenuation));
        }

        Scatter scatter = BrdfScatterRetro(sampler, scene, &surf, rd);
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

PtResult VEC_CALL Pt_TraceRay(
    PtSampler *const pim_noalias sampler,
    PtScene *const pim_noalias scene,
    float4 ro,
    float4 rd)
{
    PtResult result = { 0 };
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

        RayHit hit = pt_intersect_local(scene, ro, rd, 0.0f, 1 << 20);
        if (hit.type == Hit_Nothing)
        {
            break;
        }

        SurfHit surf = GetSurface(scene, ro, rd, hit, b);
        if (hit.type == Hit_Backface && !(surf.flags & MatFlag_Refractive))
        {
            break;
        }

        if (b > 0)
        {
            LightOnHit(sampler, scene, ro, surf.emission, hit.index);
        }

        {
            Scatter scatter = ScatterRay(sampler, scene, ro, rd, hit.wuvt.w, b);
            luminance = f4_add(luminance, f4_mul(scatter.luminance, attenuation));
            if (scatter.pdf > kEpsilon)
            {
                {
                    float t = 1.0f / (b + 1);
                    float4 albedo = Media_Albedo(&scene->mediaDesc, scatter.pos);
                    result.albedo = f3_lerp(result.albedo, f4_f3(albedo), t);
                    result.normal = f3_lerp(result.normal, f4_f3(f4_neg(rd)), t);
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

        {
            float t = 1.0f / (b + 1);
            result.albedo = f3_lerp(result.albedo, f4_f3(surf.albedo), t);
            result.normal = f3_lerp(result.normal, f4_f3(surf.N), t);
        }
        if ((b == 0) || (prevFlags & MatFlag_Refractive))
        {
            if (!(hit.flags & MatFlag_Portal))
            {
                luminance = f4_add(luminance, f4_mul(surf.emission, attenuation));
            }
        }
        if (hit.flags & MatFlag_Sky)
        {
            break;
        }

        {
            float4 Li = EstimateDirect(sampler, scene, &surf, &hit, rd, b);
            luminance = f4_add(luminance, f4_mul(Li, attenuation));
        }

        Scatter scatter = BrdfScatter(sampler, scene, &surf, rd);
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
    PtSampler*const pim_noalias sampler)
{
    // https://www.desmos.com/calculator/g8turex13k
    float2 Xi = UvPrng(sampler);
    float angle = Xi.x * kTau;
    float radius = f1_gauss_invcdf(Xi.y, 0.0f, 1.0f);
    Xi.x = cosf(angle);
    Xi.y = sinf(angle);
    return f2_mulvs(Xi, radius);
}

pim_inline Ray VEC_CALL CalculateDof(
    PtSampler*const pim_noalias sampler,
    const DofInfo* dof,
    float4 right,
    float4 up,
    float4 fwd,
    Ray ray)
{
    float2 offset;
    {
        i32 side = Prng_i32(&sampler->rng);
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
    Task task;
    PtScene* scene;
    float alpha;
    u32 minSamples;
} TaskUpdateDists;

static void UpdateDistsFn(void* pbase, i32 begin, i32 end)
{
    TaskUpdateDists*const pim_noalias task = pbase;
    PtScene*const pim_noalias scene = task->scene;
    Dist1D*const pim_noalias dists = scene->lightDists;
    float alpha = task->alpha;
    u32 minSamples = task->minSamples;
    for (i32 i = begin; i < end; ++i)
    {
        Dist1D_Update(&dists[i], alpha, minSamples);
    }
}

ProfileMark(pm_updatedists, UpdateDists)
static void UpdateDists(PtScene*const pim_noalias scene)
{
    i32 worklen = Grid_Len(&scene->lightGrid);
    if (worklen > 0)
    {
        ProfileBegin(pm_updatedists);
        TaskUpdateDists *const pim_noalias task = Temp_Calloc(sizeof(*task));
        task->scene = scene;
        task->alpha = ConVar_GetFloat(&cv_pt_dist_alpha);
        task->minSamples = ConVar_GetInt(&cv_pt_dist_samples);
        Task_Run(task, UpdateDistsFn, worklen);
        ProfileEnd(pm_updatedists);
    }
}

static void DofUpdate(PtTrace* trace, const Camera* camera)
{
    DofInfo* dof = &trace->dofinfo;
    if (dof->autoFocus)
    {
        float4 ro = camera->position;
        float4 rd = quat_fwd(camera->rotation);
        RayHit hit = pt_intersect_local(trace->scene, ro, rd, 0.0f, 1 << 20);
        if (hit.type != Hit_Nothing)
        {
            float dt = (float)Time_Deltaf();
            float rate = dof->autoFocusSpeed;
            float t = 1.0f - expf(-dt * rate);
            dof->focalLength = f1_lerp(dof->focalLength, hit.wuvt.w, t);
        }
    }
}

typedef struct trace_task_s
{
    Task task;
    PtTrace* trace;
    Camera camera;
} trace_task_t;

static void TraceFn(void* pbase, i32 begin, i32 end)
{
    trace_task_t *const pim_noalias task = pbase;

    PtTrace *const pim_noalias trace = task->trace;
    const Camera camera = task->camera;

    PtScene *const pim_noalias scene = trace->scene;
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
    const DofInfo dof = trace->dofinfo;

    const bool pt_retro = ConVar_GetBool(&cv_pt_retro);

    PtSampler sampler = GetSampler();
    for (i32 i = begin; i < end; ++i)
    {
        int2 coord = { i % size.x, i / size.x };

        // gaussian AA filter
        float2 uv = { (coord.x + 0.5f), (coord.y + 0.5f) };
        float2 Xi = SampleUv(&sampler);
        uv = f2_snorm(f2_mul(f2_add(uv, Xi), rcpSize));

        Ray ray = { eye, proj_dir(right, up, fwd, slope, uv) };
        ray = CalculateDof(&sampler, &dof, right, up, fwd, ray);

        PtResult result;
        if (!pt_retro)
        {
            result = Pt_TraceRay(&sampler, scene, ray.ro, ray.rd);
        }
        else
        {
            result = Pt_TraceRayRetro(&sampler, scene, ray.ro, ray.rd);
        }
        color[i] = f3_lerp(color[i], result.color, sampleWeight);
        albedo[i] = f3_lerp(albedo[i], result.albedo, sampleWeight);
        normal[i] = f3_lerp(normal[i], result.normal, sampleWeight);
    }
    SetSampler(sampler);
}

ProfileMark(pm_trace, Pt_Trace)
void Pt_Trace(PtTrace* desc, const Camera* camera)
{
    ProfileBegin(pm_trace);

    ASSERT(desc);
    ASSERT(desc->scene);
    ASSERT(camera);
    ASSERT(desc->color);
    ASSERT(desc->albedo);
    ASSERT(desc->normal);

    DofUpdate(desc, camera);

    PtScene_Update(desc->scene);

    trace_task_t *const pim_noalias task = Temp_Calloc(sizeof(*task));
    task->trace = desc;
    task->camera = *camera;
    const i32 workSize = desc->imageSize.x * desc->imageSize.y;
    Task_Run(task, TraceFn, workSize);

    ProfileEnd(pm_trace);
}

typedef struct pt_raygen_s
{
    Task task;
    PtScene* scene;
    float4 origin;
    float4* colors;
    float4* directions;
    float2 Xi;
} pt_raygen_t;

static void RayGenFn(void* pBase, i32 begin, i32 end)
{
    pt_raygen_t*const pim_noalias task = pBase;
    PtScene*const pim_noalias scene = task->scene;
    const float4 ro = task->origin;

    float4 *const pim_noalias colors = task->colors;
    float4 *const pim_noalias directions = task->directions;

    PtSampler sampler = GetSampler();
    float Xi = f1_mod(begin * kGoldenConj, 1.0f);
    float Yi = f1_mod(begin * kSqrt2Conj, 1.0f);

    for (i32 i = begin; i < end; ++i)
    {
        Xi = f1_wrap(Xi + kGoldenConj);
        Yi = f1_wrap(Yi + kSqrt2Conj);
        float4 rd = SampleUnitSphere(f2_v(Xi, Yi));
        directions[i] = rd;
        PtResult result = Pt_TraceRay(&sampler, scene, ro, rd);
        colors[i] = f3_f4(result.color, 1.0f);
    }

    SetSampler(sampler);
}

ProfileMark(pm_raygen, Pt_RayGen)
PtResults Pt_RayGen(
    PtScene*const pim_noalias scene,
    float4 origin,
    i32 count)
{
    ProfileBegin(pm_raygen);

    ASSERT(scene);
    ASSERT(count >= 0);

    PtScene_Update(scene);

    PtSampler sampler = GetSampler();
    pt_raygen_t *const pim_noalias task = Temp_Calloc(sizeof(*task));
    task->scene = scene;
    task->origin = origin;
    task->colors = Temp_Alloc(sizeof(task->colors[0]) * count);
    task->directions = Temp_Alloc(sizeof(task->directions[0]) * count);
    task->Xi = Sample2D(&sampler);
    SetSampler(sampler);

    Task_Run(task, RayGenFn, count);

    PtResults results =
    {
        .colors = task->colors,
        .directions = task->directions,
    };

    ProfileEnd(pm_raygen);

    return results;
}

pim_inline float VEC_CALL Sample1D(PtSampler*const pim_noalias sampler)
{
    return Prng_f32(&sampler->rng);
}

pim_inline float2 VEC_CALL Sample2D(PtSampler*const pim_noalias sampler)
{
    return f2_rand(&sampler->rng);
}

pim_inline PtSampler VEC_CALL GetSampler(void)
{
    i32 tid = Task_ThreadId();
    return ms_samplers[tid];
}

pim_inline void VEC_CALL SetSampler(PtSampler sampler)
{
    i32 tid = Task_ThreadId();
    ms_samplers[tid] = sampler;
}
