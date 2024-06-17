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
    float4* pim_noalias tangents;
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

void MeshSys_Init(void);
void MeshSys_Update(void);
void MeshSys_Shutdown(void);
void MeshSys_Gui(bool* pEnabled);

bool Mesh_New(Mesh *const mesh, Guid name, MeshId *const idOut);

bool Mesh_Exists(MeshId id);

void Mesh_Retain(MeshId id);
void Mesh_Release(MeshId id);

Mesh *const Mesh_Get(MeshId id);

bool Mesh_Find(Guid name, MeshId *const idOut);
bool Mesh_GetName(MeshId id, Guid *const dst);

Box3D Mesh_CalcBounds(MeshId id);

bool Mesh_Save(Crate *const crate, MeshId id, Guid *const dst);
bool Mesh_Load(Crate *const crate, Guid name, MeshId *const dst);

bool Mesh_Upload(MeshId id);

PIM_C_END
