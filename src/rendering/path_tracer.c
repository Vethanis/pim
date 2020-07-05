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

typedef enum
{
    hit_nothing = 0,
    hit_triangle,
    hit_sky,

    hit_COUNT
} hittype_t;

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

typedef struct rayhit_s
{
    float4 wuvt;
    hittype_t type;
    i32 index;
} rayhit_t;

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
    pt_trace_t trace;
    camera_t camera;
} trace_task_t;

typedef struct pt_raygen_s
{
    task_t task;
    const pt_scene_t* scene;
    ray_t origin;
    float4* colors;
    float4* directions;
    pt_dist_t dist;
} pt_raygen_t;

static bool ms_once;
static RTCDevice ms_device;

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
    }
    return ms_device != NULL;
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
    prng_t* rng,
    i32 iLight,
    i32 attempts)
{
    i32 hits = 0;
    const i32 iMat = scene->matIds[iLight];
    const material_t mat = scene->materials[iMat];
    if (mat.flags & matflag_sky)
    {
        return 1.0f;
    }
    const float4 flatRome = ColorToLinear(mat.flatRome);
    texture_t romeMap = { 0 };
    if (texture_get(mat.rome, &romeMap))
    {
        float2 UA = scene->uvs[iLight + 0];
        float2 UB = scene->uvs[iLight + 1];
        float2 UC = scene->uvs[iLight + 2];

        const i32 rejectionSamples = scene->rejectionSamples;
        const float threshold = 1.0f / 255.0f;
        for (i32 i = 0; i < attempts; ++i)
        {
            for (i32 j = 0; j < rejectionSamples; ++j)
            {
                float4 wuv = SampleBaryCoord(f2_rand(rng));
                float2 uv = f2_blend(UA, UB, UC, wuv);
                float4 rome = f4_mul(flatRome,
                    UvBilinearWrap_c32(romeMap.texels, romeMap.size, uv));
                if (rome.w > threshold)
                {
                    ++hits;
                    j = rejectionSamples;
                }
            }
        }
    }
    return (float)hits / (float)attempts;
}

