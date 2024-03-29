#include "rendering/vulkan/vkr_sync.h"
#include "allocator/allocator.h"
#include "common/profiler.h"
#include <string.h>

ProfileMark(pm_createsema, vkrSemaphore_New)
VkSemaphore vkrSemaphore_New(void)
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

ProfileMark(pm_destroysema, vkrSemaphore_Del)
void vkrSemaphore_Del(VkSemaphore sema)
{
    ASSERT(g_vkr.dev);
    if (sema)
    {
        ProfileBegin(pm_destroysema);
        vkDestroySemaphore(g_vkr.dev, sema, NULL);
        ProfileEnd(pm_destroysema);
    }
}

ProfileMark(pm_createfence, vkrFence_New)
VkFence vkrFence_New(bool signalled)
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

ProfileMark(pm_destroyfence, vkrFence_Del)
void vkrFence_Del(VkFence fence)
{
    ASSERT(g_vkr.dev);
    if (fence)
    {
        ProfileBegin(pm_destroyfence);
        vkDestroyFence(g_vkr.dev, fence, NULL);
        ProfileEnd(pm_destroyfence);
    }
}

void vkrFence_Reset(VkFence fence)
{
    ASSERT(g_vkr.dev);
    ASSERT(fence);
    VkCheck(vkResetFences(g_vkr.dev, 1, &fence));
}

ProfileMark(pm_waitfence, vkrFence_Wait)
void vkrFence_Wait(VkFence fence)
{
    ProfileBegin(pm_waitfence);
    ASSERT(g_vkr.dev);
    ASSERT(fence);
    const u64 timeout = -1;
    while (vkrFence_Stat(fence) != vkrFenceState_Signalled)
    {
        VkCheck(vkWaitForFences(g_vkr.dev, 1, &fence, false, timeout));
    }
    ProfileEnd(pm_waitfence);
}

vkrFenceState vkrFence_Stat(VkFence fence)
{
    ASSERT(g_vkr.dev);
    ASSERT(fence);
    VkResult state = vkGetFenceStatus(g_vkr.dev, fence);
    ASSERT(state != vkrFenceState_Lost);
    return (vkrFenceState)state;
}
