#pragma once

#include "common/macro.h"
#include "math/types.h"

PIM_C_BEGIN

typedef struct mesh_s
{
    float4* positions;
    float4* normals;
    float2* uvs;
    sphere_t bounds;
    i32 length;
} mesh_t;

typedef struct meshid_s
{
    i32 index;
    i32 version;
} meshid_t;

meshid_t mesh_new(mesh_t* mesh, const char* name);

bool mesh_exists(meshid_t id);

void mesh_retain(meshid_t id);
void mesh_release(meshid_t id);

bool mesh_get(meshid_t id, mesh_t* dst);
bool mesh_set(meshid_t id, mesh_t* src);

bool mesh_find(const char* name, meshid_t* idOut);

sphere_t mesh_calcbounds(meshid_t id);

PIM_C_END
