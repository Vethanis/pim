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

ProfileMark(pm_desc_writebindings, vkrDesc_WriteBindings)
void vkrDesc_WriteBindings(
    vkrFrameContext* ctx,
    i32 count,
    const vkrBinding* bindings)
{
    ProfileBegin(pm_desc_writebindings);
    ASSERT(ctx);
    ASSERT(ctx->descpool);
    ASSERT(count >= 0);

    if (count > 0)
    {
        VkDescriptorBufferInfo* bufferInfos = tmp_calloc(sizeof(bufferInfos[0]) * count);
        VkDescriptorImageInfo* imageInfos = tmp_calloc(sizeof(imageInfos[0]) * count);
        VkWriteDescriptorSet* writeInfos = tmp_calloc(sizeof(writeInfos[0]) * count);
        for (i32 i = 0; i < count; ++i)
        {
            ASSERT(bindings[i].set);

            writeInfos[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeInfos[i].descriptorType = bindings[i].type;
            writeInfos[i].dstSet = bindings[i].set;
            writeInfos[i].dstBinding = bindings[i].binding;
            writeInfos[i].dstArrayElement = bindings[i].arrayElem;
            writeInfos[i].descriptorCount = 1;

            switch (bindings[i].type)
            {
            default:
                ASSERT(false);
                break;
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            {
                bufferInfos[i].buffer = bindings[i].buffer;
                bufferInfos[i].offset = 0;
                bufferInfos[i].range = VK_WHOLE_SIZE;
                writeInfos[i].pBufferInfo = &bufferInfos[i];
                ASSERT(bufferInfos[i].buffer);
                ASSERT(bufferInfos[i].range);
            }
            break;
            case VK_DESCRIPTOR_TYPE_SAMPLER:
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            {
                imageInfos[i].sampler = bindings[i].texture.sampler;
                imageInfos[i].imageView = bindings[i].texture.view;
                imageInfos[i].imageLayout = bindings[i].texture.layout;
                writeInfos[i].pImageInfo = &imageInfos[i];
                ASSERT(imageInfos[i].sampler);
                ASSERT(imageInfos[i].imageView);
                ASSERT(imageInfos[i].imageLayout);
            }
            break;
            }
        }
        vkUpdateDescriptorSets(g_vkr.dev, count, writeInfos, 0, NULL);
    }
    ProfileEnd(pm_desc_writebindings);
}