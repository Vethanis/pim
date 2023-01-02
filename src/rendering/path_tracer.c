#include "rendering/path_tracer.h"
#include "rendering/mesh.h"
#include "rendering/camera.h"
#include "rendering/sampler.h"
#include "rendering/drawable.h"
#include "rendering/material.h"
#include "rendering/cubemap.h"
#include "rendering/librtc.h"

#include "math/float2_funcs.h"
#include "math/float4_funcs.h"
#include "math/int2_funcs.h"
#include "math/quat_funcs.h"
#include "math/float4x4_funcs.h"
#include "math/sampling.h"
#include "math/grid.h"
#include "math/dist1d.h"
#include "math/sdf.h"
#include "math/area.h"
#include "math/frustum.h"
#include "math/color.h"
#include "math/lighting.h"
#include "math/atmosphere.h"
#include "math/box.h"
#include "math/markov_sampler.h"

#include "allocator/allocator.h"
#include "threading/task.h"
#include "common/profiler.h"
#include "common/console.h"
#include "common/cvars.h"
#include "common/stringutil.h"
#include "common/serialize.h"
#include "common/atomics.h"
#include "common/time.h"
#include "ui/cimgui_ext.h"

#include "stb/stb_perlin_fork.h"
#include "math/noise.h"
#include <string.h>

pim_optimize;

// ----------------------------------------------------------------------------

typedef struct Cubemap_s Cubemap;
typedef struct Material_s Material;

typedef struct PtSample_s
{
    float3 luminance;
    float3 albedo;
    float3 normal;
    float2 uv;
    float weight;
} PtSample;

typedef struct PtSurfHit_s
{
    float4 P;
    float4 M; // macro normal
    float4 N; // micro normal
    float4 albedo;
    float4 emission;
    float4 meanFreePath;
    float roughness;
    float occlusion;
    float metallic;
    float ior;
    u32 flags; // MatFlag
    PtHitType type;
} PtSurfHit;

typedef struct PtScatter_s
{
    float4 pos;
    float4 dir;
    float4 attenuation;
    float4 luminance;
    float pdf;
} PtScatter;

typedef struct PtLightSample_s
{
    float4 direction;
    float4 luminance;
    float4 wuvt;
    float pdf;
} PtLightSample;

typedef struct PtMediaDesc_s
{
    float4 constantColor;
    float4 noiseColor;
    float constantMfp;
    float noiseMfp;
    float4 constantMu;
    float4 noiseMu;
    float rcpMajorant;
    float absorption;
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
} PtMediaDesc;

typedef struct PtMedia_s
{
    float4 scattering;
    float4 extinction;
    float g0, g1, gBlend;
} PtMedia;

typedef struct RTCSceneTy* RTCScene;

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
    // parameters
    PtMediaDesc mediaDesc;
    u64 modtime;
} PtScene;

typedef struct PtContext_s
{
    Prng rng;
} PtContext;

// ----------------------------------------------------------------------------

static RTCDevice ms_device;
static PtContext ms_contexts[kMaxThreads];

// ----------------------------------------------------------------------------

static void PtScene_Init(PtScene* scene);
static void PtScene_Clear(PtScene* scene);
static void OnRtcError(void* user, RTCError error, const char* msg);
static bool InitRTC(void);
static RTCScene RtcNewScene(const PtScene* pim_noalias scene);
static void FlattenDrawables(PtScene* pim_noalias scene);
static float EmissionPdf(
    const PtScene* pim_noalias scene,
    i32 iVert,
    i32 attempts);
static void CalcEmissionPdfFn(void* pbase, i32 begin, i32 end);
static void SetupEmissives(PtScene* pim_noalias scene);
static void SetupLightGridFn(void* pbase, i32 begin, i32 end);
static void SetupLightGrid(PtScene* pim_noalias scene);

static void media_desc_new(PtMediaDesc* desc);
static void media_desc_update(PtMediaDesc* desc);
static void media_desc_gui(PtMediaDesc* desc);
static void media_desc_load(PtMediaDesc* desc, const char* name);
static void media_desc_save(const PtMediaDesc* desc, const char* name);

// ----------------------------------------------------------------------------

pim_inline RTCRay VEC_CALL RtcNewRay(float4 ro, float4 rd, float tNear, float tFar);
pim_inline RTCRayHit VEC_CALL RtcIntersect(
    RTCScene scene,
    float4 ro,
    float4 rd,
    float tNear,
    float tFar);
pim_inline float4 VEC_CALL GetPosition(
    const PtScene* pim_noalias scene,
    PtRayHit hit);
pim_inline float4 VEC_CALL GetNormal(
    const PtScene* pim_noalias scene,
    PtRayHit hit);
pim_inline float2 VEC_CALL GetUV(
    const PtScene* pim_noalias scene,
    PtRayHit hit);
pim_inline float VEC_CALL GetArea(
    const PtScene* pim_noalias scene,
    i32 iVert);
pim_inline const Material* pim_noalias VEC_CALL GetMaterial(
    const PtScene* pim_noalias scene,
    PtRayHit hit);
pim_inline float4 VEC_CALL GetSky(
    const PtScene* pim_noalias scene,
    float4 ro,
    float4 rd);
pim_inline float4 VEC_CALL GetEmission(
    const PtScene* pim_noalias scene,
    float4 ro,
    float4 rd,
    PtRayHit hit,
    i32 bounce);
pim_inline PtSurfHit VEC_CALL GetSurface(
    const PtScene* pim_noalias scene,
    float4 ro,
    float4 rd,
    PtRayHit hit);
pim_inline PtRayHit VEC_CALL pt_intersect_local(
    const PtScene* pim_noalias scene,
    float4 ro,
    float4 rd,
    float tNear,
    float tFar);

// ----------------------------------------------------------------------------

pim_inline float4 VEC_CALL Eval_Refractive(
    const PtScene* pim_noalias scene,
    const PtSurfHit* pim_noalias surf,
    float4 I,
    float4 L);
pim_inline PtScatter VEC_CALL Scatter_Refractive(
    PtContext* pim_noalias ctx,
    const PtScene* pim_noalias scene,
    const PtSurfHit* pim_noalias surf,
    float4 I);

pim_inline float4 VEC_CALL Eval_Diffuse(
    const PtScene* pim_noalias scene,
    const PtSurfHit* pim_noalias surf,
    float4 I,
    float4 L);
pim_inline PtScatter VEC_CALL Scatter_Diffuse(
    PtContext* pim_noalias ctx,
    const PtScene* pim_noalias scene,
    const PtSurfHit* pim_noalias surf,
    float4 I);

pim_inline float4 VEC_CALL Eval_Specular(
    const PtScene* pim_noalias scene,
    const PtSurfHit* pim_noalias surf,
    float4 I,
    float4 L);
pim_inline PtScatter VEC_CALL Scatter_Specular(
    PtContext* pim_noalias ctx,
    const PtScene* pim_noalias scene,
    const PtSurfHit* pim_noalias surf,
    float4 I);

pim_inline float4 VEC_CALL Eval_Principled(
    const PtScene* pim_noalias scene,
    const PtSurfHit* pim_noalias surf,
    float4 I,
    float4 L);
pim_inline PtScatter VEC_CALL Scatter_Principled(
    PtContext* pim_noalias ctx,
    const PtScene* pim_noalias scene,
    const PtSurfHit* pim_noalias surf,
    float4 I);

// ----------------------------------------------------------------------------

pim_inline bool VEC_CALL LightSelect(
    PtContext* pim_noalias ctx,
    PtScene* pim_noalias scene,
    float4 position,
    i32* iVertOut,
    float* pdfOut);
pim_inline PtLightSample VEC_CALL SampleLight(
    PtContext* pim_noalias ctx,
    PtScene* pim_noalias scene,
    float4 ro,
    i32 iVert,
    i32 bounce);
