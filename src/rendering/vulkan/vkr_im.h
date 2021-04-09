#pragma once

#include "common/macro.h"
#include "math/types.h"

PIM_C_BEGIN

PIM_DECL_HANDLE(VkCommandBuffer);

// ----------------------------------------------------------------------------
// system

bool vkrImSys_Init(void);
void vkrImSys_Shutdown(void);

void vkrImSys_Clear(void);
void vkrImSys_Flush(VkCommandBuffer cmd);

void vkrImSys_Draw(VkCommandBuffer cmd);
void vkrImSys_DrawDepth(VkCommandBuffer cmd);

// ----------------------------------------------------------------------------
// user

void vkrIm_Begin(void);
void vkrIm_End(void);

void vkrIm_Mesh(
    const float4* pim_noalias positions,
    const float4* pim_noalias normals,
    const float4* pim_noalias uvs,
    const int4* pim_noalias texInds,
    i32 vertCount);

void VEC_CALL vkrIm_Vert(float4 pos, float4 nor, float4 uv, int4 itex);

PIM_C_END
