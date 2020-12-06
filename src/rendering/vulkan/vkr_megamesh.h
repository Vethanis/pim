#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

typedef struct mesh_s mesh_t;

// MegaMesh:
// buffer large enough to hold all mesh data.
// attributes are laid out in struct of arrays form.
// a compute shader can then cull individual primitives and build an index buffer.

bool vkrMegaMesh_Init(void);
void vkrMegaMesh_Update(void);
void vkrMegaMesh_Shutdown(void);

bool vkrMegaMesh_Exists(vkrMeshId id);
vkrMeshId vkrMegaMesh_Alloc(i32 vertCount);
bool vkrMegaMesh_Free(vkrMeshId id);

bool vkrMegaMesh_Set(
    vkrMeshId id,
    const float4* positions,
    const float4* normals,
    const float4* uvs,
    const int4* texIndices,
    i32 vertCount);

void vkrMegaMesh_Draw(VkCommandBuffer cmd);
void vkrMegaMesh_DrawPosition(VkCommandBuffer cmd);

PIM_C_END
