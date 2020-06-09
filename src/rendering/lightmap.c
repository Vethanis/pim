#include "rendering/lightmap.h"
#include "allocator/allocator.h"
#include "rendering/drawable.h"
#include "math/float2_funcs.h"
#include "math/int2_funcs.h"
#include "math/float4_funcs.h"
#include "math/float4x4_funcs.h"
#include "math/sdf.h"
#include "math/sampling.h"
#include "common/console.h"
#include "common/sort.h"
#include "threading/task.h"
#include "threading/mutex.h"
#include "rendering/path_tracer.h"
#include "rendering/sampler.h"
#include "common/profiler.h"
#include "common/cmd.h"
#include "common/stringutil.h"
#include "common/atomics.h"
#include <stb/stb_image_write.h>

static lmpack_t ms_pack;
static bool ms_once;

static cmdstat_t CmdPrintLm(i32 argc, const char** argv);

lmpack_t* lmpack_get(void) { return &ms_pack; }

void lightmap_new(lightmap_t* lm, i32 size)
{
    ASSERT(lm);
    ASSERT(size > 0);
    i32 len = size * size;
    ASSERT(len > 0);

    lm->size = size;
    lm->texels = perm_malloc(sizeof(lm->texels[0]) * len);
    lm->sampleCounts = perm_malloc(sizeof(lm->sampleCounts[0]) * len);
    for (i32 i = 0; i < len; ++i)
    {
        lm->texels[i] = f4_s(0.1f);
        lm->sampleCounts[i] = 0.0f;
    }
}

void lightmap_del(lightmap_t* lm)
{
    if (lm)
    {
        lm->size = 0;
        pim_free(lm->texels);
        lm->texels = NULL;
        pim_free(lm->sampleCounts);
        lm->sampleCounts = NULL;
    }
}

typedef struct mask_s
{
    int2 size;
    u8* ptr;
} mask_t;

typedef struct atlas_s
{
    mutex_t mtx;
    mask_t mask;
    i32 nodecount;
} atlas_t;

static i32 chartnode_cmp(const void* lhs, const void* rhs, void* usr)
{
    const chartnode_t* a = lhs;
    const chartnode_t* b = rhs;
    if (a->area != b->area)
    {
        return a->area > b->area ? -1 : 1;
    }
    return a->drawableIndex - b->drawableIndex;
}

static void chartnode_sort(chartnode_t* nodes, i32 count)
{
    pimsort(nodes, count, sizeof(nodes[0]), chartnode_cmp, NULL);
}

static mask_t VEC_CALL mask_new(int2 size)
{
    i32 len = size.x * size.y;
    mask_t mask;
    mask.size = size;
    mask.ptr = pim_calloc(EAlloc_Task, sizeof(u8) * len);
    return mask;
}

static void VEC_CALL mask_del(mask_t* mask)
{
    pim_free(mask->ptr);
    mask->ptr = NULL;
    mask->size.x = 0;
    mask->size.y = 0;
}

static bool VEC_CALL mask_fits(mask_t a, mask_t b, int2 tr)
{
    for (i32 by = 0; by < b.size.y; ++by)
    {
        for (i32 bx = 0; bx < b.size.x; ++bx)
        {
            i32 bi = bx + by * b.size.x;
            if (b.ptr[bi])
            {
                i32 ax = bx + tr.x;
                i32 ay = by + tr.y;
                if (ax < 0 || ay < 0)
                {
                    return false;
                }
                if (ax >= a.size.x || ay >= a.size.y)
                {
                    return false;
                }
                i32 ai = ax + ay * a.size.x;
                if (a.ptr[ai])
                {
                    return false;
                }
            }
        }
    }
    return true;
}

static void VEC_CALL mask_write(mask_t a, mask_t b, int2 tr)
{
    for (i32 by = 0; by < b.size.y; ++by)
    {
        for (i32 bx = 0; bx < b.size.x; ++bx)
        {
            i32 bi = bx + by * b.size.x;
            if (b.ptr[bi])
            {
                i32 ax = bx + tr.x;
                i32 ay = by + tr.y;
                i32 ai = ax + ay * a.size.x;
                ASSERT(ax >= 0);
                ASSERT(ay >= 0);
                ASSERT(ax < a.size.x);
                ASSERT(ay < a.size.y);
                ASSERT(!a.ptr[ai]);
                a.ptr[ai] = b.ptr[bi];
            }
        }
    }
}

