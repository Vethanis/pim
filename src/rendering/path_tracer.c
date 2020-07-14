#include "rendering/path_tracer.h"
#include "rendering/material.h"
#include "rendering/mesh.h"
#include "rendering/camera.h"
#include "rendering/sampler.h"
#include "rendering/lights.h"
#include "rendering/drawable.h"
#include "rendering/librtc.h"

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

#include "allocator/allocator.h"
#include "threading/task.h"
#include "common/random.h"
#include "common/profiler.h"
#include "common/console.h"
#include "common/cvar.h"

#include <string.h>

#define NEE                 1
#define MIS                 0
#define kRejectionSamples   3
#define kRngSeqLen          64

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
    // emissive texel chance of triangle
    // [emissiveCount]
    float* pim_noalias emPdfs;

    // surface description, indexed by matIds
    // [matCount]
    material_t* pim_noalias materials;

    // array lengths
    i32 vertCount;
    i32 matCount;
    i32 emissiveCount;
    i32 nodeCount;
    // hyperparameters
    float3 sunDirection;
    float sunIntensity;
} pt_scene_t;

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
    float4 dir;
    float4 attenuation;
    float4 irradiance;
    float pdf;
} scatter_t;

typedef struct trace_task_s
{
    task_t task;
    pt_trace_t* trace;
    camera_t camera;
} trace_task_t;

typedef struct pt_raygen_s
{
    task_t task;
    const pt_scene_t* scene;
    float4 origin;
    float4* colors;
    float4* directions;
} pt_raygen_t;


static bool ms_once;
static RTCDevice ms_device;
static pt_sampler_t ms_samplers[kNumThreads];
static float2 ms_hammersley[kRngSeqLen];

static cvar_t* cv_r_sun_az;
static cvar_t* cv_r_sun_ze;
static cvar_t* cv_r_sun_rad;

static void OnRtcError(void* user, RTCError error, const char* msg)
{
    if (error != RTC_ERROR_NONE)
    {
        con_logf(LogSev_Error, "rtc", "%s", msg);
        ASSERT(false);
    }
}

static bool EnsureInit(void)
{
    if (!ms_once)
    {
        ms_once = true;
        bool hasInit = rtc_init();
        if (!hasInit)
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

        cv_r_sun_az = cvar_find("r_sun_az");
        cv_r_sun_ze = cvar_find("r_sun_ze");
        cv_r_sun_rad = cvar_find("r_sun_rad");

        prng_t rng = prng_get();
        for (i32 i = 0; i < kRngSeqLen; ++i)
        {
            ms_hammersley[i] = Hammersley2D(i, kRngSeqLen);
        }
        for (i32 i = 0; i < kRngSeqLen; ++i)
        {
            i32 j = prng_i32(&rng) & (kRngSeqLen - 1);
            float2 tmp = ms_hammersley[i];
            ms_hammersley[i] = ms_hammersley[j];
            ms_hammersley[j] = tmp;
        }
        prng_set(rng);
    }
    return ms_device != NULL;
}

