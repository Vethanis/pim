#pragma once

#include "common/macro.h"
#include "math/types.h"
#include "common/guid.h"
#include "common/dbytes.h"
#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

typedef struct crate_s crate_t;
typedef struct material_s material_t;

typedef struct meshid_s
{
    u32 version : 8;
    u32 index : 24;
} meshid_t;

typedef struct dmeshid_s
{
    guid_t id;
} dmeshid_t;

typedef struct mesh_s
{
    float4* pim_noalias positions;
    float4* pim_noalias normals;
    float4* pim_noalias uvs;
    int4* pim_noalias texIndices;
    i32 length;
    vkrMeshId id;
} mesh_t;

#define kMeshVersion 5
typedef struct dmesh_s
{
    i32 version;
    i32 length;
    char name[64];
} dmesh_t;

void mesh_sys_init(void);
void mesh_sys_update(void);
void mesh_sys_shutdown(void);
void mesh_sys_gui(bool* pEnabled);

bool mesh_new(mesh_t *const mesh, guid_t name, meshid_t *const idOut);

bool mesh_exists(meshid_t id);

void mesh_retain(meshid_t id);
void mesh_release(meshid_t id);

mesh_t *const mesh_get(meshid_t id);

bool mesh_find(guid_t name, meshid_t *const idOut);
bool mesh_getname(meshid_t id, guid_t *const dst);

box_t mesh_calcbounds(meshid_t id);

bool mesh_setmaterial(meshid_t id, material_t const *const mat);
bool VEC_CALL mesh_settransform(meshid_t id, float4x4 localToWorld);
// upload changes to gpu
bool mesh_update(mesh_t *const mesh);

bool mesh_save(crate_t *const crate, meshid_t id, guid_t *const dst);
bool mesh_load(crate_t *const crate, guid_t name, meshid_t *const dst);

PIM_C_END
