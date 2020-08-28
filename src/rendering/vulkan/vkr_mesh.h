#pragma once

#include "rendering/vulkan/vkr.h"
#include "math/types.h"

PIM_C_BEGIN

bool vkrMesh_New(
    vkrMesh* mesh,
    i32 length,
    const float4* pim_noalias positions,
    const float4* pim_noalias normals,
    const float4* pim_noalias uv01);
void vkrMesh_Del(vkrMesh* mesh);

PIM_C_END
