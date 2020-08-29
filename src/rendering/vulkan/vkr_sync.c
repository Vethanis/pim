#include "rendering/vulkan/vkr_sync.h"
#include "allocator/allocator.h"
#include "common/profiler.h"
#include <string.h>

ProfileMark(pm_createsema, vkrCreateSemaphore)
VkSemaphore vkrCreateSemaphore(void)
{
    ProfileBegin(pm_createsema);
    ASSERT(g_vkr.dev);
    const VkSemaphoreCreateInfo info =
    {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    VkSemaphore handle = NULL;
    VkCheck(vkCreateSemaphore(g_vkr.dev, &info, NULL, &handle));
    ASSERT(handle);
    ProfileEnd(pm_createsema);
    return handle;
}

ProfileMark(pm_destroysema, vkrDestroySemaphore)
void vkrDestroySemaphore(VkSemaphore sema)
{
    ASSERT(g_vkr.dev);
    if (sema)
    {
        ProfileBegin(pm_destroysema);
        vkDestroySemaphore(g_vkr.dev, sema, NULL);
        ProfileEnd(pm_destroysema);
    }
}

ProfileMark(pm_createfence, vkrCreateFence)
VkFence vkrCreateFence(bool signalled)
{
    ProfileBegin(pm_createfence);
    ASSERT(g_vkr.dev);
    const VkFenceCreateInfo info =
    {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = signalled ? VK_FENCE_CREATE_SIGNALED_BIT : 0,
    };
    VkFence handle = NULL;
    VkCheck(vkCreateFence(g_vkr.dev, &info, NULL, &handle));
    ASSERT(handle);
    ProfileEnd(pm_createfence);
    return handle;
}

ProfileMark(pm_destroyfence, vkrDestroyFence)
void vkrDestroyFence(VkFence fence)
{
    ASSERT(g_vkr.dev);
    if (fence)
    {
        ProfileBegin(pm_destroyfence);
        vkDestroyFence(g_vkr.dev, fence, NULL);
        ProfileEnd(pm_destroyfence);
    }
}

ProfileMark(pm_resetfence, vkrResetFence)
void vkrResetFence(VkFence fence)
{
    ProfileBegin(pm_resetfence);
    ASSERT(g_vkr.dev);
    ASSERT(fence);
    VkCheck(vkResetFences(g_vkr.dev, 1, &fence));
    ProfileEnd(pm_resetfence);
}

ProfileMark(pm_waitfence, vkrWaitFence)
void vkrWaitFence(VkFence fence)
{
    ProfileBegin(pm_waitfence);
    ASSERT(g_vkr.dev);
    ASSERT(fence);
    const u64 timeout = -1;
    VkCheck(vkWaitForFences(g_vkr.dev, 1, &fence, true, timeout));
    ProfileEnd(pm_waitfence);
}
