#pragma once

#include "common/macro.h"
#include "math/types.h"
#include "common/guid.h"
#include "common/dbytes.h"
#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

typedef struct Crate_s Crate;
typedef struct Material_s Material;

typedef struct MeshId_s
{
    u32 version : 8;
    u32 index : 24;
} MeshId;

typedef struct DiskMeshId_s
{
    Guid id;
} DiskMeshId;

typedef struct Mesh_s
{
    float4* pim_noalias positions;
    float4* pim_noalias normals;
    float4* pim_noalias uvs;
    int4* pim_noalias texIndices;
    i32 length;
    vkrMeshId id;
} Mesh;

#define kMeshVersion 5
typedef struct DiskMesh_s
{
    i32 version;
    i32 length;
    char name[64];
} DiskMesh;

void mesh_sys_init(void);
void mesh_sys_update(void);
void mesh_sys_shutdown(void);
void mesh_sys_gui(bool* pEnabled);

bool mesh_new(Mesh *const mesh, Guid name, MeshId *const idOut);

bool mesh_exists(MeshId id);

void mesh_retain(MeshId id);
void mesh_release(MeshId id);

Mesh *const mesh_get(MeshId id);

bool mesh_find(Guid name, MeshId *const idOut);
bool mesh_getname(MeshId id, Guid *const dst);

Box3D mesh_calcbounds(MeshId id);

bool mesh_setmaterial(MeshId id, Material const *const mat);
bool VEC_CALL mesh_settransform(MeshId id, float4x4 localToWorld);
// upload changes to gpu
bool mesh_update(Mesh *const mesh);

bool mesh_save(Crate *const crate, MeshId id, Guid *const dst);
bool mesh_load(Crate *const crate, Guid name, MeshId *const dst);

PIM_C_END
