#pragma once

#include "rendering/vulkan/vkr.h"
#include "math/types.h"

PIM_C_BEGIN

bool vkrMeshSys_Init(void);
void vkrMeshSys_Update(void);
void vkrMeshSys_Shutdown(void);

bool vkrMesh_Exists(vkrMeshId id);
vkrMeshId vkrMesh_New(
    i32 vertCount,
    const float4* pim_noalias positions,
    const float4* pim_noalias normals,
    const float4* pim_noalias uv01,
    const int4* pim_noalias texIndices);
bool vkrMesh_Del(vkrMeshId id);

bool vkrMesh_Upload(
    vkrMeshId id,
    i32 vertCount,
    const float4* pim_noalias positions,
    const float4* pim_noalias normals,
    const float4* pim_noalias uv01,
    const int4* pim_noalias texIndices);

PIM_C_END
