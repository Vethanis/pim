#include "rendering/vulkan/vkr_desc.h"
#include "allocator/allocator.h"
#include "common/profiler.h"
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

ProfileMark(pm_descpool_reset, vkrDescPool_Reset)
void vkrDescPool_Reset(VkDescriptorPool pool)
{
    ProfileBegin(pm_descpool_reset);
    ASSERT(pool);
    VkCheck(vkResetDescriptorPool(g_vkr.dev, pool, 0x0));
    ProfileEnd(pm_descpool_reset);
}

ProfileMark(pm_desc_new, vkrDesc_New)
VkDescriptorSet vkrDesc_New(vkrFrameContext* ctx, VkDescriptorSetLayout layout)
{
    ProfileBegin(pm_desc_new);
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
    ProfileEnd(pm_desc_new);
    return handle;
}

ProfileMark(pm_desc_writebuffer, vkrDesc_WriteBuffer)
void vkrDesc_WriteBuffer(
    vkrFrameContext* ctx,
    const vkrBufferBinding* binding)
{
    ProfileBegin(pm_desc_writebuffer);
    ASSERT(ctx);
    ASSERT(ctx->descpool);
    ASSERT(binding);
    ASSERT(binding->set);
    ASSERT(binding->buffer.handle);

    const VkDescriptorBufferInfo bufferInfo =
    {
        .buffer = binding->buffer.handle,
        .offset = 0,
        .range = binding->buffer.size,
    };
    const VkWriteDescriptorSet writeInfo =
    {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = binding->set,
        .dstBinding = binding->binding,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = binding->type,
        .pBufferInfo = &bufferInfo,
    };
    vkUpdateDescriptorSets(g_vkr.dev, 1, &writeInfo, 0, NULL);
    ProfileEnd(pm_desc_writebuffer);
}

ProfileMark(pm_desc_writebuffers, vkrDesc_WriteBuffers)
void vkrDesc_WriteBuffers(
    vkrFrameContext* ctx,
    i32 count,
    const vkrBufferBinding* bindings)
{
    ProfileBegin(pm_desc_writebuffers);
    ASSERT(ctx);
    ASSERT(ctx->descpool);
    ASSERT(count >= 0);

    if (count > 0)
    {
        VkDescriptorBufferInfo* bufferInfos = tmp_calloc(sizeof(bufferInfos[0]) * count);
        VkWriteDescriptorSet* writeInfos = tmp_calloc(sizeof(writeInfos[0]) * count);
        for (i32 i = 0; i < count; ++i)
        {
            ASSERT(bindings[i].buffer.handle);
            ASSERT(bindings[i].set);

            bufferInfos[i].buffer = bindings[i].buffer.handle;
            bufferInfos[i].offset = 0;
            bufferInfos[i].range = bindings[i].buffer.size;

            writeInfos[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeInfos[i].dstSet = bindings[i].set;
            writeInfos[i].dstBinding = bindings[i].binding;
            writeInfos[i].dstArrayElement = 0;
            writeInfos[i].descriptorCount = 1;
            writeInfos[i].descriptorType = bindings[i].type;
            writeInfos[i].pBufferInfo = &bufferInfos[i];
        }
        vkUpdateDescriptorSets(g_vkr.dev, count, writeInfos, 0, NULL);
    }
    ProfileEnd(pm_desc_writebuffers);
}