#include "rendering/vulkan/vkr_sync.h"
#include "allocator/allocator.h"
#include <string.h>

VkSemaphore vkrCreateSemaphore(void)
{
    ASSERT(g_vkr.dev);
    const VkSemaphoreCreateInfo info =
    {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    VkSemaphore handle = NULL;
    VkCheck(vkCreateSemaphore(g_vkr.dev, &info, NULL, &handle));
    ASSERT(handle);
    return handle;
}

void vkrDestroySemaphore(VkSemaphore sema)
{
    ASSERT(g_vkr.dev);
    if (sema)
    {
        vkDestroySemaphore(g_vkr.dev, sema, NULL);
    }
}

VkFence vkrCreateFence(bool signalled)
{
    ASSERT(g_vkr.dev);
    const VkFenceCreateInfo info =
    {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = signalled ? VK_FENCE_CREATE_SIGNALED_BIT : 0,
    };
    VkFence handle = NULL;
    VkCheck(vkCreateFence(g_vkr.dev, &info, NULL, &handle));
    return handle;
}

void vkrDestroyFence(VkFence fence)
{
    ASSERT(g_vkr.dev);
    if (fence)
    {
        vkDestroyFence(g_vkr.dev, fence, NULL);
    }
}

void vkrResetFence(VkFence fence)
{
    ASSERT(g_vkr.dev);
    ASSERT(fence);
    VkCheck(vkResetFences(g_vkr.dev, 1, &fence));
}

void vkrWaitFence(VkFence fence)
{
    ASSERT(g_vkr.dev);
    ASSERT(fence);
    const u64 timeout = -1;
    VkCheck(vkWaitForFences(g_vkr.dev, 1, &fence, true, timeout));
}
