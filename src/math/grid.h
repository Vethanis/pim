#pragma once

#include "math/types.h"
#include "math/float4_funcs.h"

typedef struct grid_s
{
    box_t bounds;
    int3 size;
    float cellsPerUnit;
} grid_t;

static void VEC_CALL grid_new(grid_t* grid, box_t bounds, float cellsPerUnit)
{
    float4 range = f4_sub(bounds.hi, bounds.lo);
    float4 sizef = f4_mulvs(range, cellsPerUnit);
    int3 size = { (i32)(sizef.x + 0.5f), (i32)(sizef.y + 0.5f), (i32)(sizef.z + 0.5f) };
    grid->bounds = bounds;
    grid->size = size;
    grid->cellsPerUnit = cellsPerUnit;
}

static i32 VEC_CALL grid_len(const grid_t* grid)
{
    int3 size = grid->size;
    return size.x * size.y * size.z;
}

pim_inline float4 VEC_CALL grid_position(const grid_t* grid, i32 index)
{
    const int3 size = grid->size;
    const float unitsPerCell = 1.0f / grid->cellsPerUnit;
    i32 ix = index % size.x;
    i32 iy = (index / size.x) % size.y;
    i32 iz = index / (size.x * size.y);
    float4 offset = { (ix + 0.5f) * unitsPerCell, (iy + 0.5f) * unitsPerCell, (iz + 0.5f) * unitsPerCell };
    return f4_add(grid->bounds.lo, offset);
}

pim_inline i32 VEC_CALL grid_index(const grid_t* grid, float4 position)
{
    float4 offset = f4_mulvs(f4_sub(position, grid->bounds.lo), grid->cellsPerUnit);
    const int3 size = grid->size;
    i32 x = i1_clamp((i32)offset.x, 0, size.x - 1);
    i32 y = i1_clamp((i32)offset.y, 0, size.y - 1);
    i32 z = i1_clamp((i32)offset.z, 0, size.z - 1);
    return x + y * size.x + z * size.x * size.y;
}
