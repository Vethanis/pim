#include "rendering/gi_grid.h"
#include "allocator/allocator.h"
#include "math/scalar.h"
#include "math/float4_funcs.h"
#include "math/float3_funcs.h"
#include "math/int3_funcs.h"
#include "math/sphgauss.h"
#include "math/sampling.h"
#include "math/lighting.h"
#include "threading/task.h"
#include "rendering/path_tracer.h"
#include "common/profiler.h"
#include <string.h>

static gigrid_t ms_instance;
gigrid_t* gigrid_get(void) { return &ms_instance; }

void gigrid_new(gigrid_t* grid, float probesPerMeter, box_t bounds)
{
    ASSERT(grid);

    memset(grid, 0, sizeof(*grid));
    SG_Generate(grid->axii, kGiDirections, SGDist_Sphere);

    bounds.center.w = 0.0f;
    bounds.extents.w = 0.0f;
    float4 lo = f4_sub(bounds.center, bounds.extents);
    float4 hi = f4_add(bounds.center, bounds.extents);
    float4 range = f4_sub(hi, lo);
    float4 sizef = f4_mulvs(range, probesPerMeter);
    sizef = f4_ceil(sizef);

    int3 size = { (i32)sizef.x, (i32)sizef.y, (i32)sizef.z };
    const i32 length = size.x * size.y * size.z;
    ASSERT(length >= 0);
    ASSERT(length < (1 << 20)); // probably don't want over a million of these

    grid->lo = lo;
    grid->hi = hi;
    grid->rcpRange = f4_rcp(range);
    grid->size = size;
    grid->probes = perm_calloc(sizeof(grid->probes[0]) * length);
    grid->weights = perm_calloc(sizeof(grid->weights[0]) * length);
    grid->positions = perm_calloc(sizeof(grid->positions[0]) * length);

    float4* positions = grid->positions;
    const float dz = 1.0f / range.z;
    const float dy = 1.0f / range.y;
    const float dx = 1.0f / range.x;
    for (i32 z = 0; z < size.z; ++z)
    {
        float zf = lo.z + dz * (z+0.5f);
        for (i32 y = 0; y < size.y; ++y)
        {
            float yf = lo.y + dy * (y+0.5f);
            for (i32 x = 0; x < size.x; ++x)
            {
                float xf = lo.x + dx * (x + 0.5f);
                float4 position = f4_v(xf, yf, zf, 0.0f);
                i32 i = x + y * size.x + z * size.x * size.y;
                positions[i] = position;
            }
        }
    }
}

void gigrid_del(gigrid_t* grid)
{
    if (grid)
    {
        pim_free(grid->probes);
        pim_free(grid->weights);
        pim_free(grid->positions);
        memset(grid, 0, sizeof(*grid));
    }
}

pim_inline float3 VEC_CALL UvToCoordf3(int3 size, float3 uvw)
{
    return f3_addvs(f3_mul(uvw, i3_f3(size)), 0.5f);
}

pim_inline i32 VEC_CALL Clamp3(int3 size, int3 coord)
{
    coord.x = i1_clamp(coord.x, 0, size.x - 1);
    coord.y = i1_clamp(coord.y, 0, size.y - 1);
    coord.z = i1_clamp(coord.z, 0, size.z - 1);
    i32 index = coord.x + coord.y * size.x + coord.z * size.x * size.y;
    return index;
}

typedef struct trilinear_s
{
    i32 indices[8];
    float3 frac;
} trilinear_t;

pim_inline trilinear_t VEC_CALL UvTrilinearClamp(int3 size, float3 uvw)
{
    trilinear_t t;

    float3 coordf = UvToCoordf3(size, uvw);
    t.frac = f3_frac(coordf);
    int3 coord = f3_i3(coordf);

    for (i32 i = 0; i < 8; ++i)
    {
        i32 x = (i & 1) ? 1 : 0;
        i32 y = (i & 2) ? 1 : 0;
        i32 z = (i & 4) ? 1 : 0;
        t.indices[i] = Clamp3(size, i3_add(coord, i3_v(x, y, z)));
    }

    return t;
}