static int2 VEC_CALL tri_size(tri2d_t tri)
{
    float2 hi = f2_max(f2_max(tri.a, tri.b), tri.c);
    int2 size = f2_i2(f2_ceil(hi));
    return size;
}

static void VEC_CALL mask_tri(mask_t mask, tri2d_t tri)
{
    // assumes tri_size == mask.size
    const int2 size = mask.size;
    for (i32 y = 0; y < size.y; ++y)
    {
        for (i32 x = 0; x < size.x; ++x)
        {
            const i32 i = x + y * size.x;
            float2 pt = { x + 0.5f, y + 0.5f };
            float dist = sdTriangle2D(tri.a, tri.b, tri.c, pt);
            if (dist < 1.0f)
            {
                mask.ptr[i] = 0xff;
            }
        }
    }
}

static mask_t VEC_CALL mask_fromtri(tri2d_t tri)
{
    int2 size = tri_size(tri);
    size.x += 2;
    size.y += 2;
    mask_t mask = mask_new(size);
    mask_tri(mask, tri);
    return mask;
}

static bool VEC_CALL mask_find(mask_t atlas, mask_t item, int2* trOut, i32 prevRow)
{
    int2 lo = i2_addvs(i2_neg(item.size), 1);
    int2 hi = i2_subvs(atlas.size, 1);
    i32 y = lo.y;
    if (prevRow != 0)
    {
        y = prevRow;
    }
    for (; y < hi.y; ++y)
    {
        for (i32 x = lo.x; x < hi.x; ++x)
        {
            int2 tr = { x, y };
            if (mask_fits(atlas, item, tr))
            {
                *trOut = tr;
                return true;
            }
        }
    }
    return false;
}

static float2 VEC_CALL ProjUv(float3x3 TBN, float4 pt)
{
    float u = f4_dot3(TBN.c0, pt);
    float v = f4_dot3(TBN.c1, pt);
    return f2_v(u, v);
}

static float VEC_CALL TriArea3D(float4 A, float4 B, float4 C)
{
    return 0.5f * f4_length3(f4_cross3(f4_sub(B, A), f4_sub(C, A)));
}

static float VEC_CALL TriArea2D(tri2d_t tri)
{
    tri.a = f2_ceil(tri.a);
    tri.b = f2_ceil(tri.b);
    tri.c = f2_ceil(tri.c);
    float2 ab = f2_sub(tri.b, tri.a);
    float2 ac = f2_sub(tri.c, tri.a);
    float cr = ab.x * ac.y - ac.x * ab.y;
    return 0.5f * cr;
}

static tri2d_t VEC_CALL ProjTri(float4 A, float4 B, float4 C, float texelsPerUnit)
{
    plane_t plane = triToPlane(A, B, C);
    float3x3 TBN = NormalToTBN(plane.value);
    tri2d_t tri;
    tri.a = ProjUv(TBN, A);
    tri.b = ProjUv(TBN, B);
    tri.c = ProjUv(TBN, C);
    float2 lo = f2_min(f2_min(tri.a, tri.b), tri.c);
    tri.a = f2_sub(tri.a, lo);
    tri.b = f2_sub(tri.b, lo);
    tri.c = f2_sub(tri.c, lo);
    tri.a = f2_mulvs(tri.a, texelsPerUnit);
    tri.b = f2_mulvs(tri.b, texelsPerUnit);
    tri.c = f2_mulvs(tri.c, texelsPerUnit);
    if (TriArea2D(tri) < 1.0f)
    {
        tri.a = f2_0;
        tri.b = f2_0;
        tri.c = f2_0;
    }
    tri.a = f2_ceil(tri.a);
    tri.b = f2_ceil(tri.b);
    tri.c = f2_ceil(tri.c);
    tri.a = f2_addvs(tri.a, 2.0f);
    tri.b = f2_addvs(tri.b, 2.0f);
    tri.c = f2_addvs(tri.c, 2.0f);
    return tri;
}

static chartnode_t VEC_CALL chartnode_new(float4 A, float4 B, float4 C, float texelsPerUnit, i32 iDrawable, i32 iVert)
{
    chartnode_t node = { 0 };
    node.tri = ProjTri(A, B, C, texelsPerUnit);
    node.area = TriArea2D(node.tri);
    node.drawableIndex = iDrawable;
    node.vertIndex = iVert;
    node.atlasIndex = -1;
    return node;
}

