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
#include "ui/cimgui.h"

#include "stb/stb_perlin_fork.h"
#include <string.h>

#define kPixelRadius    2.0f

// ----------------------------------------------------------------------------

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
    i32 flags;
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
    float4 logConstantAlbedo;
    float4 logNoiseAlbedo;
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

    cubemap_t* sky;

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
pim_inline RTCRay VEC_CALL RtcNewRay(ray_t ray, float tNear, float tFar);
pim_inline RTCRayHit VEC_CALL RtcIntersect(
    RTCScene scene,
    ray_t ray,
    float tNear,
    float tFar);
static RTCScene RtcNewScene(const pt_scene_t* scene);
static void FlattenDrawables(pt_scene_t* scene);
static float EmissionPdf(
    pt_sampler_t* sampler,
    const pt_scene_t* scene,
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
    rayhit_t hit);
pim_inline rayhit_t VEC_CALL pt_intersect_local(
    const pt_scene_t* scene,
    ray_t ray,
    float tNear,
    float tFar);
pim_inline float4 VEC_CALL SampleSpecular(
    pt_sampler_t* sampler,
    float4 I,
    float4 N,
    float alpha);
pim_inline float4 VEC_CALL SampleDiffuse(
    pt_sampler_t* sampler,
    float4 N);
pim_inline float4 VEC_CALL RefractBrdfEval(
    pt_sampler_t* sampler,
    float4 I,
    const surfhit_t* surf,
    float4 L);
pim_inline float4 VEC_CALL BrdfEval(
    pt_sampler_t* sampler,
    float4 I,
    const surfhit_t* surf,
    float4 L);
pim_inline scatter_t VEC_CALL RefractScatter(
    pt_sampler_t* sampler,
    const surfhit_t* surf,
    float4 I);
pim_inline scatter_t VEC_CALL BrdfScatter(
    pt_sampler_t* sampler,
    const surfhit_t* surf,
    float4 I);
pim_inline bool VEC_CALL LightSelect(
    pt_sampler_t* sampler,
    const pt_scene_t* scene,
    float4 position,
    i32* iVertOut,
    float* pdfOut);
pim_inline lightsample_t VEC_CALL LightSample(
    pt_sampler_t* sampler,
    const pt_scene_t* scene,
    float4 ro,
    i32 iLight);
pim_inline float VEC_CALL LightEvalPdf(
    pt_sampler_t* sampler,
    const pt_scene_t* scene,
    float4 ro,
    float4 rd,
    rayhit_t* hitOut);
pim_inline float4 VEC_CALL EstimateDirect(
    pt_sampler_t* sampler,
    const pt_scene_t* scene,
    const surfhit_t* surf,
    i32 iLight,
    float selectPdf,
    float4 I);
pim_inline float4 VEC_CALL SampleLights(
    pt_sampler_t* sampler,
    const pt_scene_t* scene,
    const surfhit_t* surf,
    const rayhit_t* hit,
    float4 I);
pim_inline bool VEC_CALL EvaluateLight(
    pt_sampler_t* sampler,
    const pt_scene_t* scene,
    float4 P,
    float4* lightOut,
    float4* dirOut);
static void media_desc_new(media_desc_t* desc);
static void media_desc_update(media_desc_t* desc);
static void media_desc_gui(media_desc_t* desc);
static void media_desc_load(media_desc_t* desc, const char* name);
static void media_desc_save(const media_desc_t* desc, const char* name);
pim_inline float4 VEC_CALL AlbedoToLogAlbedo(float4 albedo);
pim_inline float2 VEC_CALL Media_Density(const media_desc_t* desc, float4 P);
pim_inline media_t VEC_CALL Media_Sample(const media_desc_t* desc, float4 P);
pim_inline float4 VEC_CALL Media_Albedo(const media_desc_t* desc, float4 P);
pim_inline float4 VEC_CALL Media_Normal(const media_desc_t* desc, float4 P);
pim_inline media_t VEC_CALL Media_Lerp(media_t lhs, media_t rhs, float t);
pim_inline i32 VEC_CALL CalcStepCount(float rayLen);
pim_inline float4 VEC_CALL CalcTransmittance(
    pt_sampler_t* sampler,
    const pt_scene_t* scene,
    float4 ro,
    float4 rd,
    float rayLen);
