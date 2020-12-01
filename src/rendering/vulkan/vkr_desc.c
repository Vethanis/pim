#include "rendering/vulkan/vkr_desc.h"
#include "allocator/allocator.h"
#include "common/profiler.h"
#include <string.h>

VkDescriptorSetLayout vkrDescSetLayout_New(
    i32 bindingCount,
    const VkDescriptorSetLayoutBinding* pBindings,
    VkDescriptorSetLayoutCreateFlags flags)
{
    ASSERT(pBindings || !bindingCount);
    ASSERT(bindingCount >= 0);
    const VkDescriptorSetLayoutCreateInfo createInfo =
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .flags = flags,
        .bindingCount = bindingCount,
        .pBindings = pBindings,
    };
    VkDescriptorSetLayout handle = NULL;
    VkCheck(vkCreateDescriptorSetLayout(g_vkr.dev, &createInfo, NULL, &handle));
    ASSERT(handle);
    return handle;
}

void vkrDescSetLayout_Del(VkDescriptorSetLayout handle)
{
    if (handle)
    {
        vkDestroyDescriptorSetLayout(g_vkr.dev, handle, NULL);
    }
}

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
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
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

VkDescriptorSet vkrDescSet_New(VkDescriptorPool pool, VkDescriptorSetLayout layout)
{
    ASSERT(pool);
    ASSERT(layout);
    const VkDescriptorSetAllocateInfo info =
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout,
    };
    VkDescriptorSet handle = NULL;
    VkCheck(vkAllocateDescriptorSets(g_vkr.dev, &info, &handle));
    ASSERT(handle);
    return handle;
}

void vkrDescSet_Del(VkDescriptorPool pool, VkDescriptorSet set)
{
    ASSERT(pool);
    if (set)
    {
        VkCheck(vkFreeDescriptorSets(g_vkr.dev, pool, 1, &set));
    }
}

ProfileMark(pm_writeImageTable, vkrDesc_WriteImageTable)
void vkrDesc_WriteImageTable(
    VkDescriptorSet set, i32 binding,
    VkDescriptorType type,
    i32 count, const VkDescriptorImageInfo* bindings)
{
    ASSERT(set);
    ASSERT(type >= VK_DESCRIPTOR_TYPE_SAMPLER);
    ASSERT(type <= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    if (count > 0)
    {
        ProfileBegin(pm_writeImageTable);
        ASSERT(bindings);
        const VkWriteDescriptorSet writeInfo =
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .descriptorType = type,
            .dstSet = set,
            .dstBinding = binding,
            .dstArrayElement = 0,
            .descriptorCount = count,
            .pImageInfo = bindings,
        };
        vkUpdateDescriptorSets(g_vkr.dev, 1, &writeInfo, 0, NULL);
        ProfileEnd(pm_writeImageTable);
    }
}

ProfileMark(pm_writeBufferTable, vkrDesc_WriteBufferTable)
void vkrDesc_WriteBufferTable(
    VkDescriptorSet set, i32 binding,
    VkDescriptorType type,
    i32 count, const VkDescriptorBufferInfo* bindings)
{
    ASSERT(set);
    ASSERT(type >= VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER);
    ASSERT(type <= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC);
    if (count > 0)
    {
        ProfileBegin(pm_writeBufferTable);
        ASSERT(bindings);
        const VkWriteDescriptorSet writeInfo =
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .descriptorType = type,
            .dstSet = set,
            .dstBinding = binding,
            .dstArrayElement = 0,
            .descriptorCount = count,
            .pBufferInfo = bindings,
        };
        vkUpdateDescriptorSets(g_vkr.dev, 1, &writeInfo, 0, NULL);
        ProfileEnd(pm_writeBufferTable);
    }
}
