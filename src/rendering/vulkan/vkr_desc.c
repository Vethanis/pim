#include "rendering/vulkan/vkr_desc.h"
#include "allocator/allocator.h"
#include <string.h>

VkDescriptorPool vkrDescPool_New(
    i32 maxSets,
    i32 sizeCount,
    const VkDescriptorPoolSize* sizes)
{
    ASSERT(maxSets > 0);
    ASSERT(sizeCount > 0);
    ASSERT(sizes);
    const VkDescriptorPoolCreateInfo poolInfo =
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = maxSets,
        .poolSizeCount = sizeCount,
        .pPoolSizes = sizes,
    };
    VkDescriptorPool handle = NULL;
    VkCheck(vkCreateDescriptorPool(g_vkr.dev, &poolInfo, NULL, &handle));
    ASSERT(handle);
    return handle;
}

void vkrDescPool_Del(VkDescriptorPool pool)
{
    if (pool)
    {
        vkDestroyDescriptorPool(g_vkr.dev, pool, NULL);
    }
}

void vkrDescPool_Reset(VkDescriptorPool pool)
{
    ASSERT(pool);
    VkCheck(vkResetDescriptorPool(g_vkr.dev, pool, 0x0));
}

VkDescriptorSet vkrDesc_New(vkrFrameContext* ctx, VkDescriptorSetLayout layout)
{
    ASSERT(ctx);
    ASSERT(ctx->descpool);
    ASSERT(layout);
    const VkDescriptorSetAllocateInfo info =
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = ctx->descpool,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout,
    };
    VkDescriptorSet handle = NULL;
    VkCheck(vkAllocateDescriptorSets(g_vkr.dev, &info, &handle));
    ASSERT(handle);
    return handle;
}

void vkrDesc_WriteSSBO(
    vkrFrameContext* ctx,
    VkDescriptorSet set,
    i32 binding,
    vkrBuffer buffer)
{
    ASSERT(ctx);
    ASSERT(ctx->descpool);
    ASSERT(set);
    ASSERT(buffer.handle);

    const VkDescriptorBufferInfo bufferInfo =
    {
        .buffer = buffer.handle,
        .offset = 0,
        .range = buffer.size,
    };
    const VkWriteDescriptorSet writeInfo =
    {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = set,
        .dstBinding = binding,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pBufferInfo = &bufferInfo,
    };
    vkUpdateDescriptorSets(g_vkr.dev, 1, &writeInfo, 0, NULL);
}
