#pragma once

#include "rendering/vulkan/vkr.h"
#include "math/types.h"

PIM_C_BEGIN

bool vkrMesh_New(
    vkrMesh* mesh,
    i32 vertCount,
    const float4* pim_noalias positions,
    const float4* pim_noalias normals,
    const float4* pim_noalias uv01,
    i32 indexCount,
    const u16* pim_noalias indices);
void vkrMesh_Del(vkrMesh* mesh);

bool vkrMesh_Upload(
    vkrMesh* mesh,
    i32 vertCount,
    const float4* pim_noalias positions,
    const float4* pim_noalias normals,
    const float4* pim_noalias uv01,
    i32 indexCount,
    const u16* pim_noalias indices);

PIM_C_END