static atlas_t atlas_new(i32 size)
{
    atlas_t atlas = { 0 };
    mutex_create(&atlas.mtx);
    atlas.mask = mask_new(i2_s(size));
    return atlas;
}

static void atlas_del(atlas_t* atlas)
{
    if (atlas)
    {
        mutex_destroy(&atlas->mtx);
        mask_del(&atlas->mask);
        memset(atlas, 0, sizeof(*atlas));
    }
}

static bool atlas_search(
    atlas_t* pim_noalias atlases,
    i32 atlasCount,
    chartnode_t* pim_noalias node,
    mask_t mask,
    i32* pim_noalias prevAtlas,
    i32* pim_noalias prevRow)
{
    int2 tr;
    for (i32 i = *prevAtlas; i < atlasCount; ++i)
    {
        atlas_t* pim_noalias atlas = atlases + i;
    retry:
        if (mask_find(atlas->mask, mask, &tr, *prevRow))
        {
            *prevAtlas = i;
            *prevRow = tr.y;
            mutex_lock(&atlas->mtx);
            bool fits = mask_fits(atlas->mask, mask, tr);
            if (fits)
            {
                mask_write(atlas->mask, mask, tr);
                node->atlasIndex = i;
                atlas->nodecount++;

                int2 size = atlas->mask.size;
                float2 trf = { (float)tr.x, (float)tr.y };
                float2 scf = { 1.0f / size.x, 1.0f / size.y };
                tri2d_t tri = node->tri;
                tri.a = f2_mul(f2_add(tri.a, trf), scf);
                tri.b = f2_mul(f2_add(tri.b, trf), scf);
                tri.c = f2_mul(f2_add(tri.c, trf), scf);
                node->tri = tri;
            }
            mutex_unlock(&atlas->mtx);
            if (fits)
            {
                return true;
            }
            else
            {
                goto retry;
            }
        }
        else
        {
            *prevRow = 0;
        }
    }
    return false;
}

static chartnode_t* chartnodes_create(float texelsPerUnit, i32* countOut)
{
    const drawables_t* drawables = drawables_get();
    const i32 numDrawables = drawables->count;
    const meshid_t* meshids = drawables->meshes;
    const float4x4* matrices = drawables->matrices;

    chartnode_t* nodes = NULL;
    i32 nodeCount = 0;

    for (i32 d = 0; d < numDrawables; ++d)
    {
        mesh_t mesh;
        if (!mesh_get(meshids[d], &mesh))
        {
            continue;
        }

        const float4x4 M = matrices[d];
        const i32 vertCount = mesh.length;
        const float4* positions = mesh.positions;

        i32 nodeBack = nodeCount;
        nodeCount += vertCount / 3;
        nodes = perm_realloc(nodes, sizeof(nodes[0]) * nodeCount);

        for (i32 v = 0; (v + 3) <= vertCount; v += 3)
        {
            float4 A = f4x4_mul_pt(M, positions[v + 0]);
            float4 B = f4x4_mul_pt(M, positions[v + 1]);
            float4 C = f4x4_mul_pt(M, positions[v + 2]);

            nodes[nodeBack++] = chartnode_new(A, B, C, texelsPerUnit, d, v);
        }
    }

    *countOut = nodeCount;
    return nodes;
}

typedef struct atlastask_s
{
    task_t task;
    i32 nodeHead;
    i32 nodeCount;
    i32 atlasCount;
    chartnode_t* nodes;
    atlas_t* atlases;
} atlastask_t;

static void AtlasFn(task_t* pbase, i32 begin, i32 end)
{
    atlastask_t* task = (atlastask_t*)pbase;
    chartnode_t* pim_noalias nodes = task->nodes;
    const i32 atlasCount = task->atlasCount;
    const i32 nodeCount = task->nodeCount;
    i32* pim_noalias pHead = &task->nodeHead;
    atlas_t* pim_noalias atlases = task->atlases;

    prng_t rng = prng_get();

    i32 prevAtlas = 0;
    i32 prevRow = 0;
    float prevArea = 1 << 20;
    for (i32 i = begin; i < end; ++i)
    {
        i32 iNode = inc_i32(pHead, MO_Relaxed);
        if (iNode >= nodeCount)
        {
            break;
        }
        chartnode_t node = nodes[iNode];
        mask_t nodemask = mask_fromtri(node.tri);

        if (node.area < (prevArea * 0.9f))
        {
            prevArea = node.area;
            prevAtlas = 0;
            prevRow = 0;
        }

        while (!atlas_search(atlases, atlasCount, &node, nodemask, &prevAtlas, &prevRow))
        {
            prevArea = node.area;
            prevAtlas = 0;
            prevRow = 0;
        }

        mask_del(&nodemask);
        nodes[iNode] = node;
    }

    prng_set(rng);
}

