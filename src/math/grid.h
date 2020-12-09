#pragma once

#include "math/types.h"
#include "math/float4_funcs.h"

typedef struct grid_s
{
    box_t bounds;
    int3 size;
    float cellsPerMeter;
} grid_t;

pim_inline void VEC_CALL grid_new(grid_t *const grid, box_t bounds, float cellsPerMeter)
{
    float4 range = f4_sub(bounds.hi, bounds.lo);
    float4 sizef = f4_ceil(f4_mulvs(range, cellsPerMeter));
    int3 size = { (i32)sizef.x, (i32)sizef.y, (i32)sizef.z };
    grid->bounds = bounds;
    grid->size = size;
    grid->cellsPerMeter = cellsPerMeter;
}

pim_inline i32 VEC_CALL grid_len(grid_t const *const grid)
{
    int3 size = grid->size;
    return size.x * size.y * size.z;
}

pim_inline float4 VEC_CALL grid_position(grid_t const *const grid, i32 index)
{
    const int3 size = grid->size;
    const float metersPerCell = 1.0f / grid->cellsPerMeter;
    i32 ix = index % size.x;
    i32 iy = (index / size.x) % size.y;
    i32 iz = index / (size.x * size.y);
    float4 offset =
    {
        (ix + 0.5f) * metersPerCell,
        (iy + 0.5f) * metersPerCell,
        (iz + 0.5f) * metersPerCell,
        0.0f,
    };
    return f4_add(grid->bounds.lo, offset);
}

pim_inline i32 VEC_CALL grid_index(grid_t const *const grid, float4 position)
{
    float4 offset = f4_mulvs(f4_sub(position, grid->bounds.lo), grid->cellsPerMeter);
    const int3 size = grid->size;
    i32 x = i1_clamp((i32)offset.x, 0, size.x - 1);
    i32 y = i1_clamp((i32)offset.y, 0, size.y - 1);
    i32 z = i1_clamp((i32)offset.z, 0, size.z - 1);
    return x + y * size.x + z * size.x * size.y;
}
