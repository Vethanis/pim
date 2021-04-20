#include "rendering/vulkan/vkr_queue.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_device.h"
#include "allocator/allocator.h"
#include "common/time.h"
#include "threading/task.h"
#include <string.h>

void vkrCreateQueues(vkrSys* vkr)
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

void vkrDestroyQueues(vkrSys* vkr)
{
    if (vkr)
    {
        vkrDevice_WaitIdle();
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
    props = Temp_Calloc(sizeof(props[0]) * count);
    vkGetPhysicalDeviceQueueFamilyProperties(phdev, &count, props);
    *countOut = count;
    return props;
}

static i32 vkrSelectGfxFamily(const VkQueueFamilyProperties* families, u32 famCount)
{
    i32 index = -1;
    u32 cost = 0xffffffff;
    for (u32 i = 0; i < famCount; ++i)
    {
        if (families[i].queueCount == 0)
        {
            continue;
        }
        const u32 flags = families[i].queueFlags;
        if (flags & VK_QUEUE_GRAPHICS_BIT)
        {
            u32 newCost = 0;
            newCost += (flags & VK_QUEUE_COMPUTE_BIT) ? 1 : 0;
            newCost += (flags & VK_QUEUE_TRANSFER_BIT) ? 1 : 0;
            newCost += (flags & VK_QUEUE_SPARSE_BINDING_BIT) ? 1 : 0;
            newCost += (flags & VK_QUEUE_PROTECTED_BIT) ? 1 : 0;
            if (newCost < cost)
            {
                cost = newCost;
                index = (i32)i;
            }
        }
    }
    return index;
}

static i32 vkrSelectCompFamily(const VkQueueFamilyProperties* families, u32 famCount)
{
    i32 index = -1;
    u32 cost = 0xffffffff;
    for (u32 i = 0; i < famCount; ++i)
    {
        if (families[i].queueCount == 0)
        {
            continue;
        }
        const u32 flags = families[i].queueFlags;
        if (flags & VK_QUEUE_COMPUTE_BIT)
        {
            u32 newCost = 0;
            newCost += (flags & VK_QUEUE_GRAPHICS_BIT) ? 1 : 0;
            newCost += (flags & VK_QUEUE_TRANSFER_BIT) ? 1 : 0;
            newCost += (flags & VK_QUEUE_SPARSE_BINDING_BIT) ? 1 : 0;
            newCost += (flags & VK_QUEUE_PROTECTED_BIT) ? 1 : 0;
            if (newCost < cost)
            {
                cost = newCost;
                index = (i32)i;
            }
        }
    }
    return index;
}

static i32 vkrSelectXferFamily(const VkQueueFamilyProperties* families, u32 famCount)
{
    i32 index = -1;
    u32 cost = 0xffffffff;
    for (u32 i = 0; i < famCount; ++i)
    {
        if (families[i].queueCount == 0)
        {
            continue;
        }
        const u32 flags = families[i].queueFlags;
        if (flags & VK_QUEUE_TRANSFER_BIT)
        {
            u32 newCost = 0;
            newCost += (flags & VK_QUEUE_GRAPHICS_BIT) ? 1 : 0;
            newCost += (flags & VK_QUEUE_COMPUTE_BIT) ? 1 : 0;
            newCost += (flags & VK_QUEUE_SPARSE_BINDING_BIT) ? 1 : 0;
            newCost += (flags & VK_QUEUE_PROTECTED_BIT) ? 1 : 0;
            if (newCost < cost)
            {
                cost = newCost;
                index = (i32)i;
            }
        }
    }
    return index;
}

static i32 vkrSelectPresFamily(
    VkPhysicalDevice phdev,
    VkSurfaceKHR surf,
    const VkQueueFamilyProperties* families,
    u32 famCount)
{
    i32 index = -1;
    u32 cost = 0xffffffff;
    for (u32 i = 0; i < famCount; ++i)
    {
        if (families[i].queueCount == 0)
        {
            continue;
        }
        VkBool32 presentable = false;
        VkCheck(vkGetPhysicalDeviceSurfaceSupportKHR(phdev, i, surf, &presentable));
        if (presentable)
        {
            const u32 flags = families[i].queueFlags;
            u32 newCost = 0;
            newCost += (flags & VK_QUEUE_GRAPHICS_BIT) ? 1 : 0;
            newCost += (flags & VK_QUEUE_COMPUTE_BIT) ? 1 : 0;
            newCost += (flags & VK_QUEUE_TRANSFER_BIT) ? 1 : 0;
            newCost += (flags & VK_QUEUE_SPARSE_BINDING_BIT) ? 1 : 0;
            newCost += (flags & VK_QUEUE_PROTECTED_BIT) ? 1 : 0;
            if (newCost < cost)
            {
                cost = newCost;
                index = (i32)i;
            }
        }
    }
    return index;
}

vkrQueueSupport vkrQueryQueueSupport(VkPhysicalDevice phdev, VkSurfaceKHR surf)
{
    ASSERT(phdev);
    ASSERT(surf);

    u32 famCount = 0;
    VkQueueFamilyProperties* properties = vkrEnumQueueFamilyProperties(phdev, &famCount);

    vkrQueueSupport support = { 0 };
    support.count = famCount;
    support.properties = properties;
    support.family[vkrQueueId_Graphics] = vkrSelectGfxFamily(properties, famCount);
    support.family[vkrQueueId_Compute] = vkrSelectCompFamily(properties, famCount);
    support.family[vkrQueueId_Transfer] = vkrSelectXferFamily(properties, famCount);
    support.family[vkrQueueId_Present] = vkrSelectPresFamily(phdev, surf, properties, famCount);

    i32* choicects = Temp_Calloc(sizeof(choicects[0]) * famCount);
    for (i32 id = 0; id < vkrQueueId_COUNT; ++id)
    {
        i32 family = support.family[id];
        i32 index = choicects[family]++;
        support.index[id] = index;
    }

    return support;
}