static i32 atlas_estimate(i32 atlasSize, const chartnode_t* nodes, i32 nodeCount)
{
    const float fitMult = 1.0f / 0.5f;
    i32 areaRequired = 0;
    for (i32 i = 0; i < nodeCount; ++i)
    {
        i32 area = (i32)ceilf(TriArea2D(nodes[i].tri) * fitMult);
        areaRequired += area;
    }
    ASSERT(areaRequired >= 0);

    const i32 areaPerAtlas = atlasSize * atlasSize;
    i32 atlasCount = 0;
    while ((atlasCount * areaPerAtlas) < areaRequired)
    {
        ++atlasCount;
    }

    return i1_max(1, atlasCount);
}

static i32 atlases_create(i32 atlasSize, chartnode_t* nodes, i32 nodeCount)
{
    i32 atlasCount = atlas_estimate(atlasSize, nodes, nodeCount);
    atlas_t* atlases = perm_calloc(sizeof(atlases[0]) * atlasCount);
    for (i32 i = 0; i < atlasCount; ++i)
    {
        atlases[i] = atlas_new(atlasSize);
    }

    atlastask_t* task = tmp_calloc(sizeof(*task));
    task->atlasCount = atlasCount;
    task->atlases = atlases;
    task->nodes = nodes;
    task->nodeCount = nodeCount;
    task->nodeHead = 0;
    task_submit(&task->task, AtlasFn, nodeCount);
    task_sys_schedule();
    task_await(&task->task);

    i32 usedAtlases = 0;
    for (i32 i = 0; i < atlasCount; ++i)
    {
        if (atlases[i].nodecount > 0)
        {
            ++usedAtlases;
        }
        atlas_del(atlases + i);
    }
    pim_free(atlases);

    return usedAtlases;
}

static void chartnodes_assign(chartnode_t* nodes, i32 nodeCount)
{
    const drawables_t* drawables = drawables_get();
    const i32 numDrawables = drawables->count;
    const meshid_t* meshids = drawables->meshes;

    for (i32 i = 0; i < nodeCount; ++i)
    {
        chartnode_t node = nodes[i];
        ASSERT(node.drawableIndex >= 0);
        ASSERT(node.drawableIndex < numDrawables);
        ASSERT(node.vertIndex >= 0);
        meshid_t meshid = meshids[node.drawableIndex];
        mesh_t mesh;
        if (mesh_get(meshid, &mesh))
        {
            const i32 vertCount = mesh.length;
            ASSERT((node.vertIndex + 2) < vertCount);
            float3* lmUvs = mesh.lmUvs;
            float3 a = { node.tri.a.x, node.tri.a.y, (float)node.atlasIndex };
            float3 b = { node.tri.b.x, node.tri.b.y, (float)node.atlasIndex };
            float3 c = { node.tri.c.x, node.tri.c.y, (float)node.atlasIndex };
            lmUvs[node.vertIndex + 0] = a;
            lmUvs[node.vertIndex + 1] = b;
            lmUvs[node.vertIndex + 2] = c;
        }
        else
        {
            ASSERT(false);
        }
    }
}

lmpack_t lmpack_pack(i32 atlasSize, float texelsPerUnit)
{
    ASSERT(atlasSize > 0);

    i32 nodeCount = 0;
    chartnode_t* nodes = chartnodes_create(texelsPerUnit, &nodeCount);

    chartnode_sort(nodes, nodeCount);

    i32 atlasCount = atlases_create(atlasSize, nodes, nodeCount);

    chartnodes_assign(nodes, nodeCount);

    lmpack_t pack = { 0 };
    pack.lmCount = atlasCount;
    pack.lightmaps = perm_calloc(sizeof(pack.lightmaps[0]) * atlasCount);
    pack.nodeCount = nodeCount;
    pack.nodes = nodes;

    for (i32 i = 0; i < atlasCount; ++i)
    {
        lightmap_new(pack.lightmaps + i, atlasSize);
    }

    return pack;
}

