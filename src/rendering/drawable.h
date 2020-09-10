#pragma once

#include "common/macro.h"
#include "common/dbytes.h"
#include "common/guid.h"
#include "math/types.h"

PIM_C_BEGIN

typedef struct lm_uvs_s lm_uvs_t;
typedef struct meshid_s meshid_t;
typedef struct material_s material_t;

typedef struct drawables_s
{
    i32 count;
    guid_t* pim_noalias names;          // hash identifier
    meshid_t* pim_noalias meshes;       // immutable object space mesh
    box_t* pim_noalias bounds;          // object space bounds
    material_t* pim_noalias materials;  // material description
    float4x4* pim_noalias matrices;     // local to world matrix
    float3x3* pim_noalias invMatrices;  // world to local rotation matrix
    float4* pim_noalias translations;
    quat* pim_noalias rotations;
    float4* pim_noalias scales;
} drawables_t;

#define kDrawablesVersion 3
typedef struct ddrawables_s
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
    guid_t lmpack;
} ddrawables_t;

drawables_t* drawables_get(void);

i32 drawables_add(drawables_t* dr, guid_t name);
bool drawables_rm(drawables_t* dr, guid_t name);
i32 drawables_find(const drawables_t* dr, guid_t name);
void drawables_clear(drawables_t* dr);
void drawables_del(drawables_t* dr);

void drawables_updatebounds(drawables_t* dr);
void drawables_updatetransforms(drawables_t* dr);
box_t drawables_bounds(const drawables_t* dr);

bool drawables_save(const drawables_t* src, guid_t name);
bool drawables_load(drawables_t* dst, guid_t name);

PIM_C_END
