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
} Entities;

#define kDiskEntitiesVersion 3
typedef struct DiskEntities_s
{
    i32 version;
    i32 length;
    dbytes_t names;
    dbytes_t meshes;    // dmeshid_t
    dbytes_t bounds;
    dbytes_t materials; // dmaterial_t
    dbytes_t translations;
    dbytes_t rotations;
    dbytes_t scales;
} DiskEntities;

void drawables_init(void);
void drawables_update(void);
void drawables_shutdown(void);
void drawables_gui(bool* enabled);

Entities *const drawables_get(void);

i32 drawables_add(Entities *const dr, Guid name);
bool drawables_rm(Entities *const dr, Guid name);
i32 drawables_find(Entities const *const dr, Guid name);
void drawables_clear(Entities *const dr);
void drawables_del(Entities *const dr);

void drawables_updatebounds(Entities *const dr);
void drawables_updatetransforms(Entities *const dr);
Box3D drawables_bounds(Entities const *const dr);

bool drawables_save(Crate *const crate, Entities const *const src);
bool drawables_load(Crate *const crate, Entities *const dst);


PIM_C_END