pim_inline float VEC_CALL LightSelectPdf(
    PtScene* pim_noalias scene,
    i32 iVert,
    float4 ro);
pim_inline float VEC_CALL LightEvalPdf(
    PtScene* pim_noalias scene,
    float4 ro,
    float4 rd,
    PtRayHit *const pim_noalias hitOut);

// ----------------------------------------------------------------------------

pim_inline float4 VEC_CALL EstimateDirect(
    PtContext* pim_noalias ctx,
    PtScene* pim_noalias scene,
    const PtSurfHit* pim_noalias surf,
    const PtRayHit* pim_noalias srcHit,
    float4 I,
    i32 bounce);
pim_inline bool VEC_CALL EvaluateLight(
    PtContext* pim_noalias ctx,
    PtScene* pim_noalias scene,
    float4 P,
    float4* pim_noalias lightOut,
    float4* pim_noalias dirOut,
    i32 bounce);

// ----------------------------------------------------------------------------

pim_inline float4 VEC_CALL MeanFreePathToMu(float4 albedo);
pim_inline PtMedia VEC_CALL Media_Sample(const PtMediaDesc* pim_noalias desc, float4 P);
pim_inline float4 VEC_CALL CalcMajorant(const PtMediaDesc* pim_noalias desc);
pim_inline float VEC_CALL CalcPhase(
    PtMedia media,
    float cosTheta);
pim_inline float4 VEC_CALL SamplePhaseDir(
    PtContext* pim_noalias ctx,
    PtScene* pim_noalias scene,
    PtMedia media,
    float4 rd);
pim_inline float4 VEC_CALL CalcTransmittance(
    PtContext* pim_noalias ctx,
    PtScene* pim_noalias scene,
    float4 ro,
    float4 rd,
    float rayLen);
pim_inline PtScatter VEC_CALL ScatterRay(
    PtContext* pim_noalias ctx,
    PtScene* pim_noalias scene,
    float4 ro,
    float4 rd,
    float rayLen,
    i32 bounce);

// ----------------------------------------------------------------------------

pim_inline void VEC_CALL LightOnHit(
    PtScene* pim_noalias scene,
    float4 ro,
    float4 lum,
    i32 iVert);
static void UpdateDists(PtScene* pim_noalias scene);

// ----------------------------------------------------------------------------

pim_inline PtContext* VEC_CALL PtContext_Get(void)
{
    return &ms_contexts[Task_ThreadId()];
}

pim_inline float VEC_CALL Sample1D(PtContext* pim_noalias ctx)
{
    return Prng_f32(&ctx->rng);
}

pim_inline float2 VEC_CALL Sample2D(PtContext* pim_noalias ctx)
{
    return Prng_float2(&ctx->rng);
}

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

static void PtContext_New(PtContext* ctx)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->rng.state = Prng_uint4(Prng_Get());
}

static void PtContext_Del(PtContext* ctx)
{
    memset(ctx, 0, sizeof(*ctx));
}

void PtSys_Init(void)
{
    InitRTC();
    for (i32 i = 0; i < NELEM(ms_contexts); ++i)
    {
        PtContext_New(&ms_contexts[i]);
    }
}

void PtSys_Update(void)
{

}

void PtSys_Shutdown(void)
{
    for (i32 i = 0; i < NELEM(ms_contexts); ++i)
    {
        PtContext_Del(&ms_contexts[i]);
    }
    if (ms_device)
    {
        rtc.ReleaseDevice(ms_device);
        ms_device = NULL;
    }
}

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
    const float4* pim_noalias ros,
    const float4* pim_noalias rds)
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

