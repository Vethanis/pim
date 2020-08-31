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
