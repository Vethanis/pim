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
#include "rendering/denoise.h"
#include "common/profiler.h"
#include "common/cmd.h"
#include "common/stringutil.h"
#include "common/atomics.h"
#include <stb/stb_image_write.h>
#include <string.h>

#define CHART_SPLITS    2
#define ROW_RESET       -(1<<20)

typedef enum
{
    LmChannel_Color,
    LmChannel_Position,
    LmChannel_Normal,
    LmChannel_Denoised,

    LmChannel_COUNT
} LmChannel;

typedef struct mask_s
{
    int2 size;
    u8* ptr;
} mask_t;

typedef struct chartnode_s
{
    plane_t plane;
    tri2d_t triCoord;
    tri2d_t triUv;
    float area;
    i32 drawableIndex;
    i32 vertIndex;
} chartnode_t;

typedef struct chart_s
{
    mask_t mask;
    chartnode_t* nodes;
    i32 nodeCount;
    i32 atlasIndex;
    int2 translation;
    float area;
} chart_t;

typedef struct atlas_s
{
    mutex_t mtx;
    mask_t mask;
    i32 chartCount;
} atlas_t;

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
    lm->color = perm_calloc(sizeof(lm->color[0]) * len);
    lm->denoised = perm_calloc(sizeof(lm->denoised[0]) * len);
    lm->sampleCounts = perm_calloc(sizeof(lm->sampleCounts[0]) * len);
    lm->position = perm_calloc(sizeof(lm->position[0]) * len);
    lm->normal = perm_calloc(sizeof(lm->normal[0]) * len);
}

void lightmap_del(lightmap_t* lm)
{
    if (lm)
    {
        pim_free(lm->color);
        pim_free(lm->denoised);
        pim_free(lm->sampleCounts);
        pim_free(lm->position);
        pim_free(lm->normal);
        memset(lm, 0, sizeof(*lm));
    }
}

static i32 TexelCount(const lightmap_t* lightmaps, i32 lmCount)
{
    i32 texelCount = 0;
    for (i32 i = 0; i < lmCount; ++i)
    {
        i32 lmSize = lightmaps[i].size;
        texelCount += lmSize * lmSize;
    }
    return texelCount;
}

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
    mask.ptr = perm_calloc(sizeof(u8) * len);
    return mask;
}

