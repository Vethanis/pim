#pragma once

#include "common/macro.h"
#include "common/dbytes.h"
#include "common/guid.h"
#include "math/types.h"

PIM_C_BEGIN

typedef struct meshid_s meshid_t;
typedef struct material_s material_t;
typedef struct crate_s crate_t;

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
} ddrawables_t;

void drawables_init(void);
void drawables_update(void);
void drawables_shutdown(void);
void drawables_gui(bool* enabled);

drawables_t *const drawables_get(void);

i32 drawables_add(drawables_t *const dr, guid_t name);
bool drawables_rm(drawables_t *const dr, guid_t name);
i32 drawables_find(drawables_t const *const dr, guid_t name);
void drawables_clear(drawables_t *const dr);
void drawables_del(drawables_t *const dr);

void drawables_updatebounds(drawables_t *const dr);
void drawables_updatetransforms(drawables_t *const dr);
box_t drawables_bounds(drawables_t const *const dr);

bool drawables_save(crate_t *const crate, drawables_t const *const src);
bool drawables_load(crate_t *const crate, drawables_t *const dst);


PIM_C_END
