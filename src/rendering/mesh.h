#pragma once

#include "common/macro.h"
#include "math/types.h"

PIM_C_BEGIN

typedef struct mesh_s
{
    u64 version;
    float4* positions;
    float4* normals;
    float2* uvs;
    float3* lmUvs; // x: u, y: v, z: lm index
    sphere_t bounds;
    i32 length;
} mesh_t;

typedef struct meshid_s
{
    u64 version;
    void* handle;
} meshid_t;

// creates a versioned handle to your mesh pointer
meshid_t mesh_create(mesh_t* mesh);

// atomically releases the handle, freeing the mesh buffers and mesh pointer only once
bool mesh_destroy(meshid_t id);

bool mesh_current(meshid_t id);
bool mesh_get(meshid_t id, mesh_t* dst);

bool mesh_register(const char* name, meshid_t id);
meshid_t mesh_lookup(const char* name);

sphere_t mesh_calcbounds(meshid_t id);

PIM_C_END