pim_inline scatter_t VEC_CALL ScatterRay(
    pt_sampler_t* sampler,
    const pt_scene_t* scene,
    float4 ro,
    float4 rd,
    float rayLen);
static void TraceFn(task_t* pbase, i32 begin, i32 end);
static void RayGenFn(task_t* pBase, i32 begin, i32 end);
pim_inline float VEC_CALL Sample1D(pt_sampler_t* sampler);
pim_inline float2 VEC_CALL Sample2D(pt_sampler_t* sampler);
pim_inline pt_sampler_t VEC_CALL GetSampler(void);
pim_inline void VEC_CALL SetSampler(pt_sampler_t sampler);

// ----------------------------------------------------------------------------

static cvar_t* cv_pt_lgrid_mpc;

static RTCDevice ms_device;
static dist1d_t ms_pixeldist;
static pt_sampler_t ms_samplers[256];

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
        float x = f1_lerp(0.0f, kPixelRadius, i * dt);
        dist.pdf[i] = f1_gauss(x);
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
    }
    prng_set(rng);
}

void pt_sys_init(void)
{
    cv_pt_lgrid_mpc = cvar_find("pt_lgrid_mpc");

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
float2 VEC_CALL pt_sample_2d(pt_sampler_t* sampler) { return Sample2D(sampler); }
float VEC_CALL pt_sample_1d(pt_sampler_t* sampler) { return Sample1D(sampler); }

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
                float4 uva = mesh.uvs[j + 0];
                float4 uvb = mesh.uvs[j + 1];
                float4 uvc = mesh.uvs[j + 2];
                float2 UA = f2_v(uva.x, uva.y);
                float2 UB = f2_v(uvb.x, uvb.y);
                float2 UC = f2_v(uvc.x, uvc.y);
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
    pt_sampler_t* sampler,
    const pt_scene_t* scene,
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
    float4 rome = mat->flatRome;
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
                float4 wuv = SampleBaryCoord(Sample2D(sampler));
                float2 uv = f2_blend(UA, UB, UC, wuv);
                float sample = UvWrap_c32(romeMap.texels, romeMap.size, uv).w;
                float em = sample * rome.w;
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

    pt_sampler_t sampler = pt_sampler_get();
    for (i32 i = begin; i < end; ++i)
    {
        pdfs[i] = EmissionPdf(&sampler, scene, i * 3, attempts);
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
    const float metersPerCell = cvar_get_float(cv_pt_lgrid_mpc);
    const float minDist = f1_max(metersPerCell * 0.5f, kMinLightDist);
    const float minDistSq = minDist * minDist;

    for (i32 i = begin; i < end; ++i)
    {
        dist1d_t dist;
        dist1d_new(&dist, emissiveCount);

        float4 position = grid_position(&grid, i);
        for (i32 iList = 0; iList < emissiveCount; ++iList)
        {
            i32 iVert = emissives[iList];
            i32 iMat = matIds[iVert];
            float4 A = positions[iVert + 0];
            float4 B = positions[iVert + 1];
            float4 C = positions[iVert + 2];
            float distance = sdTriangle3D(A, B, C, position);
            float distSq = distance * distance;
            distSq = f1_max(minDistSq, distSq - metersPerCell * 0.5f);
            float power = 1.0f / distSq;

            dist.pdf[iList] = power * weight;
        }

        dist1d_bake(&dist);
        dists[i] = dist;
    }
}

static void SetupLightGrid(pt_scene_t* scene)
{
    if (scene->vertCount > 0)
    {
        box_t bounds = box_from_pts(scene->positions, scene->vertCount);
        grid_t grid;
        const float metersPerCell = cvar_get_float(cv_pt_lgrid_mpc);
        grid_new(&grid, bounds, 1.0f / metersPerCell);
        const i32 len = grid_len(&grid);
        scene->lightGrid = grid;
        scene->lightDists = perm_calloc(sizeof(scene->lightDists[0]) * len);

        task_SetupLightGrid* task = tmp_calloc(sizeof(*task));
        task->scene = scene;

        task_run(&task->task, SetupLightGridFn, len);
    }
}

static void UpdateScene(pt_scene_t* scene)
{
    guid_t skyname = guid_str("sky", guid_seed);
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

void pt_scene_gui(pt_scene_t* scene)
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
    pt_scene_t* scene,
    const camera_t* camera,
    int2 imageSize)
{
    ASSERT(trace);
    ASSERT(scene);
    ASSERT(camera);
    if (trace)
    {
        const i32 texelCount = imageSize.x * imageSize.y;
        ASSERT(texelCount > 0);

        trace->imageSize = imageSize;
        trace->camera = camera;
        trace->scene = scene;
        trace->sampleWeight = 1.0f;
        trace->color = perm_calloc(sizeof(trace->color[0]) * texelCount);
        trace->albedo = perm_calloc(sizeof(trace->albedo[0]) * texelCount);
        trace->normal = perm_calloc(sizeof(trace->normal[0]) * texelCount);
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
        dof->aperture = 25.0f * kMilli;
        dof->focalLength = 4.0f;
        dof->bladeCount = 5;
        dof->bladeRot = kPi / 10.0f;
        dof->focalPlaneCurvature = 0.1f;
    }
}

void dofinfo_gui(dofinfo_t* dof)
{
    if (dof && igCollapsingHeader1("dofinfo"))
    {
        float aperture = dof->aperture / kMilli;
        igSliderFloat("Aperture, millimeters", &aperture, 0.1f, 300.0f);
        dof->aperture = aperture * kMilli;
        igSliderFloat("Focal Length, meters", &dof->focalLength, 0.01f, 100.0f);
        igSliderFloat("Focal Plane Curvature", &dof->focalPlaneCurvature, 0.0f, 1.0f);
        igSliderInt("Blade Count", &dof->bladeCount, 3, 666, "%d");
        igSliderFloat("Blade Rotation", &dof->bladeRot, 0.0f, kTau);
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

pim_inline float VEC_CALL GetArea(const pt_scene_t* scene, i32 iVert)
{
    const float4* pim_noalias positions = scene->positions;
    return TriArea3D(positions[iVert + 0], positions[iVert + 1], positions[iVert + 2]);
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
    rayhit_t hit)
{
    surfhit_t surf = { 0 };

    const material_t* mat = GetMaterial(scene, hit);
    surf.flags = mat->flags;
    surf.ior = mat->ior;
    float2 uv = GetVert2(scene->uvs, hit.index, hit.wuvt);
    surf.M = f4_normalize3(GetVert4(scene->normals, hit.index, hit.wuvt));
    surf.N = surf.M;
    surf.P = f4_add(rin.ro, f4_mulvs(rin.rd, hit.wuvt.w));
    surf.P = f4_add(surf.P, f4_mulvs(surf.M, kMilli));

    texture_t tex;
    if (texture_get(mat->normal, &tex))
    {
        float4 Nts = UvBilinearWrap_dir8(tex.texels, tex.size, uv);
        surf.N = TanToWorld(surf.N, Nts);
    }

    surf.albedo = mat->flatAlbedo;
    if (texture_get(mat->albedo, &tex))
    {
        float4 sample = UvBilinearWrap_c32(tex.texels, tex.size, uv);
        surf.albedo = f4_mul(surf.albedo, sample);
    }

    float4 rome = mat->flatRome;
    if (texture_get(mat->rome, &tex))
    {
        float4 sample = UvBilinearWrap_c32(tex.texels, tex.size, uv);
        rome = f4_mul(rome, sample);
    }
    surf.emission = UnpackEmission(surf.albedo, rome.w);
    surf.roughness = rome.x;
    surf.occlusion = rome.y;
    surf.metallic = rome.z;

    if (hit.flags & matflag_sky)
    {
        surf.emission = GetSky(scene, rin.ro, rin.rd);
    }

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
    hit.type = hit_triangle;
    if (f4_dot3(hit.normal, ray.rd) > 0.0f)
    {
        hit.type = hit_backface;
    }
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
    float2 Xi = Sample2D(sampler);
    float4 H = TanToWorld(N, SampleGGXMicrofacet(Xi, alpha));
    float4 L = f4_reflect3(I, H);
    ASSERT(IsUnitLength(I));
    ASSERT(IsUnitLength(N));
    ASSERT(IsUnitLength(L));
    ASSERT(IsUnitLength(H));
    return L;
}

pim_inline float4 VEC_CALL SampleDiffuse(
    pt_sampler_t* sampler,
    float4 N)
{
    float2 Xi = Sample2D(sampler);
    float4 L = TanToWorld(N, SampleCosineHemisphere(Xi));
    ASSERT(IsUnitLength(N));
    ASSERT(IsUnitLength(L));
    return L;
}

pim_inline float VEC_CALL Schlick(float cosTheta, float k)
{
    float r0 = (1.0f - k) / (1.0f + k);
    r0 = r0 * r0;
    float t = 1.0f - cosTheta;
    t = t * t * t * t * t;
    return f1_lerp(r0, 1.0f, t);
}

pim_inline float4 VEC_CALL RefractBrdfEval(
    pt_sampler_t* sampler,
    float4 I,
    const surfhit_t* surf,
    float4 L)
{
    return f4_0;
}

// returns attenuation value for the given interaction
pim_inline float4 VEC_CALL BrdfEval(
    pt_sampler_t* sampler,
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
    float LoH = f4_dotsat(L, H);

    // metals are only specular
    const float amtDiffuse = 1.0f - metallic;
    const float amtSpecular = 1.0f;

    float4 brdf = f4_0;
    {
        float4 F = F_SchlickEx(albedo, metallic, HoV);
        float D = D_GTR(NoH, alpha);
        float G = G_SmithGGX(NoL, NoV, alpha);
        float4 Fr = f4_mulvs(F, D * G);
        brdf = f4_add(brdf, f4_mulvs(Fr, amtSpecular));
    }
    {
        float4 Fd = f4_mulvs(albedo, Fd_Burley(NoL, NoV, LoH, roughness));
        brdf = f4_add(brdf, f4_mulvs(Fd, amtDiffuse));
    }

    brdf = f4_mulvs(brdf, NoL);

    float diffusePdf = amtDiffuse * LambertPdf(NoL);
    float specularPdf = amtSpecular * GGXPdf(NoH, HoV, alpha);
    float pdf = diffusePdf + specularPdf;

    brdf.w = pdf;
    return brdf;
}

pim_inline scatter_t VEC_CALL RefractScatter(
    pt_sampler_t* sampler,
    const surfhit_t* surf,
    float4 I)
{
    const float kAir = 1.000277f;
    const float matIor = surf->ior;

    float4 V = f4_neg(I);
    float4 N = surf->N;
    float4 M = surf->M;
    float NoV = f4_dot3(N, V);
    float sinTheta = sqrtf(f1_max(0.0f, 1.0f - NoV * NoV));
    bool entering = NoV > 0.0f;
    float etaI = entering ? kAir : matIor;
    float etaT = entering ? matIor : kAir;
    float k = etaI / etaT;
    float F = Schlick(NoV, k);

    float4 R;
    if (((k*sinTheta) > 1.0f) || (Sample1D(sampler) < F))
    {
        R = f4_reflect3(I, N);
    }
    else
    {
        R = f4_refract3(I, N, k);
        F = 1.0f - F;
    }

    bool above = f4_dot3(R, M) > 0.0f;
    float4 N2 = above ? N : f4_neg(N);
    float4 P = above ? surf->P : f4_add(surf->P, f4_mulvs(M, -3.0f * kMilli));
    float4 L = SampleDiffuse(sampler, N2);
    float alpha = BrdfAlpha(surf->roughness);
    L = f4_normalize3(f4_lerpvs(R, L, alpha));
    float NoL = f1_max(f1_abs(f4_dot3(N, L)), kEpsilon);
    float pdf = F * f1_lerp(1.0f, LambertPdf(NoL), alpha);


    float4 albedo = surf->albedo;
    float hi = f4_hmax3(albedo) + kEpsilon;
    float4 norm = f4_mulvs(albedo, 1.0f / hi);
    albedo = f4_lerpvs(f4_s(0.5f), norm, 0.5f);

    float4 att = f4_saturate(f4_mulvs(albedo, F / NoL));

    scatter_t result = { 0 };
    result.pos = P;
    result.dir = L;
    result.pdf = pdf;
    result.attenuation = att;

    return result;
}

pim_inline scatter_t VEC_CALL BrdfScatter(
    pt_sampler_t* sampler,
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
    if (Sample1D(sampler) < rcpProb)
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
    float LoH = f4_dotsat(L, H);

    float specularPdf = amtSpecular * GGXPdf(NoH, HoV, alpha);
    float diffusePdf = amtDiffuse * LambertPdf(NoL);
    float pdf = diffusePdf + specularPdf;

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
        float4 Fd = f4_mulvs(albedo, Fd_Burley(NoL, NoV, LoH, roughness));
        brdf = f4_add(brdf, f4_mulvs(Fd, amtDiffuse));
    }

    result.dir = L;
    result.pdf = pdf;
    result.attenuation = f4_mulvs(brdf, NoL);

    return result;
}

pim_inline bool VEC_CALL LightSelect(
    pt_sampler_t* sampler,
    const pt_scene_t* scene,
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

    i32 iList = dist1d_sampled(dist, Sample1D(sampler));
    float pdf = dist1d_pdfd(dist, iList);

    i32 iVert = scene->emissives[iList];

    *iVertOut = iVert;
    *pdfOut = pdf;
    return true;
}

pim_inline lightsample_t VEC_CALL LightSample(
    pt_sampler_t* sampler,
    const pt_scene_t* scene,
    float4 ro,
    i32 iLight)
{
    lightsample_t sample = { 0 };

    float4 wuv = SampleBaryCoord(Sample2D(sampler));

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
    if (VoNl > 0.0f)
    {
        ray_t ray = { ro, rd };
        rayhit_t hit = pt_intersect_local(scene, ray, 0.0f, 1 << 20);
        if ((hit.type != hit_nothing) && (hit.index == iLight))
        {
            sample.pdf = LightPdf(area, VoNl, distSq);
            surfhit_t surf = GetSurface(scene, ray, hit);
            sample.irradiance = surf.emission;
            float4 Tr = CalcTransmittance(sampler, scene, ro, rd, hit.wuvt.w);
            sample.irradiance = f4_mul(sample.irradiance, Tr);
        }
    }

    return sample;
}

pim_inline float VEC_CALL LightEvalPdf(
    pt_sampler_t* sampler,
    const pt_scene_t* scene,
    float4 ro,
    float4 rd,
    rayhit_t* hitOut)
{
    ASSERT(IsUnitLength(rd));
    ray_t ray = { ro, rd };
    rayhit_t hit = pt_intersect_local(scene, ray, 0.0f, 1 << 20);
    *hitOut = hit;
    if ((hit.type != hit_nothing))
    {
        float distance = hit.wuvt.w;
        if (distance > 0.0f)
        {
            float4 N = f4_normalize3(hit.normal);
            float cosTheta = f4_dot3(f4_neg(rd), N);
            if (cosTheta > 0.0f)
            {
                float area = GetArea(scene, hit.index);
                return LightPdf(area, cosTheta, distance * distance);
            }
        }
    }
    return 0.0f;
}

pim_inline float4 VEC_CALL EstimateDirect(
    pt_sampler_t* sampler,
    const pt_scene_t* scene,
    const surfhit_t* surf,
    i32 iLight,
    float selectPdf,
    float4 I)
{
    float4 result = f4_0;
    {
        lightsample_t sample = LightSample(sampler, scene, surf->P, iLight);
        float lightPdf = sample.pdf;
        if (lightPdf > 0.0f)
        {
            float4 brdf = BrdfEval(sampler, I, surf, sample.direction);
            float brdfPdf = brdf.w;
            if (brdfPdf > 0.0f)
            {
                float weight = PowerHeuristic(lightPdf, brdfPdf) * 0.5f;
                ray_t ray = { surf->P, sample.direction };
                float4 Tr = CalcTransmittance(sampler, scene, ray.ro, ray.rd, sample.wuvt.w);
                float4 Li = sample.irradiance;
                Li = f4_mulvs(Li, weight);
                Li = f4_mul(Li, Tr);
                Li = f4_mul(Li, brdf);
                Li = f4_divvs(Li, lightPdf * selectPdf);
                result = f4_add(result, Li);
            }
        }
    }
    {
        scatter_t sample = BrdfScatter(sampler, surf, I);
        float brdfPdf = sample.pdf;
        if (brdfPdf > 0.0f)
        {
            ray_t ray = { surf->P, sample.dir };
            rayhit_t hit = { 0 };
            float lightPdf = LightEvalPdf(sampler, scene, ray.ro, ray.rd, &hit);
            if (lightPdf > 0.0f)
            {
                float weight = PowerHeuristic(brdfPdf, lightPdf) * 0.5f;
                float4 Tr = CalcTransmittance(sampler, scene, ray.ro, ray.rd, hit.wuvt.w);
                surfhit_t surf = GetSurface(scene, ray, hit);
                float4 Li = surf.emission;
                Li = f4_mulvs(Li, weight);
                Li = f4_mul(Li, Tr);
                Li = f4_mul(Li, sample.attenuation);
                Li = f4_divvs(Li, brdfPdf);
                result = f4_add(result, Li);
            }
        }
    }
    return result;
}

pim_inline float4 VEC_CALL SampleLights(
    pt_sampler_t* sampler,
    const pt_scene_t* scene,
    const surfhit_t* surf,
    const rayhit_t* hit,
    float4 I)
{
    float4 sum = f4_0;
    i32 iLight;
    float selectPdf;
    if (LightSelect(sampler, scene, surf->P, &iLight, &selectPdf))
    {
        if (hit->index != iLight)
        {
            float4 direct = EstimateDirect(sampler, scene, surf, iLight, selectPdf, I);
            sum = f4_add(sum, direct);
        }
    }
    return sum;
}

pim_inline bool VEC_CALL EvaluateLight(
    pt_sampler_t* sampler,
    const pt_scene_t* scene,
    float4 P,
    float4* lightOut,
    float4* dirOut)
{
    i32 iLight;
    float lightPdf;
    if (LightSelect(sampler, scene, P, &iLight, &lightPdf))
    {
        lightsample_t sample = LightSample(sampler, scene, P, iLight);
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

static void media_desc_update(media_desc_t* desc)
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
}

static void media_desc_load(media_desc_t* desc, const char* name)
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
        ser_getfield_f4(desc, root, constantAlbedo);
        ser_getfield_f4(desc, root, noiseAlbedo);
        ser_getfield_f32(desc, root, absorption);
        ser_getfield_f32(desc, root, constantAmt);
        ser_getfield_f32(desc, root, noiseAmt);
        ser_getfield_i32(desc, root, noiseOctaves);
        ser_getfield_f32(desc, root, noiseGain);
        ser_getfield_f32(desc, root, noiseLacunarity);
        ser_getfield_f32(desc, root, noiseFreq);
        ser_getfield_f32(desc, root, noiseScale);
        ser_getfield_f32(desc, root, noiseHeight);
        ser_getfield_f32(desc, root, phaseDirA);
        ser_getfield_f32(desc, root, phaseDirB);
        ser_getfield_f32(desc, root, phaseBlend);
    }
    else
    {
        con_logf(LogSev_Error, "pt", "Failed to load media desc '%s'", filename);
        media_desc_new(desc);
    }
    ser_obj_del(root);
}

static void media_desc_save(const media_desc_t* desc, const char* name)
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
        ser_setfield_f4(desc, root, constantAlbedo);
        ser_setfield_f4(desc, root, noiseAlbedo);
        ser_setfield_f32(desc, root, absorption);
        ser_setfield_f32(desc, root, constantAmt);
        ser_setfield_f32(desc, root, noiseAmt);
        ser_setfield_i32(desc, root, noiseOctaves);
        ser_setfield_f32(desc, root, noiseGain);
        ser_setfield_f32(desc, root, noiseLacunarity);
        ser_setfield_f32(desc, root, noiseFreq);
        ser_setfield_f32(desc, root, noiseScale);
        ser_setfield_f32(desc, root, noiseHeight);
        ser_setfield_f32(desc, root, phaseDirA);
        ser_setfield_f32(desc, root, phaseDirB);
        ser_setfield_f32(desc, root, phaseBlend);
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

pim_inline float2 VEC_CALL Media_Density(const media_desc_t* desc, float4 P)
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

pim_inline media_t VEC_CALL Media_Sample(const media_desc_t* desc, float4 P)
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

pim_inline float4 VEC_CALL Media_Albedo(const media_desc_t* desc, float4 P)
{
    return f4_inv(Media_Sample(desc, P).logAlbedo);
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
pim_inline float4 VEC_CALL CalcMajorant(const media_desc_t* desc)
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

pim_inline float4 VEC_CALL CalcControl(const media_desc_t* desc)
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
    return f4_lerpvs(ca, na, 0.5f);
}

pim_inline float4 VEC_CALL CalcResidual(float4 control, float4 majorant)
{
    return f4_abs(f4_sub(majorant, control));
}

pim_inline float VEC_CALL CalcPhase(
    const media_desc_t* desc,
    float cosTheta)
{
    return f1_lerp(MiePhase(cosTheta, desc->phaseDirA), MiePhase(cosTheta, desc->phaseDirB), desc->phaseBlend);
}

pim_inline bool VEC_CALL RussianRoulette(
    pt_sampler_t* sampler,
    float4* pAttenuation)
{
    float4 att = *pAttenuation;
    float p = f1_sat(f4_avglum(att));
    bool kept = Sample1D(sampler) < p;
    if (kept)
    {
        att = f4_divvs(att, p);
    }
    else
    {
        att = f4_0;
    }
    *pAttenuation = att;
    return kept;
}

pim_inline float4 VEC_CALL CalcTransmittance(
    pt_sampler_t* sampler,
    const pt_scene_t* scene,
    float4 ro,
    float4 rd,
    float rayLen)
{
    const media_desc_t* desc = &scene->mediaDesc;
    const float4 u = CalcMajorant(desc);
    const float rcpUmax = 1.0f / f4_hmax3(u);
    float4 attenuation = f4_1;
    float t = 0.0f;
    while (true)
    {
        float4 P = f4_add(ro, f4_mulvs(rd, t));
        float dt = SampleFreePath(Sample1D(sampler), rcpUmax);
        t += dt;
        if (t >= rayLen)
        {
            break;
        }
        media_t media = Media_Sample(desc, P);
        float4 uT = Media_Extinction(media);
        float4 ratio = f4_inv(f4_mulvs(uT, rcpUmax));
        attenuation = f4_mul(attenuation, ratio);
    }
    return attenuation;
}

pim_inline float4 VEC_CALL SamplePhaseDir(
    pt_sampler_t* sampler,
    const media_desc_t* desc,
    float4 rd)
{
#if 0
    float4 L = SampleUnitSphere(Sample2D(sampler));
    L.w = 1.0f / (4.0f * kPi);
#else
    const float Mg = (4.0f * kPi) / 3.0f;
    float4 L;
    do
    {
        L = SampleUnitSphere(Sample2D(sampler));
        L.w = CalcPhase(desc, f4_dot3(rd, L));
    } while (Sample1D(sampler) > (L.w * Mg));
#endif
    return L;
}

pim_inline scatter_t VEC_CALL ScatterRay(
    pt_sampler_t* sampler,
    const pt_scene_t* scene,
    float4 ro,
    float4 rd,
    float rayLen)
{
    scatter_t result = { 0 };
    result.pdf = 0.0f;

    const media_desc_t* desc = &scene->mediaDesc;
    const float4 u = CalcMajorant(desc);
    const float rcpUmax = 1.0f / f4_hmax3(u);

    float t = 0.0f;
    result.attenuation = f4_1;
    while (true)
    {
        float4 P = f4_add(ro, f4_mulvs(rd, t));
        float dt = SampleFreePath(Sample1D(sampler), rcpUmax);
        t += dt;
        if (t >= rayLen)
        {
            break;
        }

        media_t media = Media_Sample(desc, P);

        float uS = Media_Scattering(media);
        float pScatter = uS * rcpUmax;
        bool scattered = Sample1D(sampler) < pScatter;
        if (scattered)
        {
            float4 rad;
            float4 L;
            if (EvaluateLight(sampler, scene, P, &rad, &L))
            {
                float ph = CalcPhase(desc, f4_dot3(rd, L));
                rad = f4_mulvs(rad, ph * dt);
                rad = f4_mul(rad, result.attenuation);
                result.irradiance = rad;
            }

            result.pos = f4_add(ro, f4_mulvs(rd, t));
            result.dir = SamplePhaseDir(sampler, desc, rd);
            result.pdf = result.dir.w;
            float ph = CalcPhase(desc, f4_dot3(rd, result.dir));
            result.attenuation = f4_mulvs(result.attenuation, ph);
    }

        float4 uT = Media_Extinction(media);
        float4 ratio = f4_inv(f4_mulvs(uT, rcpUmax));
        result.attenuation = f4_mul(result.attenuation, ratio);

        if (scattered)
        {
            break;
        }
}

    return result;
}

pt_result_t VEC_CALL pt_trace_ray(
    pt_sampler_t* sampler,
    const pt_scene_t* scene,
    ray_t ray)
{
    pt_result_t result = { 0 };
    float4 light = f4_0;
    float4 attenuation = f4_1;
    u32 prevFlags = 0;

    for (i32 b = 0; b < 666; ++b)
    {
        if (!RussianRoulette(sampler, &attenuation))
        {
            break;
        }

        rayhit_t hit = pt_intersect_local(scene, ray, 0.0f, 1 << 20);
        if (hit.type == hit_nothing)
        {
            break;
        }

        {
            scatter_t scatter = ScatterRay(sampler, scene, ray.ro, ray.rd, hit.wuvt.w);
            light = f4_add(light, f4_mul(scatter.irradiance, attenuation));
            if (scatter.pdf > 0.0f)
            {
                if (b == 0)
                {
                    result.albedo = f4_f3(Media_Albedo(&scene->mediaDesc, scatter.pos));
                    result.normal = f4_f3(f4_neg(ray.rd));
                }
                attenuation = f4_mul(attenuation, f4_divvs(scatter.attenuation, scatter.pdf));
                ray.ro = scatter.pos;
                ray.rd = scatter.dir;
                continue;
            }
            else
            {
                attenuation = f4_mul(attenuation, scatter.attenuation);
            }
        }

        if (hit.flags & matflag_sky)
        {
            light = f4_add(light, f4_mul(GetSky(scene, ray.ro, ray.rd), attenuation));
            break;
        }

        surfhit_t surf = GetSurface(scene, ray, hit);
        if (b == 0)
        {
            float t = f4_avglum(attenuation);
            float4 N = f4_lerpvs(f4_neg(ray.rd), surf.N, t);
            result.albedo = f4_f3(surf.albedo);
            result.normal = f4_f3(N);
        }

        if ((b == 0) || (prevFlags & matflag_refractive))
        {
            light = f4_add(light, f4_mul(surf.emission, attenuation));
        }
        {
            float4 direct = SampleLights(sampler, scene, &surf, &hit, ray.rd);
            light = f4_add(light, f4_mul(direct, attenuation));
        }

        scatter_t scatter = BrdfScatter(sampler, &surf, ray.rd);
        if (scatter.pdf <= 0.0f)
        {
            break;
        }
        ray.ro = scatter.pos;
        ray.rd = scatter.dir;

        attenuation = f4_mul(attenuation, f4_divvs(scatter.attenuation, scatter.pdf));
        prevFlags = surf.flags;
    }

    result.color = f4_f3(light);
    return result;
}

pim_inline float2 VEC_CALL SampleUv(
    pt_sampler_t* sampler,
    const dist1d_t* dist)
{
    float2 Xi = Sample2D(sampler);
    Xi.x = dist1d_samplec(dist, Xi.x);
    Xi.y = dist1d_samplec(dist, Xi.y);
    Xi = f2_mulvs(Xi, kPixelRadius);
    float2 b =
    {
        prng_bool(&sampler->rng) ? 1.0f : -1.0f,
        prng_bool(&sampler->rng) ? 1.0f : -1.0f,
    };
    Xi = f2_mul(Xi, b);
    return Xi;
}

pim_inline ray_t VEC_CALL CalculateDof(
    pt_sampler_t* sampler,
    const dofinfo_t* dof,
    float4 right,
    float4 up,
    float4 fwd,
    ray_t ray)
{
    float2 offset;
    if (dof->bladeCount == 666)
    {
        offset = SamplePentagram(&sampler->rng);
    }
    else
    {
        offset = SampleNGon(
            &sampler->rng,
            dof->bladeCount,
            dof->bladeRot);
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

        pt_result_t result = pt_trace_ray(&sampler, scene, ray);
        color[i] = f3_lerp(color[i], result.color, sampleWeight);
        albedo[i] = f3_lerp(albedo[i], result.albedo, sampleWeight);
        normal[i] = f3_lerp(normal[i], result.normal, sampleWeight);
    }
    SetSampler(sampler);
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

    pt_sampler_t sampler = GetSampler();
    for (i32 i = begin; i < end; ++i)
    {
        float2 Xi = Sample2D(&sampler);
        float4 rd = SampleUnitSphere(Xi);
        directions[i] = rd;
        pt_result_t result = pt_trace_ray(&sampler, scene, (ray_t) { ro, rd });
        colors[i] = f3_f4(result.color, 1.0f);
    }
    SetSampler(sampler);
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

pim_inline float VEC_CALL Sample1D(pt_sampler_t* sampler)
{
    return prng_f32(&sampler->rng);
}

pim_inline float2 VEC_CALL Sample2D(pt_sampler_t* sampler)
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
