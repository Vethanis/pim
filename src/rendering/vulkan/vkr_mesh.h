#pragma once

#include "rendering/vulkan/vkr.h"
#include "math/types.h"

PIM_C_BEGIN

bool vkrMeshSys_Init(void);
void vkrMeshSys_Update(void);
void vkrMeshSys_Shutdown(void);

bool vkrMesh_Exists(vkrMeshId id);
vkrMeshId vkrMesh_New(const vkrMeshDesc* desc);
bool vkrMesh_Del(vkrMeshId id);

bool vkrMesh_Upload(
    vkrMeshId id,
    const vkrMeshDesc* desc);

bool vkrMeshDesc_Alloc(vkrMeshDesc* desc);
void vkrMeshDesc_Free(vkrMeshDesc* desc);

void vkrMeshDesc_SetIndex(vkrMeshDesc* desc, u32 i, u32 value);
void VEC_CALL vkrMeshDesc_SetPosition(vkrMeshDesc* desc, u32 i, float4 value);
void VEC_CALL vkrMeshDesc_SetBasis(vkrMeshDesc* desc, u32 i, float4 normal, float4 tangent);
void vkrMeshDesc_Set(vkrMeshDesc* desc, vkrMeshStream s, u32 i, const void* value);

PIM_C_END
