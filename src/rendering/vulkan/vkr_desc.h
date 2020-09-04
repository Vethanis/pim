#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

VkDescriptorPool vkrDescPool_New(
    i32 maxSets,
    i32 sizeCount,
    const VkDescriptorPoolSize* sizes);
void vkrDescPool_Del(VkDescriptorPool pool);
void vkrDescPool_Reset(VkDescriptorPool pool);

VkDescriptorSet vkrDesc_New(vkrFrameContext* ctx, VkDescriptorSetLayout layout);

void vkrDesc_WriteBuffer(
    vkrFrameContext* ctx,
    const vkrBufferBinding* binding);

void vkrDesc_WriteBuffers(
    vkrFrameContext* ctx,
    i32 count,
    const vkrBufferBinding* bindings);

PIM_C_END