typedef struct PointQueryUserData
{
    pim_alignas(16)
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

pim_inline PointQueryUserData VEC_CALL RtcPointQuery(
    const PtScene* pim_noalias scene,
    float4 pt)
{
    PointQueryUserData usr = { 0 };
    usr.positions = scene->positions;
    usr.distance = kRcpEpsilon;
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

static RTCScene RtcNewScene(const PtScene* pim_noalias scene)
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
    ASSERT((vertCount % 3) == 0);

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

static void FlattenDrawables(
    PtScene* pim_noalias scene)
{
    const Entities* drawTable = Entities_Get();
    const i32 drawCount = drawTable->count;
    const MeshId* pim_noalias meshes = drawTable->meshes;
    const float4x4* pim_noalias matrices = drawTable->matrices;
    const Material* pim_noalias materials = drawTable->materials;

    i32 vertCap = 0;
    i32 matCap = 0;
    for (i32 i = 0; i < drawCount; ++i)
    {
        const Mesh* pim_noalias mesh = Mesh_Get(meshes[i]);
        if (!mesh)
        {
            continue;
        }
        vertCap += mesh->length;
        ++matCap;
    }

    i32 vertCount = 0;
    float4* positions = Perm_Calloc(sizeof(positions[0]) * vertCap);
    float4* normals = Perm_Calloc(sizeof(normals[0]) * vertCap);
    float2* uvs = Perm_Calloc(sizeof(uvs[0]) * vertCap);
    i32* matIds = Perm_Calloc(sizeof(matIds[0]) * vertCap);

    i32 matCount = 0;
    Material* sceneMats = Perm_Calloc(sizeof(sceneMats[0]) * matCap);

    for (i32 i = 0; i < drawCount; ++i)
    {
        const Mesh* pim_noalias mesh = Mesh_Get(meshes[i]);
        if (!mesh)
        {
            continue;
        }

        const i32 meshLen = mesh->length;
        const float4* pim_noalias meshPositions = mesh->positions;
        const float4* pim_noalias meshNormals = mesh->normals;
        const float4* pim_noalias meshUvs = mesh->uvs;

        const i32 vertBack = vertCount;
        const i32 matBack = matCount;
        vertCount += meshLen;
        matCount += 1;

        const float4x4 M = matrices[i];
        const float3x3 IM = f3x3_IM(M);
        sceneMats[matBack] = materials[i];

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
    const PtScene* pim_noalias scene,
    i32 iVert,
    i32 attempts)
{
    const i32 iMat = scene->matIds[iVert];
    const Material* mat = scene->materials + iMat;

    if (mat->flags & MatFlag_Sky)
    {
        return 1.0f;
    }

    const Texture* pim_noalias romeMap = Texture_Get(mat->rome);
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

    Prng* pim_noalias rng = Prng_Get();
    i32 hits = 0;
    for (i32 i = 0; i < attempts; ++i)
    {
        float4 wuv = SampleBaryCoord(Prng_float2(rng));
        float2 uv = f2_blend(UA, UB, UC, wuv);
        i32 iTexel = UvWrapPow2(texSize, uv);
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

static void CalcEmissionPdfFn(void* pbase, i32 begin, i32 end)
{
    task_CalcEmissionPdf* task = (task_CalcEmissionPdf*)pbase;
    const PtScene*const pim_noalias scene = task->scene;
    const i32 attempts = task->attempts;
    float* pim_noalias pdfs = task->pdfs;

    for (i32 i = begin; i < end; ++i)
    {
        pdfs[i] = EmissionPdf(scene, i * 3, attempts);
    }
}

static void SetupEmissives(PtScene* pim_noalias scene)
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

typedef struct task_SetupLightGrid
{
    Task task;
    PtScene* scene;
} task_SetupLightGrid;

static void SetupLightGridFn(void* pbase, i32 begin, i32 end)
{
    task_SetupLightGrid* task = (task_SetupLightGrid*)pbase;

    PtScene*const pim_noalias scene = task->scene;

    const Grid grid = scene->lightGrid;
    Dist1D *const pim_noalias dists = scene->lightDists;

    float4 const *const pim_noalias positions = scene->positions;

    const i32 emissiveCount = scene->emissiveCount;
    i32 const *const pim_noalias emitToVert = scene->emitToVert;

    const float metersPerCell = ConVar_GetFloat(&cv_pt_dist_meters);
    const float radius = metersPerCell * 0.666f;
    Prng* pim_noalias rng = Prng_Get();

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
                    PtRayHit hit = pt_intersect_local(scene, position, hamm[j], 0.0f, kRcpEpsilon);
                    if (hit.type == PtHit_Triangle)
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
                    float4 t = f4_mulvs(f4_lerpsv(-1.5f, 1.5f, Prng_float4(rng)), radius);
                    float4 ro = f4_add(position, t);
                    ro.w = 0.0f;
                    float4 at = f4_blend(A, B, C, SampleBaryCoord(Prng_float2(rng)));
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
}

static void SetupLightGrid(PtScene* pim_noalias scene)
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
void PtScene_Update(PtScene* scene)
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

void PtScene_Del(PtScene* scene)
{
    if (scene)
    {
        PtScene_Clear(scene);
        Mem_Free(scene);
    }
}

void PtScene_Gui(PtScene* scene)
{
    if (scene && igExCollapsingHeader1("PtScene"))
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
    int2 imageSize)
{
    memset(trace, 0, sizeof(*trace));
    const i32 texelCount = imageSize.x * imageSize.y;
    ASSERT(texelCount > 0);
    trace->imageSize = imageSize;
    trace->sampleWeight = 1.0f;
    trace->color = Tex_Calloc(sizeof(trace->color[0]) * texelCount);
    trace->albedo = Tex_Calloc(sizeof(trace->albedo[0]) * texelCount);
    trace->normal = Tex_Calloc(sizeof(trace->normal[0]) * texelCount);
    trace->denoised = Tex_Calloc(sizeof(trace->denoised[0]) * texelCount);
}

void PtTrace_Del(PtTrace* trace)
{
    Mem_Free(trace->color);
    Mem_Free(trace->albedo);
    Mem_Free(trace->normal);
    Mem_Free(trace->denoised);
    memset(trace, 0, sizeof(*trace));
}

void DofInfo_New(PtDofInfo* dof)
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

void DofInfo_Gui(PtDofInfo* dof)
{
    if (dof && igExCollapsingHeader1("PtDofInfo"))
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
    const PtScene* pim_noalias scene,
    PtRayHit hit)
{
    float4 const *const pim_noalias positions = scene->positions;
    return f4_blend(
        positions[hit.iVert + 0],
        positions[hit.iVert + 1],
        positions[hit.iVert + 2],
        hit.wuvt);
}

pim_inline float4 VEC_CALL GetNormal(
    const PtScene* pim_noalias scene,
    PtRayHit hit)
{
    float4 const *const pim_noalias normals = scene->normals;
    float4 N = f4_blend(
        normals[hit.iVert + 0],
        normals[hit.iVert + 1],
        normals[hit.iVert + 2],
        hit.wuvt);
    N = (f4_dot3(hit.normal, N) > 0.0f) ? N : f4_neg(N);
    return f4_normalize3(N);
}

pim_inline float2 VEC_CALL GetUV(
    const PtScene* pim_noalias scene,
    PtRayHit hit)
{
    float2 const *const pim_noalias uvs = scene->uvs;
    return f2_blend(
        uvs[hit.iVert + 0],
        uvs[hit.iVert + 1],
        uvs[hit.iVert + 2],
        hit.wuvt);
}

pim_inline float VEC_CALL GetArea(
    const PtScene* pim_noalias scene,
    i32 iVert)
{
    float4 const *const pim_noalias positions = scene->positions;
    ASSERT(iVert >= 0);
    ASSERT((iVert + 2) < scene->vertCount);
    return TriArea3D(positions[iVert + 0], positions[iVert + 1], positions[iVert + 2]);
}

pim_inline const Material* pim_noalias VEC_CALL GetMaterial(
    const PtScene* pim_noalias scene,
    PtRayHit hit)
{
    i32 iVert = hit.iVert;
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

pim_inline float4 VEC_CALL SampleSkyTex(
    const Texture* pim_noalias tex,
    float2 uv)
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

pim_inline float4 VEC_CALL TriplaneSampleSkyTex(
    const Texture* pim_noalias tex,
    float4 dir,
    float4 pos)
{
    float4 b = TriplaneBlending(dir);
    float4 x = f4_mulvs(SampleSkyTex(tex, f2_v(pos.z, pos.y)), b.x);
    float4 y = f4_mulvs(SampleSkyTex(tex, f2_v(pos.x, pos.z)), b.y);
    float4 z = f4_mulvs(SampleSkyTex(tex, f2_v(pos.x, pos.y)), b.z);
    return f4_add(x, f4_add(y, z));
}

pim_inline float4 VEC_CALL GetSky(
    const PtScene* pim_noalias scene,
    float4 ro,
    float4 rd)
{
    const Cubemap* pim_noalias sky = scene->sky;
    if (sky)
    {
        return f3_f4(Cubemap_ReadColor(sky, rd), 1.0f);
    }
    return f4_0;
}

pim_inline float4 VEC_CALL GetEmission(
    const PtScene* pim_noalias scene,
    float4 ro,
    float4 rd,
    PtRayHit hit,
    i32 bounce)
{
    if (hit.flags & MatFlag_Sky)
    {
        return GetSky(scene, ro, rd);
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
                albedo = UvBilinearWrapPow2_c32(tex->texels, tex->size, uv);
            }
        }
        float e = 0.0f;
        {
            Texture const *const tex = Texture_Get(mat->rome);
            if (tex)
            {
                e = UvBilinearWrapPow2_c32(tex->texels, tex->size, uv).w;
            }
        }
        return UnpackEmission(albedo, e);
    }
}

pim_inline float4 VEC_CALL SampleAlbedo(
    const Material* pim_noalias mat,
    float2 uv)
{
    float4 value = f4_1;
    Texture const *const pim_noalias tex = Texture_Get(mat->albedo);
    if (tex)
    {
        value = UvBilinearWrapPow2_c32(tex->texels, tex->size, uv);
    }
    return value;
}

pim_inline float4 VEC_CALL SampleRome(
    const Material* pim_noalias mat,
    float2 uv)
{
    float4 value = f4_v(0.5f, 1.0f, 0.0f, 0.0f);
    Texture const *const pim_noalias tex = Texture_Get(mat->rome);
    if (tex)
    {
        value = UvBilinearWrapPow2_c32(tex->texels, tex->size, uv);
    }
    return value;
}

// M: geometry normal
// N: shading normal
pim_inline float4 VEC_CALL FixShadingNormal(float4 M, float4 N)
{
    N = (f4_dot3(M, N) > 0.0f) ? N : f4_reflect3(N, M);
    ASSERT(IsUnitLength(N));
    return N;
}

pim_inline float4 VEC_CALL SampleNormal(
    const Material* pim_noalias mat,
    float2 uv,
    float4 N)
{
    Texture const *const pim_noalias tex = Texture_Get(mat->normal);
    if (tex)
    {
        float4 Nts = UvBilinearWrapPow2_xy16(tex->texels, tex->size, uv);
        N = FixShadingNormal(N, TanToWorld(N, Nts));
    }
    return N;
}

pim_inline PtSurfHit VEC_CALL GetSurface(
    const PtScene* pim_noalias scene,
    float4 ro,
    float4 rd,
    PtRayHit hit)
{
    PtSurfHit surf;
    surf.type = hit.type;

    Material const *const pim_noalias mat = GetMaterial(scene, hit);
    surf.flags = mat->flags;
    surf.ior = mat->ior;
    float2 uv = GetUV(scene, hit);
    surf.M = GetNormal(scene, hit);
    surf.N = surf.M;
    surf.P = GetPosition(scene, hit);
    surf.P = f4_add(surf.P, f4_mulvs(surf.M, 0.01f * kMilli));
    surf.meanFreePath = mat->meanFreePath;

    if (hit.flags & MatFlag_Sky)
    {
        surf.albedo = f4_0;
        surf.roughness = 1.0f;
        surf.occlusion = 0.0f;
        surf.metallic = 0.0f;
        surf.emission = GetSky(scene, ro, rd);
        surf.M = f4_neg(rd);
        surf.N = surf.M;
        surf.ior = 1.0f;
    }
    else
    {
        surf.N = SampleNormal(mat, uv, surf.N);
        surf.albedo = SampleAlbedo(mat, uv);
        float4 rome = SampleRome(mat, uv);
        surf.emission = UnpackEmission(surf.albedo, rome.w);
        surf.roughness = rome.x;
        surf.occlusion = rome.y;
        surf.metallic = rome.z;
    }

    return surf;
}

pim_inline PtRayHit VEC_CALL pt_intersect_local(
    const PtScene* pim_noalias scene,
    float4 ro,
    float4 rd,
    float tNear,
    float tFar)
{
    PtRayHit hit = { 0 };
    hit.wuvt.w = -1.0f;
    hit.iVert = -1;

    RTCRayHit rtcHit = RtcIntersect(scene->rtcScene, ro, rd, tNear, tFar);
    hit.normal = f4_v(rtcHit.hit.Ng_x, rtcHit.hit.Ng_y, rtcHit.hit.Ng_z, 0.0f);
    bool hitNothing =
        (rtcHit.hit.geomID == RTC_INVALID_GEOMETRY_ID) ||
        (rtcHit.ray.tfar <= 0.0f);
    if (hitNothing)
    {
        hit.type = PtHit_Nothing;
        return hit;
    }
    hit.type = PtHit_Triangle;
    if (f4_dot3(hit.normal, rd) > 0.0f)
    {
        hit.normal = f4_neg(hit.normal);
        hit.type = PtHit_Backface;
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

    hit.iVert = iVert;
    hit.wuvt = f4_v(w, u, v, t);
    hit.flags = GetMaterial(scene, hit)->flags;

    return hit;
}

PtRayHit VEC_CALL Pt_Intersect(
    const PtScene* pim_noalias scene,
    float4 ro,
    float4 rd,
    float tNear,
    float tFar)
{
    return pt_intersect_local(scene, ro, rd, tNear, tFar);
}

pim_inline float4 VEC_CALL Eval_Diffuse(
    const PtScene* pim_noalias scene,
    const PtSurfHit* pim_noalias surf,
    float4 I,
    float4 L)
{
    float4 result = f4_0;
    float4 N = surf->N;
    float NoL = f4_dot3(N, L);
    float pdf = ImportanceSampleLambertPdf(NoL);
    if (pdf > kEpsilon)
    {
        float4 V = f4_neg(I);
        float4 H = f4_normalize3(f4_add(V, L));
        float HoV = f4_dotsat(H, V);
        float NoV = f4_dotsat(N, V);
        float4 Fd = f4_mulvs(surf->albedo, Fd_Burley(NoL, NoV, HoV, surf->roughness));
        result = f4_mulvs(Fd, NoL);
        result.w = pdf;
    }
    return result;
}

pim_inline PtScatter VEC_CALL Scatter_Diffuse(
    PtContext* pim_noalias ctx,
    const PtScene* pim_noalias scene,
    const PtSurfHit* pim_noalias surf,
    float4 I)
{
    PtScatter result = { 0 };
    float4 N = surf->N;
    float4 L = TanToWorld(N, SampleCosineHemisphere(Sample2D(ctx)));
    float4 attenPdf = Eval_Diffuse(scene, surf, I, L);
    result.dir = L;
    result.attenuation = attenPdf;
    result.pdf = attenPdf.w;
    result.pos = surf->P;
    return result;
}

pim_inline float4 VEC_CALL Eval_Specular(
    const PtScene* pim_noalias scene,
    const PtSurfHit* pim_noalias surf,
    float4 I,
    float4 L)
{
    float4 result = f4_0;
    float4 N = surf->N;
    float NoL = f4_dot3(N, L);
    if (NoL > kEpsilon)
    {
        float alpha = BrdfAlpha(surf->roughness);
        float4 V = f4_neg(I);
        float4 H = f4_normalize3(f4_add(V, L));
        float NoH = f4_dot3(N, H);
        float HoV = f4_dot3(H, V);
        float pdf = GGXPdf(NoH, HoV, alpha);
        if (NoH > kEpsilon && HoV > kEpsilon && pdf > kEpsilon)
        {
            float NoV = f4_dotsat(N, V);
            float f = F_Dielectric(HoV, 1.0f, 1.5f);
            float4 f0 = F_0(surf->albedo, surf->metallic);
            float4 F = f4_lerpvs(f0, f4_s(F_90(f0)), f);
            float D = D_GTR(NoH, alpha);
            float G = V_SmithCorrelated(NoL, NoV, alpha);
            float4 Fr = f4_mulvs(F, D * G);
            Fr = f4_mul(Fr, GGXEnergyCompensation(f0, NoV, alpha));
            result = f4_mulvs(Fr, NoL);
            result.w = pdf;
        }
    }
    return result;
}

pim_inline PtScatter VEC_CALL Scatter_Specular(
    PtContext* pim_noalias ctx,
    const PtScene* pim_noalias scene,
    const PtSurfHit* pim_noalias surf,
    float4 I)
{
    PtScatter result = { 0 };
    float4 m = SampleGGXMicrofacet(Sample2D(ctx), BrdfAlpha(surf->roughness));
    m = TanToWorld(surf->N, m);
    m = FixShadingNormal(surf->M, m);
    result.dir = f4_reflect3(I, m);
    result.attenuation = Eval_Specular(scene, surf, I, result.dir);
    result.pdf = result.attenuation.w;
    result.pos = surf->P;
    return result;
}

pim_inline float4 VEC_CALL Eval_Refractive(
    const PtScene* pim_noalias scene,
    const PtSurfHit* pim_noalias surf,
    float4 I,
    float4 L)
{
    return f4_0;
}

pim_inline PtScatter VEC_CALL Scatter_Refractive(
    PtContext* pim_noalias ctx,
    const PtScene* pim_noalias scene,
    const PtSurfHit* pim_noalias surf,
    float4 I)
{
    const float etaI = 1.000277f;
    const float etaT = f1_max(1.0f, surf->ior);
    const float alpha = BrdfAlpha(surf->roughness);

    float4 V = f4_neg(I);
    float4 M = surf->M;
    float4 m = TanToWorld(surf->N, SampleGGXMicrofacet(Sample2D(ctx), alpha));
    m = FixShadingNormal(M, m);
    bool entering = surf->type != PtHit_Backface;

    float cosThetaI = f1_sat(f1_abs(f4_dot3(V, m)));
    float pdf = F_Dielectric(entering ? cosThetaI : -cosThetaI, etaI, etaT);

    float4 L;
    bool refract = false;
    if (Sample1D(ctx) < pdf)
    {
        L = f4_reflect3(I, m);
        ASSERT(IsUnitLength(L));
    }
    else
    {
        float k = entering ? etaI / etaT : etaT / etaI;
        L = f4_refract3(I, m, k);
        pdf = 1.0f - pdf;
        refract = true;
    }

    float4 P = surf->P;
    if (f4_dot3(L, M) < 0.0f)
    {
        P = f4_sub(P, f4_mulvs(M, kMilli * 0.1f));
    }

    PtScatter result = { 0 };
    result.pos = P;
    result.dir = L;
    result.pdf = pdf;

    if (refract && entering)
    {
        PtRayHit hit = pt_intersect_local(scene, P, L, 0.0f, kRcpEpsilon);
        float thickness = f1_max(hit.wuvt.w, kEpsilon);
        if (hit.type == PtHit_Nothing)
        {
            thickness = kRcpEpsilon;
        }
        float4 tr = AlbedoToTransmittance(surf->albedo, surf->roughness, thickness);
        result.attenuation = f4_mulvs(tr, pdf);
    }
    else
    {
        result.attenuation = f4_s(pdf);
    }

    return result;
}

// returns attenuation value for the given interaction
pim_inline float4 VEC_CALL Eval_Principled(
    const PtScene* pim_noalias scene,
    const PtSurfHit* pim_noalias surf,
    float4 I,
    float4 L)
{
    ASSERT(IsUnitLength(I));
    ASSERT(IsUnitLength(L));
    if (surf->flags & MatFlag_Refractive)
    {
        return Eval_Refractive(scene, surf, I, L);
    }
    ASSERT(IsUnitLength(surf->N));
    if (!(f4_dot3(surf->N, L) > kEpsilon))
    {
        return f4_0;
    }
    float amtA = f1_lerp(0.5f, 1.0f, surf->metallic);
    float amtB = 1.0f - amtA;
    float4 evalA = Eval_Specular(scene, surf, I, L);
    float4 evalB = f4_0;
    if (amtB > kEpsilon)
    {
        evalB = Eval_Diffuse(scene, surf, I, L);
        evalA = f4_lerpvs(evalA, evalB, amtB);
    }
    return evalA;
}

pim_inline PtScatter VEC_CALL Scatter_Principled(
    PtContext* pim_noalias ctx,
    const PtScene* pim_noalias scene,
    const PtSurfHit* pim_noalias surf,
    float4 I)
{
    ASSERT(IsUnitLength(I));
    ASSERT(IsUnitLength(surf->N));
    if (surf->flags & MatFlag_Refractive)
    {
        return Scatter_Refractive(ctx, scene, surf, I);
    }

    PtScatter scatter = { 0 };
    float4 eval = f4_0;

    float amtSpec = f1_lerp(0.5f, 1.0f, surf->metallic);
    float amtDiffuse = 1.0f - amtSpec;
    if (Sample1D(ctx) < amtSpec)
    {
        scatter = Scatter_Specular(ctx, scene, surf, I);
        if (amtDiffuse > kEpsilon)
        {
            eval = Eval_Diffuse(scene, surf, I, scatter.dir);
            scatter.attenuation = f4_lerpvs(scatter.attenuation, eval, amtDiffuse);
            scatter.pdf = f1_lerp(scatter.pdf, eval.w, amtDiffuse);
        }
    }
    else
    {
        scatter = Scatter_Diffuse(ctx, scene, surf, I);
        eval = Eval_Specular(scene, surf, I, scatter.dir);
        scatter.attenuation = f4_lerpvs(scatter.attenuation, eval, amtSpec);
        scatter.pdf = f1_lerp(scatter.pdf, eval.w, amtSpec);
    }

    return scatter;
}

pim_inline void VEC_CALL LightOnHit(
    PtScene* pim_noalias scene,
    float4 ro,
    float4 lum4,
    i32 iVert)
{
    float lum = f4_avglum(lum4);
    if (lum <= kEpsilon)
    {
        return;
    }
    float loglum = log2f(lum) - kLog2Epsilon;
    loglum = f1_clamp(loglum, 0.0f, 46.0f);
    u32 amt = (u32)(loglum * (0xff/46.0f) + 0.5f);
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
    PtContext* pim_noalias ctx,
    PtScene* pim_noalias scene,
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

    i32 iEmit = Dist1D_SampleD(dist, Sample1D(ctx));
    float pdf = Dist1D_PdfD(dist, iEmit);

    i32 iVert = scene->emitToVert[iEmit];

    *iVertOut = iVert;
    *pdfOut = pdf;
    return pdf > kEpsilon;
}

pim_inline float VEC_CALL LightSelectPdf(
    PtScene* pim_noalias scene,
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

pim_inline PtLightSample VEC_CALL SampleLight(
    PtContext* pim_noalias ctx,
    PtScene* pim_noalias scene,
    float4 ro,
    i32 iVert,
    i32 bounce)
{
    PtLightSample sample = { 0 };

    float4 wuv = SampleBaryCoord(Sample2D(ctx));

    const float4* pim_noalias positions = scene->positions;
    float4 A = positions[iVert + 0];
    float4 B = positions[iVert + 1];
    float4 C = positions[iVert + 2];
    float4 pt = f4_blend(A, B, C, wuv);
    float area = TriArea3D(A, B, C);

    float4 rd = f4_sub(pt, ro);
    float distSq = f4_dot3(rd, rd);
    float distance = sqrtf(f1_max(distSq, kEpsilonSq));
    wuv.w = distance;
    rd = f4_divvs(rd, distance);

    sample.direction = rd;
    sample.wuvt = wuv;

    PtRayHit hit = pt_intersect_local(scene, ro, rd, 0.0f, distance + 0.01f * kMilli);
    if ((hit.type != PtHit_Nothing) && (hit.iVert == iVert))
    {
        float cosTheta = f1_abs(f4_dot3(rd, hit.normal));
        sample.pdf = LightPdf(area, cosTheta, distSq);
        sample.luminance = GetEmission(scene, ro, rd, hit, bounce);
        if (f4_sum3(sample.luminance) > kEpsilon)
        {
            float4 Tr = CalcTransmittance(ctx, scene, ro, rd, hit.wuvt.w);
            sample.luminance = f4_mul(sample.luminance, Tr);
        }
    }

    ASSERT(f4_hmin3(sample.luminance) >= 0.0f);
    return sample;
}

pim_inline float VEC_CALL LightEvalPdf(
    PtScene* pim_noalias scene,
    float4 ro,
    float4 rd,
    PtRayHit *const pim_noalias hitOut)
{
    ASSERT(IsUnitLength(rd));
    PtRayHit hit = pt_intersect_local(scene, ro, rd, 0.0f, kRcpEpsilon);
    float pdf = 0.0f;
    if (hit.type != PtHit_Nothing)
    {
        float cosTheta = f1_abs(f4_dot3(rd, hit.normal));
        float area = GetArea(scene, hit.iVert);
        float distSq = f1_max(kEpsilon, hit.wuvt.w * hit.wuvt.w);
        pdf = LightPdf(area, cosTheta, distSq);
    }
    *hitOut = hit;
    return pdf;
}

pim_inline float4 VEC_CALL EstimateDirect(
    PtContext* pim_noalias ctx,
    PtScene* pim_noalias scene,
    const PtSurfHit* pim_noalias surf,
    const PtRayHit* pim_noalias srcHit,
    float4 I,
    i32 bounce)
{
    float4 result = f4_0;
    if (surf->flags & MatFlag_Refractive)
    {
        return result;
    }

    const float4 ro = surf->P;
    ASSERT(surf->roughness >= 0.0f);
    ASSERT(surf->roughness <= 1.0f);
    const float pRough = f1_lerp(0.05f, 0.95f, surf->roughness);
    const float pSmooth = 1.0f - pRough;
    if (Sample1D(ctx) < pRough)
    {
        i32 iVert;
        float selectPdf;
        if (LightSelect(ctx, scene, surf->P, &iVert, &selectPdf))
        {
            if (srcHit->iVert != iVert)
            {
                // already has CalcTransmittance applied
                PtLightSample sample = SampleLight(ctx, scene, ro, iVert, bounce);
                float4 rd = sample.direction;
                float4 Li = sample.luminance;
                float lightPdf = sample.pdf * selectPdf * pRough;
                if ((lightPdf > kEpsilon) && (f4_hmax3(Li) > kEpsilon))
                {
                    float4 brdf = Eval_Principled(scene, surf, I, rd);
                    Li = f4_mul(Li, brdf);
                    float brdfPdf = brdf.w * pSmooth;
                    if (brdfPdf > kEpsilon)
                    {
                        Li = f4_mulvs(Li, PowerHeuristic(lightPdf, brdfPdf) / lightPdf);
                        result = f4_add(result, Li);
                    }
                }
            }
        }
    }
    else
    {
        PtScatter sample = Scatter_Principled(ctx, scene, surf, I);
        float4 rd = sample.dir;
        float brdfPdf = sample.pdf * pSmooth;
        if (brdfPdf > kEpsilon)
        {
            PtRayHit hit;
            float lightPdf = LightEvalPdf(scene, ro, rd, &hit) * pRough;
            if (lightPdf > kEpsilon)
            {
                lightPdf *= LightSelectPdf(scene, hit.iVert, ro);
                float4 Li = f4_mul(GetEmission(scene, ro, rd, hit, bounce), sample.attenuation);
                if (f4_hmax3(Li) > kEpsilon)
                {
                    Li = f4_mulvs(Li, PowerHeuristic(brdfPdf, lightPdf) / brdfPdf);
                    Li = f4_mul(Li, CalcTransmittance(ctx, scene, ro, rd, hit.wuvt.w));
                    result = f4_add(result, Li);
                }
            }
        }
    }

    return result;
}

pim_inline bool VEC_CALL EvaluateLight(
    PtContext* pim_noalias ctx,
    PtScene* pim_noalias scene,
    float4 P,
    float4* pim_noalias lightOut,
    float4* pim_noalias dirOut,
    i32 bounce)
{
    i32 iVert;
    float selectPdf;
    if (LightSelect(ctx, scene, P, &iVert, &selectPdf))
    {
        PtLightSample sample = SampleLight(ctx, scene, P, iVert, bounce);
        if (sample.pdf > kEpsilon)
        {
            *lightOut = f4_divvs(sample.luminance, sample.pdf * selectPdf);
            *dirOut = sample.direction;
            return true;
        }
    }
    return false;
}

static void media_desc_new(PtMediaDesc* desc)
{
    desc->constantColor = f4_v(0.5f, 0.5f, 0.5f, 2.0f);
    desc->noiseColor = f4_v(0.5f, 0.5f, 0.5f, 2.0f);
    desc->constantMfp = 40.0f * kKilo;
    desc->noiseMfp = 40.0f * kKilo;
    desc->absorption = 0.1f;
    desc->noiseOctaves = 1;
    desc->noiseGain = 0.9f;
    desc->noiseLacunarity = 2.0666f;
    desc->noiseFreq = 1.0f;
    desc->noiseHeight = 20.0f;
    desc->noiseScale = 1.0f;
    desc->phaseDirA = 0.0f;
    desc->phaseDirB = 0.0f;
    desc->phaseBlend = 0.5f;
    media_desc_update(desc);
}

static void media_desc_update(PtMediaDesc* desc)
{
    float4 constantMfp = f4_lerpsv(desc->constantMfp * 0.5f, desc->constantMfp * 2.0f, desc->constantColor);
    float4 noiseMfp = f4_lerpsv(desc->noiseMfp * 0.5f, desc->noiseMfp * 2.0f, desc->noiseColor);
    desc->constantMu = MeanFreePathToMu(constantMfp);
    desc->noiseMu = MeanFreePathToMu(noiseMfp);
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

static void media_desc_load(PtMediaDesc* desc, const char* name)
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
        Ser_LoadVector(desc, root, constantColor);
        Ser_LoadVector(desc, root, noiseColor);
        Ser_LoadFloat(desc, root, constantMfp);
        Ser_LoadFloat(desc, root, noiseMfp);
        Ser_LoadFloat(desc, root, absorption);
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

static void media_desc_save(const PtMediaDesc* desc, const char* name)
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
        Ser_SaveVector(desc, root, constantColor);
        Ser_SaveVector(desc, root, noiseColor);
        Ser_SaveFloat(desc, root, constantMfp);
        Ser_SaveFloat(desc, root, noiseMfp);
        Ser_SaveFloat(desc, root, absorption);
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

static void media_desc_gui(PtMediaDesc* desc)
{
    const u32 ldrPicker =
        ImGuiColorEditFlags_Float |
        ImGuiColorEditFlags_AlphaBar |
        ImGuiColorEditFlags_AlphaPreviewHalf |
        ImGuiColorEditFlags_PickerHueWheel |
        ImGuiColorEditFlags_InputRGB |
        ImGuiColorEditFlags_DisplayRGB;
    const u32 slider =
        ImGuiSliderFlags_NoRoundToFormat |
        ImGuiSliderFlags_AlwaysClamp;
    const u32 logSlider =
        slider |
        ImGuiSliderFlags_Logarithmic;

    if (desc && igExCollapsingHeader1("MediaDesc"))
    {
        igIndent(0.0f);
        static char name[PIM_PATH];
        igInputText("Preset Name", name, sizeof(name), 0, NULL, NULL);
        if (igExButton("Load Preset"))
        {
            media_desc_load(desc, name);
        }
        if (igExButton("Save Preset"))
        {
            media_desc_save(desc, name);
        }

        const float kMinMfp = 0.1f;
        const float kMaxMfp = 40.0f * kKilo;
        const float kMinG = -0.99f;
        const float kMaxG = 0.99f;

        igExSliderFloat("Phase Dir A", &desc->phaseDirA, kMinG, kMaxG);
        igExSliderFloat("Phase Dir B", &desc->phaseDirB, kMinG, kMaxG);
        igExSliderFloat("Phase Blend", &desc->phaseBlend, 0.0f, 1.0f);

        igSliderFloat(
            "Absorption",
            &desc->absorption,
            0.01f, 10.0f, "%.3f",
            logSlider);

        igColorEdit3(
            "Const Color",
            &desc->constantColor.x,
            ldrPicker);
        igSliderFloat(
            "Const Mean Free Path",
            &desc->constantMfp,
            kMinMfp, kMaxMfp,
            "%.3f",
            logSlider);

        igColorEdit3(
            "Noise Color",
            &desc->noiseColor.x,
            ldrPicker);
        igSliderFloat(
            "Noise Mean Free Path",
            &desc->noiseMfp,
            kMinMfp, kMaxMfp,
            "%.3f",
            logSlider);

        igExSliderInt("Noise Octaves", &desc->noiseOctaves, 1, 10);
        igExSliderFloat("Noise Gain", &desc->noiseGain, 0.0f, 1.0f);
        igExSliderFloat("Noise Lacunarity", &desc->noiseLacunarity, 1.0f, 3.0f);
        igSliderFloat(
            "Noise Frequency",
            &desc->noiseFreq,
            0.1f, 10.0f, "%.3f",
            logSlider);
        igExSliderFloat("Noise Height", &desc->noiseHeight, -20.0f, 20.0f);
        igSliderFloat(
            "Noise Scale",
            &desc->noiseScale,
            0.1f, 10.0f, "%.3f",
            logSlider);

        igUnindent(0.0f);

        media_desc_update(desc);
    }
}

pim_inline PtMedia VEC_CALL Media_Sample(
    const PtMediaDesc* pim_noalias desc,
    float4 P)
{
    PtMedia hit;
    hit.scattering = desc->constantMu;
    hit.g0 = desc->phaseDirA;
    hit.g1 = desc->phaseDirB;
    hit.gBlend = desc->phaseBlend;

    if (f1_distance(P.y, desc->noiseHeight) <= desc->noiseRange)
    {
        float noiseFreq = desc->noiseFreq;
        //float noise = stb_perlinf_fbm_noise3(
        //    P.x * noiseFreq,
        //    P.y * noiseFreq,
        //    P.z * noiseFreq,
        //    desc->noiseLacunarity,
        //    desc->noiseGain,
        //    desc->noiseOctaves);
        float noise = FbmGradientNoise3(
            f4_mulvs(P, noiseFreq),
            desc->noiseLacunarity,
            desc->noiseGain,
            desc->noiseOctaves,
            1);
        float noiseScale = desc->noiseScale;
        float height = desc->noiseHeight + noiseScale * noise;
        float dist = f1_distance(P.y, height) / noiseScale;
        float heightDensity = f1_sat(1.0f - dist);
        float4 mu = f4_mulvs(desc->noiseMu, heightDensity);
        hit.scattering = f4_add(hit.scattering, mu);
    }
    hit.extinction = f4_mulvs(hit.scattering, 1.0f + desc->absorption);
    return hit;
}

pim_inline float4 VEC_CALL MeanFreePathToMu(float4 mfp)
{
    // https://www.desmos.com/calculator/glcsylcry7
    return f4_rcp(mfp);
}

// maximum extinction coefficient of the media
pim_inline float4 VEC_CALL CalcMajorant(const PtMediaDesc* pim_noalias desc)
{
    float a = 1.0f + desc->absorption;
    float4 ca = f4_mulvs(desc->constantMu, a);
    float4 na = f4_mulvs(desc->noiseMu, a);
    return f4_mulvs(f4_add(ca, na), 2.0f);
}

pim_inline float VEC_CALL CalcPhase(
    PtMedia media,
    float cosTheta)
{
    return f1_lerp(
        MiePhase(cosTheta, media.g0),
        MiePhase(cosTheta, media.g1),
        media.gBlend);
}

pim_inline float4 VEC_CALL SamplePhaseDir(
    PtContext* pim_noalias ctx,
    PtScene* pim_noalias scene,
    PtMedia media,
    float4 rd)
{
    float4 L;
    do
    {
        L = SampleUnitSphere(Sample2D(ctx));
        L.w = CalcPhase(media, f4_dot3(rd, L));
    } while (Sample1D(ctx) > L.w);
    return L;
}

pim_inline float4 VEC_CALL CalcTransmittance(
    PtContext* pim_noalias ctx,
    PtScene* pim_noalias scene,
    float4 ro,
    float4 rd,
    float rayLen)
{
    const PtMediaDesc* pim_noalias desc = &scene->mediaDesc;
    const float rcpMaj = desc->rcpMajorant;
    float t = 0.0f;
    float4 attenuation = f4_1;
    while (true)
    {
        float dt = SampleFreePath(Sample1D(ctx), rcpMaj);
        if ((t + dt) >= rayLen)
        {
            break;
        }
        // ratio tracking
        // https://jannovak.info/publications/VolumeCourse/novak18monte-sig-slides-4.2-transmittance-notes.pdf#page=7
        PtMedia media = Media_Sample(desc, f4_add(ro, f4_mulvs(rd, t)));
        float4 ratio = f4_inv(f4_mulvs(media.extinction, rcpMaj));
        attenuation = f4_mul(attenuation, ratio);
        t += dt;
    }
    return attenuation;
}

pim_inline PtScatter VEC_CALL ScatterRay(
    PtContext* pim_noalias ctx,
    PtScene* pim_noalias scene,
    float4 ro,
    float4 rd,
    float rayLen,
    i32 bounce)
{
    PtScatter result = { 0 };
    result.pdf = 0.0f;

    PtMediaDesc const *const pim_noalias desc = &scene->mediaDesc;
    const float rcpMaj = desc->rcpMajorant;

    float t = 0.0f;
    result.attenuation = f4_1;
    while (true)
    {
        float dt = SampleFreePath(Sample1D(ctx), rcpMaj);
        t += dt;
        if (t >= rayLen)
        {
            break;
        }

        float4 P = f4_add(ro, f4_mulvs(rd, t));
        PtMedia media = Media_Sample(desc, P);
        float4 ratio = f4_inv(f4_mulvs(media.extinction, rcpMaj));
        result.attenuation = f4_mul(result.attenuation, ratio);

        float scatterProb = f4_hmax3(media.scattering) * rcpMaj;
        if (Sample1D(ctx) < scatterProb)
        {
            float4 lum;
            float4 L;
            if (EvaluateLight(ctx, scene, P, &lum, &L, bounce))
            {
                float ph = CalcPhase(media, f4_dot3(rd, L));
                lum = f4_mulvs(lum, ph * dt);
                lum = f4_mul(result.attenuation, lum);
                result.luminance = lum;
            }

            result.pos = P;
            result.dir = SamplePhaseDir(ctx, scene, media, rd);
            result.pdf = result.dir.w;
            float ph = CalcPhase(media, f4_dot3(rd, result.dir));
            result.attenuation = f4_mulvs(result.attenuation, ph);
            break;
        }
    }

    return result;
}

PtResult VEC_CALL Pt_TraceRay(
    PtScene* pim_noalias scene,
    float4 ro,
    float4 rd)
{
    PtResult result = { 0 };
    float resultWeight = 0.0f;
    float4 luminance = f4_0;
    float4 attenuation = f4_1;
    u32 prevFlags = 0;

    PtContext* pim_noalias ctx = PtContext_Get();

    for (i32 b = 0; b < 666; ++b)
    {
        {
            float p = f1_sat(f4_avglum(attenuation));
            if (Sample1D(ctx) < p)
            {
                attenuation = f4_divvs(attenuation, p);
            }
            else
            {
                break;
            }
        }

        PtRayHit hit = pt_intersect_local(scene, ro, rd, 0.0f, kRcpEpsilon);
        if (hit.type == PtHit_Nothing)
        {
            // TODO: toggle this off for lightmaps, on otherwise.
            // luminance = f4_add(luminance, f4_mul(attenuation, GetSky(scene, ro, rd)));
            break;
        }
        if ((hit.type == PtHit_Backface) && !(hit.flags & MatFlag_Refractive))
        {
            break;
        }

        {
            PtScatter scatter = ScatterRay(ctx, scene, ro, rd, hit.wuvt.w, b);
            if (scatter.pdf > kEpsilon)
            {
                luminance = f4_add(luminance, f4_mul(attenuation, scatter.luminance));
                attenuation = f4_mul(attenuation, f4_divvs(scatter.attenuation, scatter.pdf));
                {
                    float4 a = f4_mulvs(attenuation, 1.0f / kTau);
                    float w = f1_sat(1.0f - f4_avglum(a));
                    resultWeight += w;
                    result.albedo = f3_add(result.albedo, f3_mulvs(f4_f3(a), w));
                    result.normal = f3_add(result.normal, f3_mulvs(f4_f3(f4_neg(rd)), w));
                }
                ro = scatter.pos;
                rd = scatter.dir;
                prevFlags = 0;
                continue;
            }
            else
            {
                attenuation = f4_mul(attenuation, scatter.attenuation);
            }
        }

        PtSurfHit surf = GetSurface(scene, ro, rd, hit);
        if (b > 0)
        {
            LightOnHit(scene, ro, surf.emission, hit.iVert);
        }

        if ((b == 0) || (prevFlags & MatFlag_Refractive))
        {
            luminance = f4_add(luminance, f4_mul(surf.emission, attenuation));
        }
        if (hit.flags & MatFlag_Sky)
        {
            break;
        }

        {
            float4 Li = EstimateDirect(ctx, scene, &surf, &hit, rd, b);
            luminance = f4_add(luminance, f4_mul(Li, attenuation));
        }

        PtScatter scatter = Scatter_Principled(ctx, scene, &surf, rd);
        if (!(scatter.pdf > kEpsilon))
        {
            break;
        }
        ro = scatter.pos;
        rd = scatter.dir;

        attenuation = f4_mul(attenuation, f4_divvs(scatter.attenuation, scatter.pdf));
        prevFlags = surf.flags;

        {
            float4 a = f4_mulvs(attenuation, 1.0f / kPi);
            float w = f1_sat(1.0f - f4_avglum(a));
            resultWeight += w;
            result.albedo = f3_add(result.albedo, f3_mulvs(f4_f3(surf.albedo), w));
            result.normal = f3_add(result.normal, f3_mulvs(f4_f3(surf.N), w));
        }
    }

    {
        float s = 1.0f / f1_max(resultWeight, kEpsilon);
        result.color = f4_f3(luminance);
        result.albedo = f3_mulvs(result.albedo, s);
        result.normal = f3_mulvs(result.normal, s);
    }
    return result;
}

pim_inline Ray VEC_CALL CalculateDof(
    PtContext* pim_noalias ctx,
    const PtDofInfo* pim_noalias dof,
    float4 right,
    float4 up,
    float4 fwd,
    Ray ray)
{
    float2 offset;
    {
        u32 side = Prng_u32(&ctx->rng);
        float2 Xi = Sample2D(ctx);
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
} TaskUpdateDists;

static void UpdateDistsFn(void* pbase, i32 begin, i32 end)
{
    TaskUpdateDists*const pim_noalias task = pbase;
    PtScene*const pim_noalias scene = task->scene;
    Dist1D*const pim_noalias dists = scene->lightDists;
    for (i32 i = begin; i < end; ++i)
    {
        Dist1D_Update(&dists[i]);
    }
}

ProfileMark(pm_updatedists, UpdateDists)
static void UpdateDists(PtScene* pim_noalias scene)
{
    i32 worklen = Grid_Len(&scene->lightGrid);
    if (worklen > 0)
    {
        ProfileBegin(pm_updatedists);
        TaskUpdateDists *const pim_noalias task = Temp_Calloc(sizeof(*task));
        task->scene = scene;
        Task_Run(task, UpdateDistsFn, worklen);
        ProfileEnd(pm_updatedists);
    }
}

static void DofUpdate(
    PtDofInfo* pim_noalias dof,
    const PtScene* pim_noalias scene,
    const Camera* pim_noalias camera)
{
    if (dof->autoFocus)
    {
        float4 ro = camera->position;
        float4 rd = quat_fwd(camera->rotation);
        PtRayHit hit = pt_intersect_local(scene, ro, rd, 0.0f, kRcpEpsilon);
        if (hit.type != PtHit_Nothing)
        {
            float dt = (float)Time_Deltaf();
            float rate = dof->autoFocusSpeed;
            float t = 1.0f - expf(-dt * rate);
            dof->focalLength = f1_lerp(dof->focalLength, hit.wuvt.w, t);
        }
    }
}

typedef struct PtTraceTask_s
{
    Task task;
    const Camera* pim_noalias camera;
    const PtDofInfo* pim_noalias dof;
    PtScene* pim_noalias scene;
    PtTrace* pim_noalias trace;
} PtTraceTask;

static void TraceFn(void* pbase, i32 begin, i32 end)
{
    PtTraceTask *const pim_noalias task = pbase;

    const PtDofInfo* pim_noalias dof = task->dof;
    const Camera* pim_noalias camera = task->camera;
    PtScene *const pim_noalias scene = task->scene;
    PtTrace* const pim_noalias trace = task->trace;

    float3* const pim_noalias colors = trace->color;
    float3* const pim_noalias albedos = trace->albedo;
    float3* const pim_noalias normals = trace->normal;

    const int2 size = trace->imageSize;
    const float2 rcpSize = f2_rcp(i2_f2(size));

    const quat rot = camera->rotation;
    const float4 eye = camera->position;
    const float4 right = quat_right(rot);
    const float4 up = quat_up(rot);
    const float4 fwd = quat_fwd(rot);
    const float2 slope = proj_slope(f1_radians(camera->fovy), (float)size.x / (float)size.y);
    const float sampleWeight = trace->sampleWeight;

    PtContext* pim_noalias ctx = PtContext_Get();
    for (i32 i = begin; i < end; ++i)
    {
        const int2 coord = { i % size.x, i / size.x };
        const float2 baseUv = { (coord.x + 0.5f) * rcpSize.x, (coord.y + 0.5f) * rcpSize.y };
        const float2 aa = SampleGaussPixelFilter(Sample2D(ctx), 1.0f);
        const float2 rayUv = f2_add(baseUv, f2_mul(aa, rcpSize));

        Ray ray = { eye, proj_dir(right, up, fwd, slope, f2_snorm(rayUv)) };
        ray = CalculateDof(ctx, dof, right, up, fwd, ray);

        PtResult result = Pt_TraceRay(scene, ray.ro, ray.rd);
        colors[i] = f3_lerpvs(colors[i], result.color, sampleWeight);
        albedos[i] = f3_lerpvs(albedos[i], result.albedo, sampleWeight);
        normals[i] = f3_lerpvs(normals[i], result.normal, sampleWeight);
    }
}

ProfileMark(pm_trace, Pt_Trace)
void Pt_Trace(
    PtTrace* pim_noalias trace,
    PtDofInfo* pim_noalias dof,
    PtScene* pim_noalias scene,
    const Camera* pim_noalias camera)
{
    ProfileBegin(pm_trace);

    ASSERT(trace);
    ASSERT(dof);
    ASSERT(scene);
    ASSERT(camera);
    ASSERT(trace->color);
    ASSERT(trace->albedo);
    ASSERT(trace->normal);

    PtScene_Update(scene);
    DofUpdate(dof, scene, camera);

    PtTraceTask* pim_noalias task = Temp_Calloc(sizeof(*task));
    task->dof = dof;
    task->scene = scene;
    task->camera = camera;
    task->trace = trace;
    const i32 workSize = trace->imageSize.x * trace->imageSize.y;
    Task_Run(task, TraceFn, workSize);

    ProfileEnd(pm_trace);
}

typedef struct PtRayGenTask_s
{
    Task task;
    float4 origin;
    PtScene* pim_noalias scene;
    float4* pim_noalias colors;
    float4* pim_noalias directions;
} PtRayGenTask;

static void RayGenFn(void* pBase, i32 begin, i32 end)
{
    PtRayGenTask* pim_noalias task = pBase;
    const float4 ro = task->origin;
    PtScene* pim_noalias scene = task->scene;
    float4* pim_noalias colors = task->colors;
    float4* pim_noalias directions = task->directions;

    PtContext* pim_noalias ctx = PtContext_Get();
    for (i32 i = begin; i < end; ++i)
    {
        float4 rd = SampleUnitSphere(Sample2D(ctx));
        directions[i] = rd;
        PtResult result = Pt_TraceRay(scene, ro, rd);
        colors[i] = f3_f4(result.color, 0.0f);
    }
}

ProfileMark(pm_raygen, Pt_RayGen)
PtResults Pt_RayGen(
    PtScene* pim_noalias scene,
    float4 origin,
    i32 count)
{
    ProfileBegin(pm_raygen);

    ASSERT(scene);
    ASSERT(count >= 0);

    PtScene_Update(scene);

    PtRayGenTask* pim_noalias task = Temp_Calloc(sizeof(*task));
    task->scene = scene;
    task->origin = origin;
    task->colors = Temp_Alloc(sizeof(task->colors[0]) * count);
    task->directions = Temp_Alloc(sizeof(task->directions[0]) * count);

    Task_Run(task, RayGenFn, count);

    PtResults results =
    {
        .colors = task->colors,
        .directions = task->directions,
    };

    ProfileEnd(pm_raygen);

    return results;
}