void lmpack_del(lmpack_t* pack)
{
    if (pack)
    {
        for (i32 i = 0; i < pack->lmCount; ++i)
        {
            lightmap_del(pack->lightmaps + i);
        }
        pim_free(pack->lightmaps);
        pim_free(pack->nodes);
        memset(pack, 0, sizeof(*pack));
    }
}

static float4 VEC_CALL rand_wuv(prng_t* rng)
{
    float a;
    float b;
    float c;
    do
    {
        a = prng_f32(rng);
        b = prng_f32(rng);
        c = 1.0f - (a + b);
    } while (c < 0.0f);

    float4 wuv;
    switch (prng_i32(rng) % 3)
    {
    case 0:
        wuv.x = a;
        wuv.y = b;
        wuv.z = c;
        break;
    case 1:
        wuv.x = b;
        wuv.y = c;
        wuv.z = a; // ll
        break;
    case 2:
        wuv.x = c; // ops
        wuv.y = a; // re
        wuv.z = b; // astards
        break;
    }

    return wuv;
}

static float4 VEC_CALL norm_blend(float4 A, float4 B, float4 C, float4 wuv)
{
    float4 N = f4_blend(A, B, C, wuv);
    return f4_normalize3(N);
}

static float2 VEC_CALL lm_blend(float3 a, float3 b, float3 c, float4 wuv)
{
    float3 lm3 = f3_blend(a, b, c, wuv);
    float2 lm = { lm3.x, lm3.y };
    return lm;
}

typedef struct bake_s
{
    task_t task;
    const pt_scene_t* scene;
    i32 bounces;
} bake_t;

static void BakeFn(task_t* pbase, i32 begin, i32 end)
{
    bake_t* task = (bake_t*)pbase;
    const pt_scene_t* scene = task->scene;
    const i32 bounces = task->bounces;

    const drawables_t* drawables = drawables_get();
    const meshid_t* pim_noalias meshids = drawables->meshes;
    const float4x4* pim_noalias matrices = drawables->matrices;

    lmpack_t* pack = lmpack_get();
    lightmap_t* pim_noalias lightmaps = pack->lightmaps;
    const i32 lmCount = pack->lmCount;

    prng_t rng = prng_get();
    for (i32 iMesh = begin; iMesh < end; ++iMesh)
    {
        meshid_t meshid = meshids[iMesh];
        mesh_t mesh;
        if (!mesh_get(meshid, &mesh))
        {
            continue;
        }

        const float4x4 M = matrices[iMesh];
        const float4x4 IM = f4x4_inverse(f4x4_transpose(M));
        const float4* pim_noalias positions = mesh.positions;
        const float4* pim_noalias normals = mesh.normals;
        const float3* pim_noalias lmUvs = mesh.lmUvs;
        const i32 vertCount = mesh.length;

        for (i32 iVert = 0; (iVert + 3) <= vertCount; iVert += 3)
        {
            const i32 a = iVert + 0;
            const i32 b = iVert + 1;
            const i32 c = iVert + 2;

            float3 LMA = lmUvs[a];
            float3 LMB = lmUvs[b];
            float3 LMC = lmUvs[c];

            const i32 iLightmap = (i32)LMA.z;
            ASSERT(iLightmap < lmCount);
            if (iLightmap < 0)
            {
                continue;
            }

            lightmap_t lightmap = lightmaps[iLightmap];
            ASSERT(lightmap.size > 0);
            ASSERT(lightmap.texels);
            ASSERT(lightmap.sampleCounts);

            float4* pim_noalias texels = lightmap.texels;
            float* pim_noalias sampleCounts = lightmap.sampleCounts;
            const int2 lmSize = { lightmap.size, lightmap.size };

            float4 A = f4x4_mul_pt(M, positions[a]);
            float4 B = f4x4_mul_pt(M, positions[b]);
            float4 C = f4x4_mul_pt(M, positions[c]);

            float4 NA = f4_normalize3(f4x4_mul_dir(IM, normals[a]));
            float4 NB = f4_normalize3(f4x4_mul_dir(IM, normals[b]));
            float4 NC = f4_normalize3(f4x4_mul_dir(IM, normals[c]));

            float area = TriArea3D(A, B, C);
            area *= 4.0f;
            i32 samplesToTake = (i32)area;
            if (prng_f32(&rng) < area)
            {
                ++samplesToTake;
            }

            for (i32 iSample = 0; iSample < samplesToTake; ++iSample)
            {
                float4 wuv = rand_wuv(&rng);
                float4 P = f4_blend(A, B, C, wuv);
                float4 N = norm_blend(NA, NB, NC, wuv);
                float2 LM = lm_blend(LMA, LMB, LMC, wuv);

                float4 rd;
                float pdf = ScatterLambertian(&rng, N, N, &rd, 1.0f);
                ray_t ray = { P, rd };
                float4 light = pt_trace_ray(&rng, scene, ray, bounces);
                light = f4_mulvs(light, pdf);

                bilinear_t bi = Bilinear(lmSize, LM);

                i32 a = Clamp(lmSize, bi.a);
                i32 b = Clamp(lmSize, bi.b);
                i32 c = Clamp(lmSize, bi.c);
                i32 d = Clamp(lmSize, bi.d);

                float aw = (1.0f - bi.frac.x) * (1.0f - bi.frac.y);
                float bw = bi.frac.x * (1.0f - bi.frac.y);
                float cw = (1.0f - bi.frac.x) * bi.frac.y;
                float dw = bi.frac.x * bi.frac.y;

                float as = 1.0f / (1.0f + sampleCounts[a]);
                float bs = 1.0f / (1.0f + sampleCounts[b]);
                float cs = 1.0f / (1.0f + sampleCounts[c]);
                float ds = 1.0f / (1.0f + sampleCounts[d]);

                sampleCounts[a] += aw;
                sampleCounts[b] += bw;
                sampleCounts[c] += cw;
                sampleCounts[d] += dw;

                aw *= as;
                bw *= bs;
                cw *= cs;
                dw *= ds;

                texels[a] = f4_lerpvs(texels[a], light, aw);
                texels[b] = f4_lerpvs(texels[b], light, bw);
                texels[c] = f4_lerpvs(texels[c], light, cw);
                texels[d] = f4_lerpvs(texels[d], light, dw);
            }
        }
    }
    prng_set(rng);
}

