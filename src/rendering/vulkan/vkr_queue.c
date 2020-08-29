#include "rendering/vulkan/vkr_queue.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "allocator/allocator.h"
#include "common/time.h"
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

vkrQueue vkrCreateQueue(
    VkDevice device,
    const vkrQueueSupport* support,
    vkrQueueId id)
{
    ASSERT(device);

    vkrQueue queue = { 0 };

    i32 family = support->family[id];
    i32 index = support->index[id];
    ASSERT(family >= 0);
    ASSERT(index >= 0);

    queue.family = family;
    queue.index = index;
    queue.granularity = support->properties[family].minImageTransferGranularity;

    VkQueue handle = NULL;
    vkGetDeviceQueue(device, family, index, &handle);
    ASSERT(handle);
    queue.handle = handle;

    const i32 threadcount = task_thread_ct();
    queue.threadcount = threadcount;

    for (i32 fr = 0; fr < kFramesInFlight; ++fr)
    {
        VkCommandPool* pools = perm_calloc(sizeof(pools[0]) * threadcount);
        vkrCmdBuf* buffers = perm_calloc(sizeof(buffers[0]) * threadcount);

        for (i32 tr = 0; tr < threadcount; ++tr)
        {
            VkCommandPool pool = vkrCmdPool_New(family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
            pools[tr] = pool;
            buffers[tr] = vkrCmdBuf_New(pool, id);
        }

        queue.pools[fr] = pools;
        queue.buffers[fr] = buffers;
    }

    return queue;
}

void vkrCreateQueues(vkr_t* vkr)
{
    ASSERT(vkr);
    ASSERT(vkr->dev);
    ASSERT(vkr->display.surface);

    VkDevice device = vkr->dev;
    vkrQueueSupport support = vkrQueryQueueSupport(vkr->phdev, vkr->display.surface);
    for (i32 id = 0; id < vkrQueueId_COUNT; ++id)
    {
        vkr->queues[id] = vkrCreateQueue(device, &support, id);
    }
}

void vkrDestroyQueue(VkDevice device, vkrQueue* queue)
{
    ASSERT(device);
    ASSERT(queue);
    const i32 threadcount = queue->threadcount;
    if (threadcount > 0)
    {
        for (i32 fr = 0; fr < kFramesInFlight; ++fr)
        {
            VkCommandPool* pools = queue->pools[fr];
            vkrCmdBuf* buffers = queue->buffers[fr];
            if (buffers)
            {
                for (i32 tr = 0; tr < threadcount; ++tr)
                {
                    vkrCmdBuf_Del(&buffers[tr]);
                }
                pim_free(buffers);
            }
            if (pools)
            {
                for (i32 tr = 0; tr < threadcount; ++tr)
                {
                    vkrCmdPool_Del(pools[tr]);
                }
                pim_free(pools);
            }
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
