#include "rendering/vulkan/vkr_queue.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "allocator/allocator.h"
#include "common/time.h"
#include "threading/task.h"
#include <string.h>

void vkrCreateQueues(vkr_t* vkr)
{
    ASSERT(vkr);
    ASSERT(vkr->dev);
    ASSERT(vkr->display.surface);

    vkrQueueSupport support = vkrQueryQueueSupport(vkr->phdev, vkr->display.surface);
    for (i32 id = 0; id < vkrQueueId_COUNT; ++id)
    {
        vkrQueue_New(&vkr->queues[id], &support, id);
    }
}

void vkrDestroyQueues(vkr_t* vkr)
{
    if (vkr)
    {
        VkCheck(vkDeviceWaitIdle(vkr->dev));
        for (i32 id = 0; id < vkrQueueId_COUNT; ++id)
        {
            vkrQueue_Del(&vkr->queues[id]);
        }
    }
}

// ----------------------------------------------------------------------------

bool vkrQueue_New(
    vkrQueue* queue,
    const vkrQueueSupport* support,
    vkrQueueId id)
{
    ASSERT(queue);
    ASSERT(support);
    memset(queue, 0, sizeof(*queue));

    i32 family = support->family[id];
    i32 index = support->index[id];
    ASSERT(family >= 0);
    ASSERT(index >= 0);

    VkQueue handle = NULL;
    vkGetDeviceQueue(g_vkr.dev, family, index, &handle);
    ASSERT(handle);

    if (handle)
    {
        queue->family = family;
        queue->index = index;
        queue->granularity =
            support->properties[family].minImageTransferGranularity;
        queue->handle = handle;
    }

    return handle != NULL;
}

void vkrQueue_Del(vkrQueue* queue)
{
    if (queue)
    {
        memset(queue, 0, sizeof(*queue));
    }
}

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
