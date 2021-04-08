#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrImMesh_Init(void);
void vkrImMesh_Update(void);
void vkrImMesh_Shutdown(void);

void vkrImMesh_Clear(void);

bool vkrImMesh_Add(
    const float4* pim_noalias positions,
    const float4* pim_noalias normals,
    const float4* pim_noalias uvs,
    const int4* pim_noalias texIndices,
    i32 vertCount);

void VEC_CALL vkrImMesh_Vertex(float4 pos, float4 nor, float4 uv, int4 itex);

void vkrImMesh_Draw(VkCommandBuffer cmd);
void vkrImMesh_DrawPosition(VkCommandBuffer cmd);

PIM_C_END
