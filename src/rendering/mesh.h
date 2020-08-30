#pragma once

#include "common/macro.h"
#include "math/types.h"
#include "common/guid.h"
#include "common/dbytes.h"

PIM_C_BEGIN

typedef struct meshid_s
{
    u32 index : 24;
    u32 version : 8;
} meshid_t;

typedef struct dmeshid_s
{
    guid_t id;
} dmeshid_t;

typedef struct mesh_s
{
    float4* positions;
    float4* normals;
    float2* uvs;
    i32 length;
} mesh_t;

#define kMeshVersion 2
typedef struct dmesh_s
{
    i32 version;
    i32 length;
    dbytes_t positions;
    dbytes_t normals;
    dbytes_t uvs;
} dmesh_t;

void mesh_sys_init(void);
void mesh_sys_update(void);
void mesh_sys_shutdown(void);
void mesh_sys_gui(bool* pEnabled);

bool mesh_new(mesh_t* mesh, guid_t name, meshid_t* idOut);

bool mesh_exists(meshid_t id);

void mesh_retain(meshid_t id);
void mesh_release(meshid_t id);

bool mesh_get(meshid_t id, mesh_t* dst);
bool mesh_set(meshid_t id, mesh_t* src);

bool mesh_find(guid_t name, meshid_t* idOut);
bool mesh_getname(meshid_t id, guid_t* dst);

box_t mesh_calcbounds(meshid_t id);

bool mesh_save(meshid_t id, guid_t* dst);
bool mesh_load(guid_t name, meshid_t* dst);

PIM_C_END