pim_inline giprobe_t VEC_CALL SampleProbe(
    const giprobe_t* pim_noalias probes,
    int3 size,
    int3 coord)
{
    i32 index = Clamp3(size, coord);
    return probes[index];
}

pim_inline giprobe_t VEC_CALL BlendProbe(
    giprobe_t lhs,
    giprobe_t rhs,
    float alpha)
{
    giprobe_t result;
    for (i32 i = 0; i < kGiDirections; ++i)
    {
        result.Values[i] = f4_lerpvs(lhs.Values[i], rhs.Values[i], alpha);
    }
    return result;
}

pim_inline giprobe_t VEC_CALL UvTrilinearClamp_giprobe(
    const giprobe_t* pim_noalias probes,
    int3 size,
    float3 uvw)
{
    trilinear_t t = UvTrilinearClamp(size, uvw);

    giprobe_t samples[8];
    for (i32 i = 0; i < 8; ++i)
    {
        i32 index = t.indices[i];
        samples[i] = probes[index];
    }

    samples[0] = BlendProbe(samples[0], samples[1], t.frac.x);
    samples[1] = BlendProbe(samples[2], samples[3], t.frac.x);
    samples[2] = BlendProbe(samples[4], samples[5], t.frac.x);
    samples[3] = BlendProbe(samples[6], samples[7], t.frac.x);

    samples[0] = BlendProbe(samples[0], samples[1], t.frac.y);
    samples[1] = BlendProbe(samples[2], samples[3], t.frac.y);

    samples[0] = BlendProbe(samples[0], samples[1], t.frac.z);

    return samples[0];
}

giprobe_t VEC_CALL gigrid_sample(
    const gigrid_t* grid,
    float4 position,
    float4 direction)
{
    ASSERT(grid);
    ASSERT(grid->probes);

    float3 uv = f4_f3(f4_mul(f4_sub(position, grid->lo), grid->rcpRange));
    giprobe_t probe = UvTrilinearClamp_giprobe(grid->probes, grid->size, uv);
    return probe;
}

typedef struct task_bake_s
{
    task_t task;
    gigrid_t* grid;
    const pt_scene_t* scene;
    i32 iSample;
    i32 samplesPerProbe;
} task_bake_t;

static void bakefn(task_t* pbase, i32 begin, i32 end)
{
    task_bake_t* task = (task_bake_t*)pbase;
    gigrid_t* grid = task->grid;
    const pt_scene_t* scene = task->scene;
    const i32 iSample = task->iSample;
    const i32 samplesPerProbe = task->samplesPerProbe;
    const float4* pim_noalias axii = grid->axii;
    giprobe_t* pim_noalias probes = grid->probes;
    giprobeweights_t* pim_noalias weights = grid->weights;
    const float4* pim_noalias positions = grid->positions;

    prng_t rng = prng_get();

    for (i32 i = begin; i < end; ++i)
    {
        float4 ro = positions[i];
        for (i32 j = 0; j < samplesPerProbe; ++j)
        {
            float4 rd = SampleUnitSphere(f2_rand(&rng));
            pt_result_t result = pt_trace_ray(&rng, scene, (ray_t) { ro, rd });
            SG_Accumulate(
                iSample + j,
                rd,
                f3_f4(result.color, 0.0f),
                axii,
                probes[i].Values,
                weights[i].Values,
                kGiDirections);
        }
    }

    prng_set(rng);
}

ProfileMark(pm_gigrid_bake, gigrid_bake)
void gigrid_bake(gigrid_t* grid, const pt_scene_t* scene, i32 iSample, i32 samplesPerProbe)
{
    ProfileBegin(pm_gigrid_bake);

    ASSERT(grid);
    ASSERT(scene);
    ASSERT(iSample >= 0);

    int3 size = grid->size;
    i32 worksize = size.x * size.y * size.z;

    task_bake_t* task = tmp_calloc(sizeof(*task));
    task->grid = grid;
    task->scene = scene;
    task->iSample = iSample;
    task->samplesPerProbe = samplesPerProbe;
    task_run(&task->task, bakefn, worksize);

    ProfileEnd(pm_gigrid_bake);
}