static void VEC_CALL mask_del(mask_t* mask)
{
    pim_free(mask->ptr);
    mask->ptr = NULL;
    mask->size.x = 0;
    mask->size.y = 0;
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

static bool VEC_CALL TriTest(tri2d_t tri, float2 pt)
{
    const i32 count = 8;
    for (i32 s = 0; s < count; ++s)
    {
        float2 Xi = Hammersley2D(s, count);
        float2 pt2 = { pt.x + Xi.x, pt.y + Xi.y };
        float dist = sdTriangle2D(tri.a, tri.b, tri.c, pt2);
        if (dist <= 1.5f)
        {
            return true;
        }
        if (dist >= 4.0f)
        {
            return false;
        }
    }
    return false;
}

static void VEC_CALL mask_tri(mask_t mask, tri2d_t tri)
{
    const int2 size = mask.size;
    for (i32 y = 0; y < size.y; ++y)
    {
        for (i32 x = 0; x < size.x; ++x)
        {
            const i32 i = x + y * size.x;
            if (TriTest(tri, f2_iv(x, y)))
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
    if (prevRow != ROW_RESET)
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
    float2 ab = f2_sub(tri.b, tri.a);
    float2 ac = f2_sub(tri.c, tri.a);
    float cr = ab.x * ac.y - ac.x * ab.y;
    return 0.5f * cr;
}

static tri2d_t VEC_CALL ProjTri(float4 A, float4 B, float4 C, float texelsPerUnit, plane_t* planeOut)
{
    plane_t plane = triToPlane(A, B, C);
    *planeOut = plane;
    float3x3 TBN = NormalToTBN(plane.value);
    tri2d_t tri;
    tri.a = ProjUv(TBN, A);
    tri.b = ProjUv(TBN, B);
    tri.c = ProjUv(TBN, C);
    tri.a = f2_mulvs(tri.a, texelsPerUnit);
    tri.b = f2_mulvs(tri.b, texelsPerUnit);
    tri.c = f2_mulvs(tri.c, texelsPerUnit);
    return tri;
}

static chartnode_t VEC_CALL chartnode_new(float4 A, float4 B, float4 C, float texelsPerUnit, i32 iDrawable, i32 iVert)
{
    chartnode_t node = { 0 };
    node.triCoord = ProjTri(A, B, C, texelsPerUnit, &node.plane);
    node.area = TriArea2D(node.triCoord);
    node.drawableIndex = iDrawable;
    node.vertIndex = iVert;
    return node;
}

static void chart_del(chart_t* chart)
{
    if (chart)
    {
        mask_del(&chart->mask);
        pim_free(chart->nodes);
        chart->nodes = NULL;
        chart->nodeCount = 0;
    }
}

static bool VEC_CALL plane_equal(
    plane_t lhs, plane_t rhs, float distThresh, float minCosTheta)
{
    float dist = f1_distance(lhs.value.w, rhs.value.w);
    float cosTheta = f4_dot3(lhs.value, rhs.value);
    return (dist < distThresh) && (cosTheta >= minCosTheta);
}

static i32 plane_find(
    const plane_t* planes,
    i32 planeCount,
    plane_t plane,
    float distThresh,
    float degreeThresh)
{
    const float minCosTheta = cosf(degreeThresh * kRadiansPerDegree);
    for (i32 i = 0; i < planeCount; ++i)
    {
        if (plane_equal(planes[i], plane, distThresh, minCosTheta))
        {
            return i;
        }
    }
    return -1;
}

static void chart_minmax(chart_t chart, float2* loOut, float2* hiOut)
{
    const float bigNum = 1 << 20;
    float2 lo = f2_s(bigNum);
    float2 hi = f2_s(-bigNum);
    for (i32 i = 0; i < chart.nodeCount; ++i)
    {
        tri2d_t tri = chart.nodes[i].triCoord;
        lo = f2_min(lo, tri.a);
        hi = f2_max(hi, tri.a);
        lo = f2_min(lo, tri.b);
        hi = f2_max(hi, tri.b);
        lo = f2_min(lo, tri.c);
        hi = f2_max(hi, tri.c);
    }
    *loOut = lo;
    *hiOut = hi;
}

static float chart_area(chart_t chart)
{
    float2 lo, hi;
    chart_minmax(chart, &lo, &hi);
    float2 size = f2_sub(hi, lo);
    return size.x * size.y;
}

static float chart_width(chart_t chart)
{
    float2 lo, hi;
    chart_minmax(chart, &lo, &hi);
    float2 size = f2_sub(hi, lo);
    return f2_hmax(size);
}

static float chart_triarea(chart_t chart)
{
    float sum = 0.0f;
    for (i32 i = 0; i < chart.nodeCount; ++i)
    {
        tri2d_t tri = chart.nodes[i].triCoord;
        float area = TriArea2D(tri);
        sum += area;
    }
    return sum;
}

static float chart_density(chart_t chart)
{
    float fromTri = chart_triarea(chart);
    float fromBounds = chart_area(chart);
    return fromTri / f1_max(fromBounds, f16_eps);
}

static float2 VEC_CALL tri_center(tri2d_t tri)
{
    const float scale = 1.0f / 3;
    float2 center = f2_mulvs(tri.a, scale);
    center = f2_add(center, f2_mulvs(tri.b, scale));
    center = f2_add(center, f2_mulvs(tri.c, scale));
    return center;
}

static float2 cluster_mean(const tri2d_t* tris, i32 count)
{
    const float scale = 1.0f / count;
    float2 mean = f2_0;
    for (i32 i = 0; i < count; ++i)
    {
        float2 center = tri_center(tris[i]);
        mean = f2_add(mean, f2_mulvs(center, scale));
    }
    return mean;
}

static i32 cluster_nearest(const float2* means, i32 k, tri2d_t tri)
{
    i32 chosen = -1;
    float chosenDist = 1 << 20;
    for (i32 i = 0; i < k; ++i)
    {
        float dist = sdTriangle2D(tri.a, tri.b, tri.c, means[i]);
        if (dist < chosenDist)
        {
            chosenDist = dist;
            chosen = i;
        }
    }
    return chosen;
}

static float cluster_cost(const tri2d_t* tris, i32 count)
{
    float2 mean = cluster_mean(tris, count);
    float cost = 0.0f;
    for (i32 i = 0; i < count; ++i)
    {
        tri2d_t tri = tris[i];
        float dist = sdTriangle2D(tri.a, tri.b, tri.c, mean);
        cost += dist * dist;
    }
    return cost;
}

static void chart_split(chart_t chart, chart_t* split)
{
    const i32 nodeCount = chart.nodeCount;
    const chartnode_t* nodes = chart.nodes;

    float2 means[CHART_SPLITS] = { 0 };
    float2 prevMeans[CHART_SPLITS] = { 0 };
    i32 counts[CHART_SPLITS] = { 0 };
    tri2d_t* triLists[CHART_SPLITS] = { 0 };
    i32* nodeLists[CHART_SPLITS] = { 0 };
    const i32 k = CHART_SPLITS;

    // create k initial means
    prng_t rng = prng_get();
    for (i32 i = 0; i < k; ++i)
    {
        i32 j = prng_i32(&rng) % nodeCount;
        tri2d_t tri = nodes[j].triCoord;
        means[i] = tri_center(tri);
        triLists[i] = tmp_malloc(sizeof(tri2d_t) * nodeCount);
        nodeLists[i] = tmp_malloc(sizeof(i32) * nodeCount);
    }
    prng_set(rng);

    do
    {
        // reset cluster lists
        for (i32 i = 0; i < k; ++i)
        {
            counts[i] = 0;
        }

        // associate each node with nearest mean
        for (i32 i = 0; i < nodeCount; ++i)
        {
            tri2d_t tri = nodes[i].triCoord;
            i32 cl = cluster_nearest(means, k, tri);

            i32 back = counts[cl]++;
            triLists[cl][back] = tri;
            nodeLists[cl][back] = i;
        }

        // update means
        for (i32 i = 0; i < k; ++i)
        {
            prevMeans[i] = means[i];
            means[i] = cluster_mean(triLists[i], counts[i]);
        }

    } while (memcmp(means, prevMeans, sizeof(means)));

    for (i32 i = 0; i < k; ++i)
    {
        chart_t ch = { 0 };
        ch.nodeCount = counts[i];
        if (ch.nodeCount > 0)
        {
            PermReserve(ch.nodes, ch.nodeCount);
            const i32* nodeList = nodeLists[i];
            for (i32 j = 0; j < ch.nodeCount; ++j)
            {
                i32 iNode = nodeList[j];
                ch.nodes[j] = nodes[iNode];
            }
        }
        split[i] = ch;
    }
}

typedef struct chartmask_s
{
    task_t task;
    chart_t* charts;
    i32 chartCount;
} chartmask_t;

static void ChartMaskFn(task_t* pbase, i32 begin, i32 end)
{
    chartmask_t* task = (chartmask_t*)pbase;
    chart_t* charts = task->charts;
    i32 chartCount = task->chartCount;

    for (i32 i = begin; i < end; ++i)
    {
        chart_t chart = charts[i];

        float2 lo, hi;
        chart_minmax(chart, &lo, &hi);
        lo = f2_subvs(lo, 8.0f);
        for (i32 iNode = 0; iNode < chart.nodeCount; ++iNode)
        {
            tri2d_t tri = chart.nodes[iNode].triCoord;
            tri.a = f2_sub(tri.a, lo);
            tri.b = f2_sub(tri.b, lo);
            tri.c = f2_sub(tri.c, lo);
            chart.nodes[iNode].triCoord = tri;
        }
        chart_minmax(chart, &lo, &hi);
        float2 size = f2_sub(hi, lo);
        chart.area = size.x * size.y;
        hi = f2_addvs(hi, 8.0f);

        chart.mask = mask_new(f2_i2(hi));
        for (i32 iNode = 0; iNode < chart.nodeCount; ++iNode)
        {
            tri2d_t tri = chart.nodes[iNode].triCoord;
            mask_tri(chart.mask, tri);
        }

        charts[i] = chart;
    }
}

static chart_t* chart_group(
    const chartnode_t* nodes,
    i32 nodeCount,
    i32* countOut,
    float distThresh,
    float degreeThresh,
    float maxWidth)
{
    ASSERT(nodes);
    ASSERT(nodeCount >= 0);
    ASSERT(countOut);

    i32 chartCount = 0;
    chart_t* charts = NULL;
    plane_t* planes = NULL;

    // assign nodes to charts by triangle plane
    for (i32 iNode = 0; iNode < nodeCount; ++iNode)
    {
        chartnode_t node = nodes[iNode];
        i32 iChart = plane_find(
            planes, chartCount, node.plane,
            distThresh, degreeThresh);
        if (iChart == -1)
        {
            iChart = chartCount;
            ++chartCount;
            PermGrow(charts, chartCount);
            PermGrow(planes, chartCount);
            planes[iChart] = node.plane;
        }
        chart_t chart = charts[iChart];
        chart.nodeCount += 1;
        PermReserve(chart.nodes, chart.nodeCount);
        chart.nodes[chart.nodeCount - 1] = node;
        charts[iChart] = chart;
    }

    pim_free(planes);
    planes = NULL;

    // split big charts
    for (i32 iChart = 0; iChart < chartCount; ++iChart)
    {
        chart_t chart = charts[iChart];
        if (chart.nodeCount > 1)
        {
            float width = chart_width(chart);
            float density = chart_density(chart);
            if ((width >= maxWidth) || (density < 0.1f))
            {
                chart_t split[CHART_SPLITS] = { 0 };
                chart_split(chart, split);
                for (i32 j = 0; j < NELEM(split); ++j)
                {
                    if (split[j].nodeCount > 0)
                    {
                        ++chartCount;
                        PermReserve(charts, chartCount);
                        charts[chartCount - 1] = split[j];
                    }
                }
                chart_del(&chart);

                charts[iChart] = charts[chartCount - 1];
                --chartCount;
                --iChart;
            }
        }
    }

    // move chart to origin and create mask
    chartmask_t* task = tmp_calloc(sizeof(*task));
    task->charts = charts;
    task->chartCount = chartCount;
    task_run(&task->task, ChartMaskFn, chartCount);

    *countOut = chartCount;
    return charts;
}

static void chart_apply(chart_t chart, int2 atlasSize)
{
    int2 tr = chart.translation;
    int2 size = atlasSize;

    float2 trf = { (float)tr.x, (float)tr.y };
    float2 scf = { 1.0f / size.x, 1.0f / size.y };

    for (i32 i = 0; i < chart.nodeCount; ++i)
    {
        chartnode_t* node = chart.nodes + i;

        tri2d_t triCoord = node->triCoord;
        triCoord.a = f2_add(triCoord.a, trf);
        triCoord.b = f2_add(triCoord.b, trf);
        triCoord.c = f2_add(triCoord.c, trf);
        tri2d_t triUv;
        triUv.a = f2_mul(triCoord.a, scf);
        triUv.b = f2_mul(triCoord.b, scf);
        triUv.c = f2_mul(triCoord.c, scf);

        node->triCoord = triCoord;
        node->triUv = triUv;
    }
}

static i32 chart_cmp(const void* plhs, const void* prhs, void* usr)
{
    const chart_t* lhs = plhs;
    const chart_t* rhs = prhs;
    float a = lhs->area;
    float b = rhs->area;
    if (a != b)
    {
        return a > b ? -1 : 1;
    }
    return 0;
}

static void chart_sort(chart_t* charts, i32 chartCount)
{
    pimsort(charts, chartCount, sizeof(charts[0]), chart_cmp, NULL);
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
    chart_t* chart,
    i32* pim_noalias prevAtlas,
    i32* pim_noalias prevRow)
{
    int2 tr;
    for (i32 i = *prevAtlas; i < atlasCount; ++i)
    {
        atlas_t* pim_noalias atlas = atlases + i;
    retry:
        if (mask_find(atlas->mask, chart->mask, &tr, *prevRow))
        {
            *prevAtlas = i;
            *prevRow = tr.y;
            mutex_lock(&atlas->mtx);
            bool fits = mask_fits(atlas->mask, chart->mask, tr);
            if (fits)
            {
                mask_write(atlas->mask, chart->mask, tr);
                chart->translation = tr;
                chart->atlasIndex = i;
                atlas->chartCount++;
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
            *prevRow = ROW_RESET;
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
    i32 chartCount;
    i32 atlasCount;
    chart_t* charts;
    atlas_t* atlases;
} atlastask_t;

static void AtlasFn(task_t* pbase, i32 begin, i32 end)
{
    atlastask_t* task = (atlastask_t*)pbase;
    chart_t* pim_noalias charts = task->charts;
    const i32 atlasCount = task->atlasCount;
    const i32 chartCount = task->chartCount;
    i32* pim_noalias pHead = &task->nodeHead;
    atlas_t* pim_noalias atlases = task->atlases;

    prng_t rng = prng_get();

    i32 prevAtlas = 0;
    i32 prevRow = ROW_RESET;
    float prevArea = 1 << 20;
    for (i32 i = begin; i < end; ++i)
    {
        i32 iChart = inc_i32(pHead, MO_Relaxed);
        if (iChart >= chartCount)
        {
            break;
        }
        chart_t chart = charts[iChart];
        chart.atlasIndex = -1;

        if (chart.area <= (prevArea * 0.95f))
        {
            prevArea = chart.area;
            prevAtlas = 0;
            prevRow = ROW_RESET;
        }

        while (!atlas_search(
            atlases,
            atlasCount,
            &chart,
            &prevAtlas,
            &prevRow))
        {
            prevArea = chart.area;
            prevAtlas = 0;
            prevRow = ROW_RESET;
        }

        if (chart.atlasIndex != -1)
        {
            chart_apply(chart, atlases[chart.atlasIndex].mask.size);
        }

        mask_del(&chart.mask);
        charts[iChart] = chart;
    }

    prng_set(rng);
}

static i32 atlas_estimate(i32 atlasSize, const chart_t* charts, i32 chartCount)
{
    i32 areaRequired = 0;
    for (i32 i = 0; i < chartCount; ++i)
    {
        i32 area = (i32)ceilf(chart_area(charts[i]));
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

static i32 atlases_create(i32 atlasSize, chart_t* charts, i32 chartCount)
{
    i32 atlasCount = atlas_estimate(atlasSize, charts, chartCount);
    atlas_t* atlases = perm_calloc(sizeof(atlases[0]) * atlasCount);
    for (i32 i = 0; i < atlasCount; ++i)
    {
        atlases[i] = atlas_new(atlasSize);
    }

    atlastask_t* task = tmp_calloc(sizeof(*task));
    task->atlasCount = atlasCount;
    task->atlases = atlases;
    task->charts = charts;
    task->chartCount = chartCount;
    task->nodeHead = 0;
    task_submit(&task->task, AtlasFn, chartCount);
    task_sys_schedule();
    task_await(&task->task);

    i32 usedAtlases = 0;
    for (i32 i = 0; i < atlasCount; ++i)
    {
        if (atlases[i].chartCount > 0)
        {
            ++usedAtlases;
        }
        atlas_del(atlases + i);
    }
    pim_free(atlases);

    return usedAtlases;
}

static void chartnodes_assign(
    chart_t* charts,
    i32 chartCount,
    lightmap_t* lightmaps,
    i32 lightmapCount)
{
    const drawables_t* drawables = drawables_get();
    const i32 numDrawables = drawables->count;
    const meshid_t* meshids = drawables->meshes;

    for (i32 iChart = 0; iChart < chartCount; ++iChart)
    {
        chart_t chart = charts[iChart];
        chartnode_t* nodes = chart.nodes;
        i32 nodeCount = chart.nodeCount;
        float atlasIndex = (float)chart.atlasIndex;

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
                float3 a = { node.triUv.a.x, node.triUv.a.y, atlasIndex };
                float3 b = { node.triUv.b.x, node.triUv.b.y, atlasIndex };
                float3 c = { node.triUv.c.x, node.triUv.c.y, atlasIndex };
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
}

typedef struct quadtree_s
{
    i32 maxDepth;
    i32 nodeCount;
    box2d_t* boxes;         // bounding box of node
    i32* listLens;          // number of triangles in this node
    tri2d_t** triLists;     // lightmap UV
    int2** indexLists;      // iDrawable, iVert
} quadtree_t;

static i32 CalcNodeCount(i32 maxDepth)
{
    i32 nodeCount = 0;
    for (i32 i = 0; i < maxDepth; ++i)
    {
        nodeCount += 1 << (2 * i);
    }
    return nodeCount;
}

static i32 GetChild(i32 parent, i32 i)
{
    return (parent << 2) | (i + 1);
}

static i32 GetParent(i32 c)
{
    return (c - 1) >> 2;
}

static void SetupBounds(box2d_t* boxes, i32 p, i32 nodeCount)
{
    const i32 c0 = GetChild(p, 0);
    if ((c0 + 4) <= nodeCount)
    {
        {
            const float2 pcenter = boxes[p].center;
            const float2 extents = f2_mulvs(boxes[p].extents, 0.5f);
            for (i32 i = 0; i < 4; ++i)
            {
                float2 sign;
                sign.x = (i & 1) ? -1.0f : 1.0f;
                sign.y = (i & 2) ? -1.0f : 1.0f;
                boxes[c0 + i].center = f2_add(pcenter, f2_mul(sign, extents));
                boxes[c0 + i].extents = extents;
            }
        }
        for (i32 i = 0; i < 4; ++i)
        {
            SetupBounds(boxes, c0 + i, nodeCount);
        }
    }
}

static void quadtree_new(quadtree_t* qt, i32 maxDepth, box2d_t bounds)
{
    i32 len = CalcNodeCount(maxDepth);
    qt->maxDepth = maxDepth;
    qt->nodeCount = len;
    qt->boxes = perm_calloc(sizeof(qt->boxes[0]) * len);
    qt->listLens = perm_calloc(sizeof(qt->listLens[0]) * len);
    qt->triLists = perm_calloc(sizeof(qt->triLists[0]) * len);
    qt->indexLists = perm_calloc(sizeof(qt->indexLists[0]) * len);
    if (len > 0)
    {
        qt->boxes[0] = bounds;
        SetupBounds(qt->boxes, 0, len);
    }
}

static void quadtree_del(quadtree_t* qt)
{
    if (qt)
    {
        pim_free(qt->boxes);
        pim_free(qt->listLens);
        i32 len = qt->nodeCount;
        for (i32 i = 0; i < len; ++i)
        {
            pim_free(qt->triLists[i]);
            pim_free(qt->indexLists[i]);
        }
        pim_free(qt->triLists);
        pim_free(qt->indexLists);
        memset(qt, 0, sizeof(*qt));
    }
}

static bool VEC_CALL BoxHoldsTri(box2d_t box, tri2d_t tri)
{
    float2 lo = f2_sub(box.center, box.extents);
    float2 hi = f2_add(box.center, box.extents);
    bool2 a = b2_and(b2_and(f2_gteq(tri.a, lo), f2_gteq(tri.b, lo)), f2_gteq(tri.c, lo));
    bool2 b = b2_and(b2_and(f2_lteq(tri.a, hi), f2_lteq(tri.b, hi)), f2_lteq(tri.c, hi));
    return b2_all(b2_and(a, b));
}

static bool VEC_CALL BoxHoldsPt(box2d_t box, float2 pt)
{
    float2 lo = f2_sub(box.center, box.extents);
    float2 hi = f2_add(box.center, box.extents);
    return b2_all(b2_and(f2_gteq(pt, lo), f2_lteq(pt, hi)));
}

static bool quadtree_insert(quadtree_t* qt, i32 n, tri2d_t tri, i32 iDrawable, i32 iVert)
{
    if (n < qt->nodeCount)
    {
        if (BoxHoldsTri(qt->boxes[n], tri))
        {
            for (i32 i = 0; i < 4; ++i)
            {
                if (quadtree_insert(qt, GetChild(n, i), tri, iDrawable, iVert))
                {
                    return true;
                }
            }
            i32 len = qt->listLens[n] + 1;
            qt->listLens[n] = len;
            PermReserve(qt->triLists[n], len);
            qt->triLists[n][len - 1] = tri;
            PermReserve(qt->indexLists[n], len);
            qt->indexLists[n][len - 1] = i2_v(iDrawable, iVert);
            return true;
        }
    }
    return false;
}

static float quadtree_find(
    const quadtree_t* qt,
    i32 n,
    float2 pt,
    float limit,
    int2* indOut)
{
    if (n < qt->nodeCount)
    {
        if (BoxHoldsPt(qt->boxes[n], pt))
        {
            i32 chosen = -1;
            float chosenDist = limit;
            i32 len = qt->listLens[n];
            const tri2d_t* triList = qt->triLists[n];
            for (i32 i = 0; i < len; ++i)
            {
                tri2d_t tri = triList[i];
                float dist = sdTriangle2D(tri.a, tri.b, tri.c, pt);
                if (dist < chosenDist)
                {
                    chosenDist = dist;
                    chosen = i;
                }
            }

            int2 ind = { -1, -1 };
            if (chosenDist < limit)
            {
                limit = chosenDist;
                ind = qt->indexLists[n][chosen];
            }

            for (i32 i = 0; i < 4; ++i)
            {
                int2 childInd = { -1, -1 };
                float childDist = quadtree_find(qt, GetChild(n, i), pt, limit, &childInd);
                if (childDist < limit)
                {
                    limit = childDist;
                    ind = childInd;
                }
            }
            *indOut = ind;
        }
    }
    return limit;
}

typedef struct embed_s
{
    task_t task;
    lightmap_t* lightmaps;
    const quadtree_t* trees;
    i32 lmCount;
} embed_t;

static void EmbedAttributesFn(task_t* pbase, i32 begin, i32 end)
{
    embed_t* task = (embed_t*)pbase;
    lightmap_t* lightmaps = task->lightmaps;
    const quadtree_t* trees = task->trees;
    const i32 lmCount = task->lmCount;
    const i32 lmSize = lightmaps[0].size;
    const i32 lmLen = lmSize * lmSize;
    const int2 size = { lmSize, lmSize };
    const float limit = 4.0f / lmSize;

    const drawables_t* drawables = drawables_get();
    const i32 drawablesCount = drawables->count;
    const meshid_t* pim_noalias meshids = drawables->meshes;
    const float4x4* pim_noalias matrices = drawables->matrices;

    for (i32 iWork = begin; iWork < end; ++iWork)
    {
        const i32 iLightmap = iWork / lmLen;
        const i32 iTexel = iWork % lmLen;
        const i32 x = iTexel % lmSize;
        const i32 y = iTexel / lmSize;
        const float2 uv = CoordToUv(size, i2_v(x, y));
        lightmap_t lightmap = lightmaps[iLightmap];
        const quadtree_t* qt = trees + iLightmap;

        lightmap.sampleCounts[iTexel] = 0.0f;

        int2 ind;
        float dist = quadtree_find(qt, 0, uv, limit, &ind);
        if ((dist < limit) && (ind.x != -1) && (ind.y != -1))
        {
            i32 iDraw = ind.x;
            i32 iVert = ind.y;
            mesh_t mesh;

            if (mesh_get(meshids[iDraw], &mesh))
            {
                const i32 a = iVert + 0;
                const i32 b = iVert + 1;
                const i32 c = iVert + 2;
                float2 LMA = f3_f2(mesh.lmUvs[a]);
                float2 LMB = f3_f2(mesh.lmUvs[b]);
                float2 LMC = f3_f2(mesh.lmUvs[c]);
                float area = sdEdge2D(LMA, LMB, LMC);
                if (area > 0.0f)
                {
                    const float rcpArea = 1.0f / area;
                    const float4x4 M = matrices[iDraw];
                    float4 A = f4x4_mul_pt(M, mesh.positions[a]);
                    float4 B = f4x4_mul_pt(M, mesh.positions[b]);
                    float4 C = f4x4_mul_pt(M, mesh.positions[c]);
                    const float3x3 IM = f3x3_IM(M);
                    float4 NA = f3x3_mul_col(IM, mesh.normals[a]);
                    float4 NB = f3x3_mul_col(IM, mesh.normals[a]);
                    float4 NC = f3x3_mul_col(IM, mesh.normals[a]);
                    NA = f4_normalize3(NA);
                    NB = f4_normalize3(NB);
                    NC = f4_normalize3(NC);

                    float4 wuv = bary2D(LMA, LMB, LMC, rcpArea, uv);
                    float4 P = f4_blend(A, B, C, wuv);
                    float4 N = f4_blend(NA, NB, NC, wuv);
                    N = f4_normalize3(N);

                    lightmap.position[iTexel] = f4_f3(P);
                    lightmap.normal[iTexel] = f4_f3(N);
                    lightmap.sampleCounts[iTexel] = 1.0f;
                }
            }
        }
    }
}

static void EmbedAttributes(lightmap_t* lightmaps, i32 lmCount)
{
    if (lmCount > 0)
    {
        quadtree_t* trees = tmp_calloc(sizeof(trees[0]) * lmCount);
        {
            box2d_t bounds;
            bounds.center = f2_s(0.5f);
            bounds.extents = f2_s(0.51f);
            for (i32 i = 0; i < lmCount; ++i)
            {
                quadtree_new(trees + i, 5, bounds);
            }
        }

        {
            const drawables_t* drawables = drawables_get();
            const i32 dwCount = drawables->count;
            const meshid_t* meshids = drawables->meshes;
            for (i32 d = 0; d < dwCount; ++d)
            {
                mesh_t mesh;
                if (mesh_get(meshids[d], &mesh))
                {
                    for (i32 v = 0; (v + 3) <= mesh.length; v += 3)
                    {
                        i32 l = (i32)mesh.lmUvs[v + 0].z;
                        if ((l >= 0) && (l < lmCount))
                        {
                            float2 A = f3_f2(mesh.lmUvs[v + 0]);
                            float2 B = f3_f2(mesh.lmUvs[v + 1]);
                            float2 C = f3_f2(mesh.lmUvs[v + 2]);
                            tri2d_t tri = { A, B, C };
                            quadtree_t* tree = trees + l;
                            bool inserted = quadtree_insert(tree, 0, tri, d, v);
                            ASSERT(inserted);
                        }
                    }
                }
            }
        }

        embed_t* task = tmp_calloc(sizeof(*task));
        task->lightmaps = lightmaps;
        task->trees = trees;
        task->lmCount = lmCount;
        task_run(&task->task, EmbedAttributesFn, TexelCount(lightmaps, lmCount));

        for (i32 i = 0; i < lmCount; ++i)
        {
            quadtree_del(trees + i);
        }
    }
}

lmpack_t lmpack_pack(
    i32 atlasSize,
    float texelsPerUnit,
    float distThresh,
    float degThresh)
{
    ASSERT(atlasSize > 0);

    if (!ms_once)
    {
        ms_once = true;
        cmd_reg("lm_print", CmdPrintLm);
    }

    float maxWidth = atlasSize / 3.0f;

    i32 nodeCount = 0;
    chartnode_t* nodes = chartnodes_create(texelsPerUnit, &nodeCount);

    i32 chartCount = 0;
    chart_t* charts = chart_group(
        nodes, nodeCount, &chartCount, distThresh, degThresh, maxWidth);

    chart_sort(charts, chartCount);

    i32 atlasCount = atlases_create(atlasSize, charts, chartCount);

    lmpack_t pack = { 0 };
    pack.lmCount = atlasCount;
    pack.lmSize = atlasSize;
    pack.lightmaps = perm_calloc(sizeof(pack.lightmaps[0]) * atlasCount);

    for (i32 i = 0; i < atlasCount; ++i)
    {
        lightmap_new(pack.lightmaps + i, atlasSize);
    }

    chartnodes_assign(charts, chartCount, pack.lightmaps, atlasCount);

    EmbedAttributes(pack.lightmaps, atlasCount);

    pim_free(nodes);
    for (i32 i = 0; i < chartCount; ++i)
    {
        chart_del(charts + i);
    }
    pim_free(charts);

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

typedef struct bake_s
{
    task_t task;
    const pt_scene_t* scene;
    i32 bounces;
    float timeSlice;
} bake_t;

static void BakeFn(task_t* pbase, i32 begin, i32 end)
{
    bake_t* task = (bake_t*)pbase;
    const pt_scene_t* scene = task->scene;
    const i32 bounces = task->bounces;
    const float timeSlice = task->timeSlice;

    lmpack_t* pack = lmpack_get();
    const i32 lmSize = pack->lmSize;
    const i32 lmLen = lmSize * lmSize;
    const float rcpSize = 1.0f / lmSize;
    const int2 size = { lmSize, lmSize };

    prng_t rng = prng_get();
    for (i32 iWork = begin; iWork < end; ++iWork)
    {
        i32 iLightmap = iWork / lmLen;
        i32 iTexel = iWork % lmLen;
        lightmap_t lightmap = pack->lightmaps[iLightmap];

        float sampleCount = lightmap.sampleCounts[iTexel];
        if (sampleCount > 0.0f)
        {
            float weight = 1.0f / sampleCount;
            float prob = f1_lerp(timeSlice, timeSlice * 2.0f, weight);
            if (prng_f32(&rng) < prob)
            {
                sampleCount += 1.0f;
                lightmap.sampleCounts[iTexel] = sampleCount;

                float2 jit = f2_rand(&rng);
                i32 x = iTexel % lmSize;
                i32 y = iTexel / lmSize;
                float2 uv = { (x + jit.x) * rcpSize, (y + jit.y) * rcpSize };
                float3 P3 = UvBilinearClamp_f3(lightmap.position, size, uv);
                float3 N3 = UvBilinearClamp_f3(lightmap.normal, size, uv);
                float4 P = f3_f4(P3, 1.0f);
                float4 N = f4_normalize3(f3_f4(N3, 0.0f));

                float4 rd;
                float pdf = ScatterLambertian(&rng, N, N, &rd, 1.0f);
                ray_t ray = { P, rd };
                pt_result_t result = pt_trace_ray(&rng, scene, ray, bounces);
                result.color = f3_mulvs(result.color, pdf);

                lightmap.color[iTexel] = f3_lerp(lightmap.color[iTexel], result.color, weight);
            }
        }
    }
    prng_set(rng);
}

ProfileMark(pm_Bake, lmpack_bake)
void lmpack_bake(const pt_scene_t* scene, i32 bounces, float timeSlice)
{
    ProfileBegin(pm_Bake);
    ASSERT(scene);

    const lmpack_t* pack = lmpack_get();
    i32 texelCount = TexelCount(pack->lightmaps, pack->lmCount);
    if (texelCount > 0)
    {
        bake_t* task = perm_calloc(sizeof(*task));
        task->scene = scene;
        task->bounces = bounces;
        task->timeSlice = timeSlice;
        task_run(&task->task, BakeFn, texelCount);
    }

    ProfileEnd(pm_Bake);
}

ProfileMark(pm_Denoise, lmpack_denoise)
void lmpack_denoise(void)
{
    ProfileBegin(pm_Denoise);

    lmpack_t* pack = lmpack_get();
    ASSERT(pack);
    for (i32 i = 0; i < pack->lmCount; ++i)
    {
        lightmap_t lm = pack->lightmaps[i];
        Denoise(DenoiseType_Lightmap, i2_s(lm.size), lm.color, NULL, NULL, lm.denoised);
    }

    ProfileEnd(pm_Denoise);
}

static cmdstat_t CmdPrintLm(i32 argc, const char** argv)
{
    char filename[PIM_PATH] = { 0 };
    const char* prefix = "lightmap";
    LmChannel channel = LmChannel_Color;
    if (argc > 1)
    {
        const char* arg1 = argv[1];
        if (arg1)
        {
            if (StrICmp(arg1, 16, "denoised") == 0)
            {
                channel = LmChannel_Denoised;
            }
            else if (StrICmp(arg1, 16, "color") == 0)
            {
                channel = LmChannel_Color;
            }
            else if (StrICmp(arg1, 16, "position") == 0)
            {
                channel = LmChannel_Position;
            }
            else if (StrICmp(arg1, 16, "normal") == 0)
            {
                channel = LmChannel_Normal;
            }
            else
            {
                prefix = arg1;
            }
        }
    }

    u32* buffer = NULL;
    const lmpack_t* pack = lmpack_get();
    for (i32 i = 0; i < pack->lmCount; ++i)
    {
        const lightmap_t lm = pack->lightmaps[i];
        const i32 len = lm.size * lm.size;
        buffer = tmp_realloc(buffer, sizeof(buffer[0]) * len);

        const float3* pim_noalias srcBuffer = lm.color;
        switch (channel)
        {
        default:
        case LmChannel_Color:
            srcBuffer = lm.color;
            break;
        case LmChannel_Denoised:
            srcBuffer = lm.denoised;
            break;
        case LmChannel_Position:
            srcBuffer = lm.position;
            break;
        case LmChannel_Normal:
            srcBuffer = lm.normal;
            break;
        }

        if ((channel == LmChannel_Color) || (channel == LmChannel_Denoised))
        {
            for (i32 j = 0; j < len; ++j)
            {
                float4 ldr = tmap4_reinhard(f3_f4(srcBuffer[j], 1.0f));
                u32 color = LinearToColor(ldr);
                color |= 0xff << 24;
                buffer[j] = color;
            }
        }
        if (channel == LmChannel_Position)
        {
            for (i32 j = 0; j < len; ++j)
            {
                float3 pos = srcBuffer[j];
                pos = f3_divvs(pos, 100.0f);
                u32 color = f4_rgba8(f3_f4(pos, 1.0f));
                color |= 0xff << 24;
                buffer[j] = color;
            }
        }
        if (channel == LmChannel_Normal)
        {
            for (i32 j = 0; j < len; ++j)
            {
                float3 norm = srcBuffer[j];
                norm = f3_mulvs(norm, 0.5f);
                norm = f3_addvs(norm, 0.5f);
                u32 color = f4_rgba8(f3_f4(norm, 1.0f));
                color |= 0xff << 24;
                buffer[j] = color;
            }
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
