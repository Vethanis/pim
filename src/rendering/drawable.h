#pragma once

#include "common/macro.h"
#include "math/types.h"
#include "rendering/mesh.h"
#include "rendering/material.h"

PIM_C_BEGIN

typedef struct task_s task_t;
typedef struct camera_s camera_t;
typedef struct framebuf_s framebuf_t;

typedef struct drawables_s
{
    i32 count;
    u32* names;             // hashstring identifier
    meshid_t* meshes;       // immutable object space mesh
    material_t* materials;  // material description
    meshid_t* tmpMeshes;    // per-frame world space mesh
    sphere_t* bounds;       // world space bounds
    u64* tileMasks;         // bitmask of tile visibility
    float4x4* matrices;     // local to world matrix
    float4* translations;
    quat* rotations;
    float4* scales;
} drawables_t;

drawables_t* drawables_get(void);

i32 drawables_add(u32 name);
bool drawables_rm(u32 name);
i32 drawables_find(u32 name);

task_t* drawables_trs(void);
task_t* drawables_bounds(void);
task_t* drawables_cull(
    const camera_t* camera,
    const framebuf_t* backBuf);

PIM_C_END