ProfileMark(pm_Bake, lmpack_bake)
task_t* lmpack_bake(const pt_scene_t* scene, i32 bounces)
{
    ProfileBegin(pm_Bake);
    ASSERT(scene);

    if (!ms_once)
    {
        ms_once = true;
        cmd_reg("lm_print", CmdPrintLm);
    }

    bake_t* task = perm_calloc(sizeof(*task));
    task->scene = scene;
    task->bounces = bounces;
    task_submit(&task->task, BakeFn, drawables_get()->count);

    ProfileEnd(pm_Bake);
    return &task->task;
}

static cmdstat_t CmdPrintLm(i32 argc, const char** argv)
{
    char filename[PIM_PATH] = {0};
    const char* prefix = "lightmap";
    if (argc > 1)
    {
        const char* arg1 = argv[1];
        if (arg1)
        {
            prefix = arg1;
        }
    }

    u32* buffer = NULL;
    const lmpack_t* pack = lmpack_get();
    for (i32 i = 0; i < pack->lmCount; ++i)
    {
        const lightmap_t lm = pack->lightmaps[i];
        const i32 len = lm.size * lm.size;
        buffer = tmp_realloc(buffer, sizeof(buffer[0]) * len);
        for (i32 j = 0; j < len; ++j)
        {
            float4 ldr = tmap4_reinhard(lm.texels[j]);
            u32 color = LinearToColor(ldr);
            color |= 0xff << 24;
            buffer[j] = color;
        }
        SPrintf(ARGS(filename), "%s_%d.png", prefix, i);
        if (!stbi_write_png(filename, lm.size, lm.size, 4, buffer, lm.size * sizeof(buffer[0])))
        {
            con_logf(LogSev_Error, "LM", "Failed to print lightmap image '%s'", filename);
            return cmdstat_err;
        }
        con_logf(LogSev_Info, "LM", "Printed lightmap image '%s'", filename);
    }

    return cmdstat_ok;
}
