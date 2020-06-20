#include "rendering/gi_grid.h"
#include "allocator/allocator.h"
#include "math/scalar.h"
#include "math/float4_funcs.h"
#include "math/sphgauss.h"
#include "math/sampling.h"
#include "math/lighting.h"
#include <string.h>

static gigrid_t ms_instance;
static float4 ms_axii[kGiDirections];
static float ms_integrals[kGiDirections];

static bool ms_once;
static void EnsureInit()
{
    if (!ms_once)
    {
        ms_once = true;
        SG_Generate(ms_axii, ms_integrals, kGiDirections, SGDist_Sphere);
    }
}

gigrid_t* gigrid_get(void) { return &ms_instance; }

void gigrid_new(gigrid_t* grid, i32 size, box_t bounds)
{
    EnsureInit();

    ASSERT(grid);
    ASSERT(size >= 0);

    memset(grid, 0, sizeof(*grid));

    const i32 length = size * size * size;
    ASSERT(length >= 0); // catch signed int overflow
    ASSERT(length < (1 << 20)); // probably don't want over a million of these

    bounds.center.w = 0.0f;
    bounds.extents.w = 0.0f;

    grid->lo = f4_sub(bounds.center, bounds.extents);
    grid->hi = f4_add(bounds.center, bounds.extents);
    grid->probesPerMeter = f4_mulvs(f4_rcp(f4_sub(grid->hi, grid->lo)), (float)size);
    grid->probesPerMeter.w = 0.0f;
    grid->stride = f4_v(1.0f, (float)size, (float)(size*size), 0.0f);
    grid->size = size;
    grid->length = length;
    grid->rcpSize = 1.0f / size;
    grid->probes = perm_calloc(sizeof(grid->probes[0]) * length);
}

void gigrid_del(gigrid_t* grid)
{
    if (grid)
    {
        pim_free(grid->probes);
        memset(grid, 0, sizeof(*grid));
    }
}

//float4 VEC_CALL gigrid_sample(const gigrid_t* grid, float4 position, float4 direction)
//{
//    ASSERT(grid);
//    ASSERT(grid->probes);
//
//    float4 offsetMeters = f4_sub(position, grid->lo);
//    float4 offsetProbes = f4_mul(offsetMeters, grid->probesPerMeter);
//    float4 frac = f4_frac(offsetProbes);
//    offsetProbes = f4_floor(offsetProbes);
//
//    const float cap = grid->size - 1.0f;
//    float x0 = f1_clamp(offsetProbes.x, 0.0f, cap);
//    float x1 = f1_clamp(offsetProbes.x + 1.0f, 0.0f, cap);
//    float y0 = f1_clamp(offsetProbes.y, 0.0f, cap);
//    float y1 = f1_clamp(offsetProbes.y + 1.0f, 0.0f, cap);
//    float z0 = f1_clamp(offsetProbes.z, 0.0f, cap);
//    float z1 = f1_clamp(offsetProbes.z + 1.0f, 0.0f, cap);
//    const float w = 0.0f;
//
//    const float4 stride = grid->stride;
//    i32 indices[8];
//    float weights[8];
//    float4 light = f4_0;
//    for (i32 i = 0; i < 8; ++i)
//    {
//        float4 pt;
//        pt.x = (i & 1) ? x1 : x0;
//        pt.y = (i & 2) ? y1 : y0;
//        pt.z = (i & 4) ? z1 : z0;
//        pt.w = 0.0f;
//        i32 index = (i32)f4_dot3(pt, stride);
//        indices[i] = index;
//        ASSERT(index >= 0);
//        ASSERT(index < grid->length);
//        float wx = (i & 1) ? frac.x : 1.0f - frac.x;
//        float wy = (i & 2) ? frac.y : 1.0f - frac.y;
//        float wz = (i & 4) ? frac.z : 1.0f - frac.z;
//        float weight = wx * wy * wz;
//        weights[i] = weight;
//
//        if (weight > kEpsilon)
//        {
//            const giprobe_t* probe = grid->probes + index;
//            for (i32 j = 0; j < kGiDirections; ++j)
//            {
//                float4 amplitude = probe->Values[j];
//                float4 axis = ms_axii[j];
//                float4 irradiance = SG_Irradiance(axis, amplitude, direction);
//            }
//        }
//    }
//}

void VEC_CALL gigrid_blend(gigrid_t* grid, float4 position, float4 direction, float4 radiance, float weight);

void gigrid_bake(gigrid_t* grid, const pt_scene_t* scene, float weight);