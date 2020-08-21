#include "rendering/vulkan/vkr_queue.h"
#include "allocator/allocator.h"
#include "threading/task.h"
#include <string.h>

VkQueueFamilyProperties* vkrEnumQueueFamilyProperties(
    VkPhysicalDevice phdev,
    u32* countOut)
{
    ASSERT(phdev);
    ASSERT(countOut);
    u32 count = 0;
    VkQueueFamilyProperties* props = NULL;
    vkGetPhysicalDeviceQueueFamilyProperties(phdev, &count, NULL);
    props = tmp_calloc(sizeof(props[0]) * count);
    vkGetPhysicalDeviceQueueFamilyProperties(phdev, &count, props);
    *countOut = count;
    return props;
}

static void vkrSelectFamily(
    vkrQueueSupport* support,
    vkrQueueId id,
    i32 family,
    i32* choicects,
    const VkQueueFamilyProperties* props)
{
    i32 prev = support->family[id];
    if (prev < 0)
    {
        support->family[id] = family;
        choicects[family] += 1;
    }
    else
    {
        if (choicects[family] < choicects[prev])
        {
            support->family[id] = family;
            choicects[family] += 1;
            choicects[prev] -= 1;
        }
    }
}

vkrQueueSupport vkrQueryQueueSupport(VkPhysicalDevice phdev, VkSurfaceKHR surf)
{
    ASSERT(phdev);
    ASSERT(surf);

    vkrQueueSupport support = { 0 };
    for (i32 id = 0; id < vkrQueueId_COUNT; ++id)
    {
        support.family[id] = -1;
        support.index[id] = 0;
    }

    u32 famCount = 0;
    VkQueueFamilyProperties* properties = vkrEnumQueueFamilyProperties(phdev, &famCount);
    support.count = famCount;
    support.properties = properties;

    i32* choicects = tmp_calloc(sizeof(choicects[0]) * famCount);
    for (u32 family = 0; family < famCount; ++family)
    {
        if (properties[family].queueCount == 0)
        {
            continue;
        }
        VkQueueFlags flags = properties[family].queueFlags;

        if (flags & VK_QUEUE_GRAPHICS_BIT)
        {
            vkrSelectFamily(&support, vkrQueueId_Gfx, family, choicects, properties);
        }
        if (flags & VK_QUEUE_COMPUTE_BIT)
        {
            vkrSelectFamily(&support, vkrQueueId_Comp, family, choicects, properties);
        }
        if (flags & VK_QUEUE_TRANSFER_BIT)
        {
            vkrSelectFamily(&support, vkrQueueId_Xfer, family, choicects, properties);
        }
        VkBool32 presentable = false;
        VkCheck(vkGetPhysicalDeviceSurfaceSupportKHR(phdev, family, surf, &presentable));
        if (presentable)
        {
            vkrSelectFamily(&support, vkrQueueId_Pres, family, choicects, properties);
        }
    }
    for (u32 family = 0; family < famCount; ++family)
    {
        choicects[family] = 0;
    }
    for (i32 id = 0; id < vkrQueueId_COUNT; ++id)
    {
        i32 family = support.family[id];
        i32 index = choicects[family]++;
        support.index[id] = index;
    }

    return support;
}

static vkrQueue vkrCreateQueue(
    VkDevice device,
    const vkrQueueSupport* support,
    vkrQueueId id)
{
    vkrQueue queue = { 0 };

    i32 family = support->family[id];
    i32 index = support->index[id];
    ASSERT(family >= 0);

    queue.family = family;
    queue.index = index;
    queue.granularity = support->properties[family].minImageTransferGranularity;

    VkQueue handle = NULL;
    vkGetDeviceQueue(device, family, index, &handle);
    ASSERT(handle);
    queue.handle = handle;

    const VkCommandPoolCreateInfo poolInfo =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex = family,
    };

    const i32 threadcount = task_thread_ct();
    queue.poolcount = threadcount;

    for (i32 fr = 0; fr < kMaxSwapchainLength; ++fr)
    {
        VkCommandPool* pools = perm_calloc(sizeof(pools[0]) * threadcount);

        for (i32 tr = 0; tr < threadcount; ++tr)
        {
            VkCommandPool pool = NULL;
            VkCheck(vkCreateCommandPool(device, &poolInfo, NULL, &pool));
            ASSERT(pool);
            pools[tr] = pool;
        }

        queue.pools[fr] = pools;
    }

    return queue;
}

void vkrCreateQueues(vkr_t* vkr)
{
    ASSERT(vkr);
    ASSERT(vkr->dev);
    ASSERT(vkrAlive(vkr->display));

    VkDevice device = vkr->dev;
    vkrQueueSupport support = vkrQueryQueueSupport(vkr->phdev, vkr->display->surface);
    for (i32 id = 0; id < vkrQueueId_COUNT; ++id)
    {
        vkr->queues[id] = vkrCreateQueue(device, &support, id);
    }
}

static void vkrDestroyQueue(VkDevice device, vkrQueue* queue)
{
    ASSERT(device);
    const i32 poolcount = queue->poolcount;
    for (i32 fr = 0; fr < kMaxSwapchainLength; ++fr)
    {
        VkCommandPool* pools = queue->pools[fr];
        queue->pools[fr] = NULL;
        if (pools)
        {
            for (i32 tr = 0; tr < poolcount; ++tr)
            {
                if (pools[tr])
                {
                    vkDestroyCommandPool(device, pools[tr], NULL);
                    pools[tr] = NULL;
                }
            }
            pim_free(pools);
        }
    }
    memset(queue, 0, sizeof(*queue));
}

void vkrDestroyQueues(vkr_t* vkr)
{
    if (vkr)
    {
        VkDevice device = vkr->dev;
        VkCheck(vkDeviceWaitIdle(device));
        for (i32 id = 0; id < vkrQueueId_COUNT; ++id)
        {
            vkrDestroyQueue(device, vkr->queues + id);
        }
    }
}
