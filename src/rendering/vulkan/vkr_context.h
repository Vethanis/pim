#pragma once

#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

vkrThreadContext* vkrContext_Get(void);

bool vkrContext_New(vkrContext* ctx);
void vkrContext_Del(vkrContext* ctx);

bool vkrThreadContext_New(vkrThreadContext* ctx);
void vkrThreadContext_Del(vkrThreadContext* ctx);

void vkrContext_OnSwapRecreate(vkrContext* ctx);
void vkrThreadContext_OnSwapRecreate(vkrThreadContext* ctx);

VkCommandBuffer vkrContext_GetTmpCmd(
    vkrThreadContext* ctx,
    vkrQueueId id,
    VkFence* fenceOut,
    VkQueue* queueOut);
VkCommandBuffer vkrContext_GetSecCmd(
    vkrThreadContext* ctx,
    vkrQueueId id,
    VkFence primaryFence);

PIM_C_END
