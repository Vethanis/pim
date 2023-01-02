#pragma once

#include "common/macro.h"
#include "math/types.h"

PIM_C_BEGIN

// ----------------------------------------------------------------------------
// system

bool vkrImSys_Init(void);
void vkrImSys_Shutdown(void);

void vkrImSys_Clear(void);
void vkrImSys_Flush(void);

void vkrImSys_Draw(void);
void vkrImSys_DrawDepth(void);

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
