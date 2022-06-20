#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

bool vkrTexTable_Init(void);
void vkrTexTable_Update(void);
void vkrTexTable_Shutdown(void);

void vkrTexTable_Write(VkDescriptorSet set);

bool vkrTexTable_Exists(vkrTextureId id);
vkrTextureId vkrTexTable_Alloc(
    VkImageViewType type,
    VkFormat format,
    VkSamplerAddressMode clamp,
    i32 width,
    i32 height,
    i32 depth,
    i32 layers,
    bool mips);
bool vkrTexTable_Free(vkrTextureId id);
bool vkrTexTable_Upload(
    vkrTextureId id,
    i32 layer,
    void const *const data,
    i32 bytes);

bool vkrTexTable_SetSampler(
    vkrTextureId id,
    VkFilter filter,
    VkSamplerMipmapMode mipMode,
    VkSamplerAddressMode addressMode,
    float aniso);

// returns bindless index
u32 vkrTexTable_State(
    vkrTextureId id,
    vkrCmdBuf* cmd,
    const vkrImageState_t* state);
u32 vkrTexTable_FragSample(
    vkrTextureId id,
    vkrCmdBuf* cmd);
u32 vkrTexTable_ComputeSample(
    vkrTextureId id,
    vkrCmdBuf* cmd);
u32 vkrTexTable_ComputeLoadStore(
    vkrTextureId id,
    vkrCmdBuf* cmd);

void vkrTexTable_StateAll(
    vkrCmdBuf* cmd,
    const vkrImageState_t* state);
void vkrTexTable_FragSampleAll(vkrCmdBuf* cmd);
void vkrTexTable_ComputeSampleAll(vkrCmdBuf* cmd);
void vkrTexTable_ComputeLoadStoreAll(vkrCmdBuf* cmd);

PIM_C_END
