#pragma once

#include "common/macro.h"
#include "common/dbytes.h"
#include "common/guid.h"
#include "math/types.h"

PIM_C_BEGIN

typedef struct MeshId_s MeshId;
typedef struct Material_s Material;
typedef struct Crate_s Crate;

typedef struct Entities_s
{
    i32 count;
    Guid* pim_noalias names;          // hash identifier
    MeshId* pim_noalias meshes;       // immutable object space mesh
    Box3D* pim_noalias bounds;          // object space bounds
    Material* pim_noalias materials;  // material description
    float4x4* pim_noalias matrices;     // local to world matrix
    float3x3* pim_noalias invMatrices;  // world to local rotation matrix
    float4* pim_noalias translations;
    quat* pim_noalias rotations;
    float4* pim_noalias scales;
    u64 modtime;
} Entities;

#define kDiskEntitiesVersion 3
typedef struct DiskEntities_s
{
    i32 version;
    i32 length;
    DiskBytes names;
    DiskBytes meshes;    // dmeshid_t
    DiskBytes bounds;
    DiskBytes materials; // dmaterial_t
    DiskBytes translations;
    DiskBytes rotations;
    DiskBytes scales;
} DiskEntities;

void EntSys_Init(void);
void EntSys_Update(void);
void EntSys_Shutdown(void);
void EntSys_Gui(bool* enabled);

Entities *const Entities_Get(void);

i32 Entities_Add(Entities *const dr, Guid name);
bool Entities_Rm(Entities *const dr, Guid name);
i32 Entities_Find(Entities const *const dr, Guid name);
void Entities_Clear(Entities *const dr);
void Entities_Del(Entities *const dr);

void Entities_UpdateBounds(Entities *const dr);
void Entities_UpdateTransforms(Entities *const dr);
Box3D Entities_GetBounds(Entities const *const dr);

bool Entities_Save(Crate *const crate, Entities const *const src);
bool Entities_Load(Crate *const crate, Entities *const dst);

PIM_C_END
