#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "math/types.h"

typedef struct mesh_s
{
    u64 version;
    float4* positions;
    float4* normals;
    float2* uvs;
    i32 length;
} mesh_t;

typedef struct meshid_s
{
    u64 version;
    void* handle;
} meshid_t;

meshid_t mesh_create(mesh_t* src);
bool mesh_destroy(meshid_t id);

bool mesh_current(meshid_t id);
bool mesh_get(meshid_t id, mesh_t* dst);

PIM_C_END