pim_inline float2 VEC_CALL pt_sampler_next2(pt_sampler_t* sampler)
{
    float2 jit = sampler->seq.jit;
    i32 c = sampler->seq.cur++ & (kRngSeqLen - 1);
    if (c == 0)
    {
        jit = f2_rand(&sampler->rng);
        sampler->seq.jit = jit;
    }
    float2 Xi = ms_hammersley[c];
    Xi = f2_add(Xi, jit);
    Xi = f2_frac(Xi);
    return Xi;
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

static RTCRayHit VEC_CALL RtcIntersect(
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

static bool VEC_CALL RtcOccluded(
    RTCScene scene,
    ray_t ray,
    float tNear,
    float tFar)
{
    RTCIntersectContext ctx;
    rtcInitIntersectContext(&ctx);
    RTCRay rtcRay = RtcNewRay(ray, tNear, tFar);
    rtc.Occluded1(scene, &ctx, &rtcRay);
    // tfar == -inf on hit
    return rtcRay.tfar >= 0.0f;
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

static float EmissionPdf(
    const pt_scene_t* scene,
    pt_sampler_t* sampler,
    i32 iVert,
    i32 attempts)
{
    i32 hits = 0;
    const i32 iMat = scene->matIds[iVert];
    const material_t* mat = scene->materials + iMat;
    const float2* pim_noalias uvs = scene->uvs;
    if (mat->flags & matflag_sky)
    {
        return 1.0f;
    }
    texture_t romeMap = { 0 };
    if (texture_get(mat->rome, &romeMap))
    {
        const float2 UA = uvs[iVert + 0];
        const float2 UB = uvs[iVert + 1];
        const float2 UC = uvs[iVert + 2];
        const int2 size = romeMap.size;
        const float2 sizef = { (float)size.x, (float)size.y };

        for (i32 i = 0; i < attempts; ++i)
        {
            for (i32 j = 0; j < kRejectionSamples; ++j)
            {
                float4 wuv = SampleBaryCoord(f2_rand(&sampler->rng));
                float2 uv = f2_blend(UA, UB, UC, wuv);
                float2 coordf = f2_addvs(f2_mul(sizef, uv), 0.5f);
                int2 coord = { (i32)coordf.x, (i32)coordf.y };
                coord = i2_mod(coord, size);
                i32 iTexel = coord.x + coord.y * size.x;
                u32 texel = romeMap.texels[iTexel];
                u32 a = (texel >> 24) & 0xff;
                if (a)
                {
                    ++hits;
                    j = kRejectionSamples;
                }
            }
        }
    }
    return (float)hits / (float)attempts;
}

pim_inline float4 VEC_CALL SampleEmissiveCoord(
    const pt_scene_t* scene,
    pt_sampler_t* sampler,
    i32 iVert)
{
    const i32 iMat = scene->matIds[iVert];
    const material_t* mat = scene->materials + iMat;
    const float2* pim_noalias uvs = scene->uvs;
    if (!(mat->flags & matflag_sky))
    {
        texture_t romeMap = { 0 };
        if (texture_get(mat->rome, &romeMap))
        {
            const float2 UA = uvs[iVert + 0];
            const float2 UB = uvs[iVert + 1];
            const float2 UC = uvs[iVert + 2];
            const int2 size = romeMap.size;
            const float2 sizef = { (float)size.x, (float)size.y };

            for (i32 i = 0; i < kRejectionSamples; ++i)
            {
                float4 wuv = SampleBaryCoord(f2_rand(&sampler->rng));
                float2 uv = f2_blend(UA, UB, UC, wuv);
                float2 coordf = f2_addvs(f2_mul(sizef, uv), 0.5f);
                int2 coord = { (i32)coordf.x, (i32)coordf.y };
                coord = i2_mod(coord, size);
                i32 iTexel = coord.x + coord.y * size.x;
                u32 texel = romeMap.texels[iTexel];
                u32 a = (texel >> 24) & 0xff;
                if (a)
                {
                    return wuv;
                }
            }
        }
    }

    return SampleBaryCoord(f2_rand(&sampler->rng));
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

    pt_sampler_t sampler = pt_sampler_get();
    for (i32 i = begin; i < end; ++i)
    {
        pdfs[i] = EmissionPdf(scene, &sampler, i * 3, attempts);
    }
    pt_sampler_set(sampler);
}

static void SetupEmissives(pt_scene_t* scene)
{
    const i32 vertCount = scene->vertCount;
    const i32 triCount = vertCount / 3;

    task_CalcEmissionPdf* task = tmp_calloc(sizeof(*task));
    task->scene = scene;
    task->pdfs = tmp_malloc(sizeof(task->pdfs[0]) * triCount);
    task->attempts = 1000;

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

static void UpdateScene(pt_scene_t* scene)
{
    float azimuth = cv_r_sun_az ? f1_sat(cv_r_sun_az->asFloat) : 0.0f;
    float zenith = cv_r_sun_ze ? f1_sat(cv_r_sun_ze->asFloat) : 0.5f;
    float irradiance = cv_r_sun_rad ? cv_r_sun_rad->asFloat : 100.0f;
    const float4 kUp = { 0.0f, 1.0f, 0.0f, 0.0f };

    float4 sunDir = TanToWorld(kUp, SampleUnitHemisphere(f2_v(azimuth, zenith)));
    scene->sunDirection = f4_f3(sunDir);
    scene->sunIntensity = irradiance;
}


pt_sampler_t pt_sampler_get(void)
{
    const i32 tid = task_thread_id();
    return ms_samplers[tid];
}

void pt_sampler_set(pt_sampler_t sampler)
{
    const i32 tid = task_thread_id();
    ms_samplers[tid] = sampler;
}

pt_scene_t* pt_scene_new(void)
{
    if (!EnsureInit())
    {
        return NULL;
    }

    pt_scene_t* scene = perm_calloc(sizeof(*scene));
    UpdateScene(scene);

    FlattenDrawables(scene);
    SetupEmissives(scene);
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

        memset(scene, 0, sizeof(*scene));
        pim_free(scene);
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

pim_inline float4 VEC_CALL GetSky(const pt_scene_t* scene, ray_t rin)
{
    return f3_f4(EarthAtmosphere(f4_f3(rin.ro), f4_f3(rin.rd), scene->sunDirection, scene->sunIntensity), 0.0f);
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
    surf.P = f4_add(rin.ro, f4_mulvs(rin.rd, hit.wuvt.w));
    float4 Nws = f4_normalize3(GetVert4(scene->normals, hit.index, hit.wuvt));
    surf.P = f4_add(surf.P, f4_mulvs(Nws, kMilli));
    surf.N = TanToWorld(Nws, material_normal(mat, uv));
    float4 albedo = material_albedo(mat, uv);
    float4 rome = material_rome(mat, uv);
    surf.albedo = albedo;
    surf.roughness = rome.x;
    surf.occlusion = rome.y;
    surf.metallic = rome.z;
    surf.emission = UnpackEmission(albedo, rome.w);

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
    pt_sampler_t* sampler,
    float4 I,
    float4 N,
    float alpha)
{
    float2 Xi = pt_sampler_next2(sampler);
    float4 H = TanToWorld(N, SampleGGXMicrofacet(Xi, alpha));
    float4 L = f4_normalize3(f4_reflect3(I, H));
    return L;
}

pim_inline float4 VEC_CALL SampleDiffuse(pt_sampler_t* sampler, float4 N)
{
    float2 Xi = pt_sampler_next2(sampler);
    float4 L = TanToWorld(N, SampleCosineHemisphere(Xi));
    return L;
}

// samples a light direction of the brdf
pim_inline float4 VEC_CALL BrdfSample(
    pt_sampler_t* sampler,
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
    if (prng_f32(&sampler->rng) < specProb)
    {
        L = SampleSpecular(sampler, I, N, alpha);
    }
    else
    {
        L = SampleDiffuse(sampler, N);
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
    pt_sampler_t* sampler,
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
    if (prng_f32(&sampler->rng) < rcpProb)
    {
        L = SampleSpecular(sampler, I, N, alpha);
    }
    else
    {
        L = SampleDiffuse(sampler, N);
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

pim_inline bool LightSelect(
    const pt_scene_t* scene,
    pt_sampler_t* sampler,
    i32* idOut,
    float* pdfOut)
{
    i32 lightCount = scene->emissiveCount;
    if (lightCount > 0)
    {
        i32 iList = prng_i32(&sampler->rng) % lightCount;
        i32 iVert = scene->emissives[iList];
        float emPdf = scene->emPdfs[iList];
        *idOut = iVert;
        *pdfOut = emPdf / lightCount;
        return true;
    }
    *idOut = -1;
    *pdfOut = 0.0f;
    return false;
}

pim_inline float4 VEC_CALL LightRad(const pt_scene_t* scene, i32 iLight, float4 wuv, float4 ro, float4 rd)
{
    i32 iMat = scene->matIds[iLight];
    const material_t* mat = scene->materials + iMat;
    if (mat->flags & matflag_sky)
    {
        return f3_f4(EarthAtmosphere(f4_f3(ro), f4_f3(rd), scene->sunDirection, scene->sunIntensity), 0.0f);
    }
    float4 albedo = ColorToLinear(mat->flatAlbedo);
    float4 rome = ColorToLinear(mat->flatRome);
    texture_t albedoMap = { 0 };
    if (texture_get(mat->albedo, &albedoMap))
    {
        float2 uv = GetVert2(scene->uvs, iLight, wuv);
        float4 sample = UvBilinearWrap_c32(albedoMap.texels, albedoMap.size, uv);
        albedo = f4_mul(albedo, sample);
        return UnpackEmission(albedo, rome.w);
    }
    return f4_0;
}

typedef struct lightsample_s
{
    float4 direction;
    float4 irradiance;
    float4 wuvt;
    float pdf;
} lightsample_t;

pim_inline lightsample_t VEC_CALL LightSample(
    const pt_scene_t* scene,
    pt_sampler_t* sampler,
    const surfhit_t* surf,
    i32 iLight)
{
    lightsample_t sample = { 0 };

    float4 wuv = SampleEmissiveCoord(scene, sampler, iLight);

    const float4* pim_noalias positions = scene->positions;
    float4 A = positions[iLight + 0];
    float4 B = positions[iLight + 1];
    float4 C = positions[iLight + 2];
    float4 pt = f4_blend(A, B, C, wuv);
    float area = TriArea3D(A, B, C);

    float4 ro = surf->P;
    float4 rd = f4_sub(pt, ro);
    float distSq = f4_dot3(rd, rd);
    float distance = sqrtf(distSq);
    wuv.w = distance;
    rd = f4_divvs(rd, distance);

    sample.direction = rd;
    sample.wuvt = wuv;

    float LoNs = f4_dot3(rd, surf->N);
    if (LoNs > kEpsilon)
    {
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
            }
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
                return LightPdf(GetArea(scene, iLight), cosTheta, distance * distance);
            }
        }
    }
    return 0.0f;
}

pim_inline float4 VEC_CALL EstimateDirect(
    const pt_scene_t* scene,
    pt_sampler_t* sampler,
    const surfhit_t* surf,
    const rayhit_t* hit,
    i32 iLight,
    float4 I)
{
    float4 result = f4_0;
    {
        lightsample_t sample = LightSample(scene, sampler, surf, iLight);
        if (sample.pdf > kEpsilon)
        {
            float4 brdf = BrdfEval(I, surf, sample.direction);
            float brdfPdf = brdf.w;
            if (brdf.w > kEpsilon)
            {
                float4 irradiance = LightRad(scene, iLight, sample.wuvt, surf->P, sample.direction);
#if MIS
                float weight = PowerHeuristic(sample.pdf, brdf.w);
                brdf = f4_mulvs(brdf, weight);
#endif // MIS
                float4 Li = f4_divvs(f4_mul(irradiance, brdf), sample.pdf);
                result = f4_add(result, Li);
            }
        }
    }
#if MIS
    {
        scatter_t sample = BrdfScatter(rng, surf, I);
        if (sample.pdf > kEpsilon)
        {
            float4 wuv;
            float lightPdf = LightEvalPdf(scene, iLight, surf->P, sample.dir, &wuv);
            if (lightPdf > kEpsilon)
            {
                float4 irradiance = LightRad(scene, iLight, wuv, surf->P, sample.dir);
                float weight = PowerHeuristic(sample.pdf, lightPdf);
                sample.attenuation = f4_mulvs(sample.attenuation, weight);
                float4 Li = f4_divvs(f4_mul(irradiance, sample.attenuation), sample.pdf);
                result = f4_add(result, Li);
            }
        }
    }
#endif // MIS
    return result;
}

pim_inline float4 VEC_CALL SampleLights(
    const pt_scene_t* scene,
    pt_sampler_t* sampler,
    const surfhit_t* surf,
    const rayhit_t* hit,
    float4 I)
{
    i32 iLight;
    float lightPdfWeight;
    if (LightSelect(scene, sampler, &iLight, &lightPdfWeight))
    {
        if (hit->index != iLight)
        {
            float4 direct = EstimateDirect(scene, sampler, surf, hit, iLight, I);
            return f4_divvs(direct, lightPdfWeight);
        }
    }
    return f4_0;
}

pt_result_t VEC_CALL pt_trace_ray(
    pt_sampler_t* sampler,
    const pt_scene_t* scene,
    ray_t ray)
{
    float4 albedo = f4_0;
    float4 normal = f4_0;
    float4 light = f4_0;
    float4 attenuation = f4_1;

    for (i32 b = 0; b < 666; ++b)
    {
        const rayhit_t hit = pt_intersect_local(scene, ray, 0.0f, 1 << 20);
        if (hit.type == hit_nothing)
        {
            break;
        }
        if (hit.type == hit_backface)
        {
            break;
        }
        if (hit.type == hit_sky)
        {
            light = f4_add(light, f4_mul(GetSky(scene, ray), attenuation));
            break;
        }

        const surfhit_t surf = GetSurface(scene, ray, hit, b);
        if (b == 0)
        {
            albedo = surf.albedo;
            normal = surf.N;
        }

#if NEE
        if (b == 0)
        {
            light = f4_add(light, f4_mul(surf.emission, attenuation));
        }
        {
            float4 direct = SampleLights(scene, sampler, &surf, &hit, ray.rd);
            light = f4_add(light, f4_mul(direct, attenuation));
        }
#else
        light = f4_add(light, f4_mul(surf.emission, attenuation));
#endif // NEE

        const scatter_t scatter = BrdfScatter(sampler, &surf, ray.rd);
        if (scatter.pdf <= 0.0f)
        {
            break;
        }

        attenuation = f4_mul(
            attenuation,
            f4_divvs(scatter.attenuation, scatter.pdf));

        ray.ro = surf.P;
        ray.rd = scatter.dir;

        {
            float p = f1_lerp(0.01f, 0.99f, f1_sat(f4_hmax3(attenuation)));
            if (prng_f32(&sampler->rng) < p)
            {
                attenuation = f4_divvs(attenuation, p);
            }
            else
            {
                break;
            }
        }
    }

    pt_result_t result;
    result.color = f4_f3(light);
    result.albedo = f4_f3(albedo);
    result.normal = f4_f3(normal);
    return result;
}

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

    pt_sampler_t sampler = pt_sampler_get();
    for (i32 i = begin; i < end; ++i)
    {
        int2 coord = { i % size.x, i / size.x };

        float2 Xi = f2_tent(f2_rand(&sampler.rng));
        Xi = f2_mul(Xi, rcpSize);

        float2 uv = CoordToUv(size, coord);
        uv = f2_add(uv, Xi);
        uv = f2_snorm(uv);

        float4 rd = proj_dir(right, up, fwd, slope, uv);
        ray_t ray = { ro, rd };
        pt_result_t result = pt_trace_ray(&sampler, scene, ray);
        color[i] = f3_lerp(color[i], result.color, sampleWeight);
        albedo[i] = f3_lerp(albedo[i], result.albedo, sampleWeight);
        normal[i] = f3_lerp(normal[i], result.normal, sampleWeight);
    }
    pt_sampler_set(sampler);
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

static void RayGenFn(task_t* pBase, i32 begin, i32 end)
{
    pt_raygen_t* task = (pt_raygen_t*)pBase;
    const pt_scene_t* scene = task->scene;
    const float4 ro = task->origin;

    float4* pim_noalias colors = task->colors;
    float4* pim_noalias directions = task->directions;

    pt_sampler_t sampler = pt_sampler_get();
    for (i32 i = begin; i < end; ++i)
    {
        float2 Xi = f2_rand(&sampler.rng);
        float4 rd = SampleUnitSphere(Xi);
        directions[i] = rd;
        pt_result_t result = pt_trace_ray(&sampler, scene, (ray_t) { ro, rd });
        colors[i] = f3_f4(result.color, 1.0f);
    }
    pt_sampler_set(sampler);
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