static float4 VEC_CALL SelectEmissiveCoord(
    const pt_scene_t* scene,
    prng_t* rng,
    i32 iLight)
{
    float4 wuv = SampleBaryCoord(f2_rand(rng));
    const i32 iMat = scene->matIds[iLight];
    const material_t mat = scene->materials[iMat];
    if (mat.flags & matflag_sky)
    {
        return wuv;
    }
    const float4 flatRome = ColorToLinear(mat.flatRome);
    texture_t romeMap = { 0 };
    if (texture_get(mat.rome, &romeMap))
    {
        float2 UA = scene->uvs[iLight + 0];
        float2 UB = scene->uvs[iLight + 1];
        float2 UC = scene->uvs[iLight + 2];

        const i32 rejectionSamples = scene->rejectionSamples;
        const float threshold = 1.0f / 255.0f;
        for (i32 i = 0; i < rejectionSamples; ++i)
        {
            float2 uv = f2_blend(UA, UB, UC, wuv);
            float4 rome = f4_mul(flatRome,
                UvBilinearWrap_c32(romeMap.texels, romeMap.size, uv));
            if (rome.w > threshold)
            {
                break;
            }
            wuv = SampleBaryCoord(f2_rand(rng));
        }
    }
    return wuv;
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
    task->attempts = 1000;

    task_run(&task->task, CalcEmissionPdfFn, triCount);

    i32 emissiveCount = 0;
    i32* emissives = NULL;
    float* pim_noalias emPdfs = NULL;

    const float* pim_noalias taskPdfs = task->pdfs;
    for (i32 iTri = 0; iTri < triCount; ++iTri)
    {
        float pdf = taskPdfs[iTri];
        if (pdf >= 0.1f)
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

static void UpdateSun(pt_scene_t* scene)
{
    float azimuth = cv_r_sun_az ? f1_sat(cv_r_sun_az->asFloat) : 0.0f;
    float zenith = cv_r_sun_ze ? f1_sat(cv_r_sun_ze->asFloat) : 0.5f;
    float irradiance = cv_r_sun_rad ? cv_r_sun_rad->asFloat : 100.0f;
    const float4 kUp = { 0.0f, 1.0f, 0.0f, 0.0f };

    float4 sunDir = TanToWorld(kUp, SampleUnitSphere(f2_v(zenith, azimuth)));
    scene->sunDirection = f4_f3(sunDir);
    scene->sunIntensity = irradiance;
}

pt_scene_t* pt_scene_new(i32 maxDepth, i32 rejectionSamples)
{
    if (!EnsureInit())
    {
        return NULL;
    }

    pt_scene_t* scene = perm_calloc(sizeof(*scene));
    scene->rejectionSamples = rejectionSamples;
    UpdateSun(scene);

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
    rayhit_t hit)
{
    return f4_blend(
        vertices[hit.index + 0],
        vertices[hit.index + 1],
        vertices[hit.index + 2],
        hit.wuvt);
}

pim_inline float2 VEC_CALL GetVert2(
    const float2* vertices,
    rayhit_t hit)
{
    return f2_blend(
        vertices[hit.index + 0],
        vertices[hit.index + 1],
        vertices[hit.index + 2],
        hit.wuvt);
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

static surfhit_t VEC_CALL GetSurface(
    const pt_scene_t* scene,
    ray_t rin,
    rayhit_t hit,
    i32 bounce)
{
    surfhit_t surf = { 0 };

    const material_t* mat = GetMaterial(scene, hit);
    if (hit.type == hit_sky)
    {
        if (bounce == 0)
        {
            // only 0th bounce accesses emission from surfhit
            // later bounces use EstimateDirect
            surf.emission = f3_f4(EarthAtmosphere(f4_f3(rin.ro), f4_f3(rin.rd), scene->sunDirection, scene->sunIntensity), 0.0f);
        }
    }
    else
    {
        float2 uv = GetVert2(scene->uvs, hit);
        surf.P = f4_add(rin.ro, f4_mulvs(rin.rd, hit.wuvt.w));
        float4 Nws = f4_normalize3(GetVert4(scene->normals, hit));
        surf.P = f4_add(surf.P, f4_mulvs(Nws, kMilli));
        surf.N = TanToWorld(Nws, material_normal(mat, uv));
        float4 albedo = material_albedo(mat, uv);
        float4 rome = material_rome(mat, uv);
        surf.albedo = albedo;
        surf.roughness = rome.x;
        surf.occlusion = rome.y;
        surf.metallic = rome.z;
        surf.emission = UnpackEmission(albedo, rome.w);
    }

    return surf;
}

static rayhit_t VEC_CALL TraceRay(
    const pt_scene_t* scene,
    ray_t ray,
    float tFar)
{
    rayhit_t hit = { 0 };
    hit.wuvt.w = tFar;

    RTCRayHit rtcHit = RtcIntersect(scene->rtcScene, ray, 0.0f, tFar);
    bool hitNothing =
        (rtcHit.hit.geomID == RTC_INVALID_GEOMETRY_ID) ||
        (rtcHit.ray.tfar <= 0.0f);
    if (hitNothing)
    {
        hit.index = -1;
        hit.type = hit_nothing;
        return hit;
    }
    ASSERT(rtcHit.hit.primID != RTC_INVALID_GEOMETRY_ID);
    i32 iVert = rtcHit.hit.primID * 3;
    ASSERT(iVert >= 0);
    ASSERT(iVert < scene->vertCount);
    float u = rtcHit.hit.u;
    float v = rtcHit.hit.v;
    float w = 1.0f - (u + v);
    float t = rtcHit.ray.tfar;

    hit.type = hit_triangle;
    hit.wuvt = f4_v(w, u, v, t);
    hit.index = iVert;

    if (GetMaterial(scene, hit)->flags & matflag_sky)
    {
        hit.type = hit_sky;
    }

    return hit;
}

static rngseq_t rngseq_SampleSpecular[kNumThreads];
pim_inline float4 VEC_CALL SampleSpecular(
    prng_t* rng,
    float4 I,
    float4 N,
    float alpha)
{
    float2 Xi = rngseq_next(rng, rngseq_SampleSpecular + task_thread_id(), 64);
    float4 H = TanToWorld(N, SampleGGXMicrofacet(Xi, alpha));
    float4 L = f4_normalize3(f4_reflect3(I, H));
    return L;
}

static rngseq_t rngseq_SampleDiffuse[kNumThreads];
pim_inline float4 VEC_CALL SampleDiffuse(prng_t* rng, float4 N)
{
    float2 Xi = rngseq_next(rng, rngseq_SampleDiffuse + task_thread_id(), 64);
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

// returns pdf of the given interaction
pim_inline float VEC_CALL BrdfEvalPdf(
    float4 I,
    const surfhit_t* surf,
    float4 L)
{
    float4 N = surf->N;
    float NoL = f4_dot3(N, L);
    if (NoL < kEpsilon)
    {
        return 0.0f;
    }

    float4 V = f4_neg(I);
    float4 H = f4_normalize3(f4_add(V, L));
    float NoH = f4_dotsat(N, H);
    float HoV = f4_dotsat(H, V);

    // metals are only specular
    const float amtDiffuse = 1.0f - surf->metallic;
    const float amtSpecular = 1.0f;
    const float rcpProb = 1.0f / (amtSpecular + amtDiffuse);

    float alpha = BrdfAlpha(surf->roughness);
    float diffusePdf = amtDiffuse * LambertPdf(NoL);
    float specularPdf = amtSpecular * GGXPdf(NoH, HoV, alpha);
    float pdf = (diffusePdf + specularPdf) * rcpProb;

    return pdf;
}

// returns attenuation value for the given interaction
pim_inline float4 VEC_CALL BrdfEval(
    float4 I,
    const surfhit_t* surf,
    float4 L)
{
    float4 N = surf->N;
    float NoL = f4_dot3(N, L);
    if (NoL < kEpsilon)
    {
        return f4_0;
    }

    float4 V = f4_neg(I);
    float4 albedo = surf->albedo;
    float metallic = surf->metallic;
    float roughness = surf->roughness;
    float alpha = BrdfAlpha(roughness);
    const float amtDiffuse = 1.0f - metallic;
    const float amtSpecular = 1.0f;
    float4 H = f4_normalize3(f4_add(V, L));
    float NoH = f4_dotsat(N, H);
    float HoV = f4_dotsat(H, V);
    float NoV = f4_dotsat(N, V);

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
    return f4_mulvs(brdf, NoL);
}

static scatter_t VEC_CALL BrdfScatter(
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

    bool specular = prng_f32(rng) < rcpProb;
    float4 L = specular ?
        SampleSpecular(rng, I, N, alpha) :
        SampleDiffuse(rng, N);

    float NoL = f4_dotsat(N, L);
    if (NoL < kEpsilon)
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

    if (pdf < kEpsilon)
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

static bool LightSelect(
    const pt_scene_t* scene,
    prng_t* rng,
    i32* idOut,
    float* pdfOut)
{
    i32 lightCount = scene->emissiveCount;
    if (lightCount > 0)
    {
        i32 iList = prng_i32(rng) % lightCount;
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

static float4 VEC_CALL LightRad(const pt_scene_t* scene, i32 iLight, float4 wuv, float4 P, float4 L)
{
    i32 iMat = scene->matIds[iLight];
    const material_t mat = scene->materials[iMat];
    if (mat.flags & matflag_sky)
    {
        return f3_f4(EarthAtmosphere(f4_f3(P), f4_f3(L), scene->sunDirection, scene->sunIntensity), 0.0f);
    }
    float4 albedo = ColorToLinear(mat.flatAlbedo);
    float4 rome = ColorToLinear(mat.flatRome);
    texture_t albedoMap = { 0 };
    if (texture_get(mat.albedo, &albedoMap))
    {
        float2 UA = scene->uvs[iLight + 0];
        float2 UB = scene->uvs[iLight + 1];
        float2 UC = scene->uvs[iLight + 2];
        float2 uv = f2_blend(UA, UB, UC, wuv);
        float4 sample = UvBilinearWrap_c32(albedoMap.texels, albedoMap.size, uv);
        albedo = f4_mul(albedo, sample);
    }
    float4 emission = UnpackEmission(albedo, rome.w);
    return emission;
}

static float4 VEC_CALL LightSample(
    const pt_scene_t* scene,
    prng_t* rng,
    const surfhit_t* surf,
    i32 iLight)
{
    float4 wuv = SelectEmissiveCoord(scene, rng, iLight);

    float4 A = scene->positions[iLight + 0];
    float4 B = scene->positions[iLight + 1];
    float4 C = scene->positions[iLight + 2];
    float4 pt = f4_blend(A, B, C, wuv);
    return f4_normalize3(f4_sub(pt, surf->P));
}

static float VEC_CALL LightEvalPdf(
    const pt_scene_t* scene,
    prng_t* rng,
    const surfhit_t* surf,
    i32 iLight,
    float4* wuvOut,
    float4 L)
{
    ray_t ray = { surf->P, L };
    rayhit_t hit = TraceRay(scene, ray, 1 << 20);
    if (hit.index != iLight)
    {
        return 0.0f;
    }

    *wuvOut = hit.wuvt;

    float4 A = scene->positions[iLight + 0];
    float4 B = scene->positions[iLight + 1];
    float4 C = scene->positions[iLight + 2];
    float4 N = f4_cross3(f4_sub(B, A), f4_sub(C, A));
    float area = f4_length3(N);
    N = f4_divvs(N, area);
    area *= 0.5f;

    float t = hit.wuvt.w;
    float cosTheta = f4_dotsat(N, f4_neg(L));
    float pdf = LightPdf(area, cosTheta, t * t);

    return pdf;
}

static float4 VEC_CALL EstimateDirect(
    const pt_scene_t* scene,
    prng_t* rng,
    const surfhit_t* surf,
    const rayhit_t* hit,
    i32 iLight,
    float4 I)
{
    float4 L = LightSample(scene, rng, surf, iLight);
    float4 wuv;
    float lightPdf = LightEvalPdf(scene, rng, surf, iLight, &wuv, L);
    if (lightPdf > kEpsilon)
    {
        float brdfPdf = BrdfEvalPdf(I, surf, L);
        if (brdfPdf > kEpsilon)
        {
            float weight = PowerHeuristic(1, lightPdf, 1, brdfPdf);
            float4 attenuation = BrdfEval(I, surf, L);
            attenuation = f4_mulvs(attenuation, weight);
            float4 irradiance = LightRad(scene, iLight, wuv, surf->P, L);
            float4 Li = f4_divvs(f4_mul(irradiance, attenuation), lightPdf);
            return Li;
        }
    }
    return f4_0;
}

static float4 VEC_CALL SampleLights(
    const pt_scene_t* scene,
    prng_t* rng,
    const surfhit_t* surf,
    const rayhit_t* hit,
    float4 I)
{
    float4 light = f4_0;
    const i32 kLightSamples = 3;
    i32 iLight;
    float lightPdfWeight;
    for (i32 i = 0; i < kLightSamples; ++i)
    {
        if (LightSelect(scene, rng, &iLight, &lightPdfWeight))
        {
            if (hit->index != iLight)
            {
                float4 direct = EstimateDirect(
                    scene, rng, surf, hit, iLight, I);
                lightPdfWeight *= kLightSamples;
                direct = f4_divvs(direct, lightPdfWeight);
                light = f4_add(light, direct);
            }
        }
    }
    return light;
}

static float4 VEC_CALL pt_trace_albedo(
    const pt_scene_t* scene,
    ray_t ray)
{
    rayhit_t hit = TraceRay(scene, ray, 1 << 20);
    if (hit.type != hit_nothing)
    {
        surfhit_t surf = GetSurface(scene, ray, hit, 0);
        return surf.albedo;
    }
    return f4_0;
}

pt_result_t VEC_CALL pt_trace_ray(
    prng_t* rng,
    const pt_scene_t* scene,
    ray_t ray)
{
    float4 albedo = f4_0;
    float4 normal = f4_0;
    float4 light = f4_0;
    float4 attenuation = f4_1;

    for (i32 b = 0; b < 20; ++b)
    {
        rayhit_t hit = TraceRay(scene, ray, 1 << 20);
        if (hit.type == hit_nothing)
        {
            break;
        }

        surfhit_t surf = GetSurface(scene, ray, hit, b);
        if (b == 0)
        {
            albedo = surf.albedo;
            normal = surf.N;
        }

        if (hit.type == hit_sky)
        {
            light = f4_add(light, f4_mul(surf.emission, attenuation));
            break;
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
        if (scatter.pdf < kEpsilon)
        {
            break;
        }

        attenuation = f4_mul(
            attenuation,
            f4_divvs(scatter.attenuation, scatter.pdf));

        ray.ro = surf.P;
        ray.rd = scatter.dir;

        {
            float p = f4_perlum(attenuation);
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

    pt_result_t result;
    result.color = f4_f3(light);
    result.albedo = f4_f3(albedo);
    result.normal = f4_f3(normal);
    return result;
}

static void TraceFn(task_t* pbase, i32 begin, i32 end)
{
    trace_task_t* task = (trace_task_t*)pbase;

    const pt_scene_t* scene = task->trace.scene;

    float3* pim_noalias color = task->trace.color;
    float3* pim_noalias albedo = task->trace.albedo;
    float3* pim_noalias normal = task->trace.normal;

    const int2 size = task->trace.imageSize;
    const float sampleWeight = task->trace.sampleWeight;
    const camera_t camera = task->camera;
    const quat rot = camera.rotation;
    const float4 ro = camera.position;
    const float4 right = quat_right(rot);
    const float4 up = quat_up(rot);
    const float4 fwd = quat_fwd(rot);
    const float2 slope = proj_slope(f1_radians(camera.fovy), (float)size.x / (float)size.y);

    prng_t rng = prng_get();

    const float2 rcpSize = f2_rcp(i2_f2(size));
    for (i32 i = begin; i < end; ++i)
    {
        int2 coord = { i % size.x, i / size.x };

        float2 Xi = f2_tent(f2_rand(&rng));
        Xi = f2_mul(Xi, rcpSize);

        float2 uv = CoordToUv(size, coord);
        uv = f2_add(uv, Xi);
        uv = f2_snorm(uv);

        float4 rd = proj_dir(right, up, fwd, slope, uv);
        ray_t ray = { ro, rd };
        pt_result_t result = pt_trace_ray(&rng, scene, ray);
        color[i] = f3_lerp(color[i], result.color, sampleWeight);
        albedo[i] = f3_lerp(albedo[i], result.albedo, sampleWeight);
        normal[i] = f3_lerp(normal[i], result.normal, sampleWeight);
        normal[i] = f3_normalize(normal[i]);
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

    UpdateSun(desc->scene);

    trace_task_t* task = tmp_calloc(sizeof(*task));
    task->trace = *desc;
    task->camera = desc->camera[0];

    const i32 workSize = desc->imageSize.x * desc->imageSize.y;
    task_run(&task->task, TraceFn, workSize);

    ProfileEnd(pm_trace);
}

static void RayGenFn(task_t* pBase, i32 begin, i32 end)
{
    prng_t rng = prng_get();

    pt_raygen_t* task = (pt_raygen_t*)pBase;
    const pt_scene_t* scene = task->scene;
    const pt_dist_t dist = task->dist;
    const ray_t origin = task->origin;
    const float3x3 TBN = NormalToTBN(origin.rd);

    float4* pim_noalias colors = task->colors;
    float4* pim_noalias directions = task->directions;

    ray_t ray;
    ray.ro = origin.ro;

    for (i32 i = begin; i < end; ++i)
    {
        float2 Xi = f2_rand(&rng);
        switch (dist)
        {
        default:
        case ptdist_sphere:
            ray.rd = SampleUnitSphere(Xi);
            break;
        case ptdist_hemi:
            ray.rd = TbnToWorld(TBN, SampleUnitHemisphere(Xi));
            break;
        }
        directions[i] = ray.rd;
        pt_result_t result = pt_trace_ray(&rng, scene, ray);
        colors[i] = f3_f4(result.color, 1.0f);
    }

    prng_set(rng);
}

ProfileMark(pm_raygen, pt_raygen)
pt_results_t pt_raygen(
    pt_scene_t* scene,
    ray_t origin,
    pt_dist_t dist,
    i32 count)
{
    ProfileBegin(pm_raygen);

    ASSERT(scene);
    ASSERT(count >= 0);

    UpdateSun(scene);

    pt_raygen_t* task = tmp_calloc(sizeof(*task));
    task->scene = scene;
    task->origin = origin;
    task->colors = tmp_malloc(sizeof(task->colors[0]) * count);
    task->directions = tmp_malloc(sizeof(task->directions[0]) * count);
    task->dist = dist;
    task_run(&task->task, RayGenFn, count);

    pt_results_t results =
    {
        .colors = task->colors,
        .directions = task->directions,
    };

    ProfileEnd(pm_raygen);

    return results;
}
