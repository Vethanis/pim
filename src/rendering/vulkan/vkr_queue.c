#include "rendering/vulkan/vkr_queue.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_device.h"
#include "allocator/allocator.h"
#include "common/time.h"
#include "threading/task.h"
#include <string.h>

// ----------------------------------------------------------------------------

static const VkPipelineStageFlags kPresentStages =
    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT |
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT |
    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
static const VkAccessFlags kPresentAccess =
    VK_ACCESS_MEMORY_READ_BIT |
    VK_ACCESS_MEMORY_WRITE_BIT;

static const VkPipelineStageFlags kGraphicsStages =
    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT |
    VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT |
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT |
    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT |
    VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT |
    VK_PIPELINE_STAGE_VERTEX_INPUT_BIT |
    VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
    VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT |
    VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT |
    VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT |
    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
    VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT |
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
    VK_PIPELINE_STAGE_CONDITIONAL_RENDERING_BIT_EXT |
    VK_PIPELINE_STAGE_TASK_SHADER_BIT_NV |
    VK_PIPELINE_STAGE_MESH_SHADER_BIT_NV;
static const VkAccessFlags kGraphicsAccess =
    VK_ACCESS_MEMORY_READ_BIT |
    VK_ACCESS_MEMORY_WRITE_BIT |
    VK_ACCESS_INDIRECT_COMMAND_READ_BIT |
    VK_ACCESS_INDEX_READ_BIT |
    VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
    VK_ACCESS_UNIFORM_READ_BIT |
    VK_ACCESS_INPUT_ATTACHMENT_READ_BIT |
    VK_ACCESS_SHADER_READ_BIT |
    VK_ACCESS_SHADER_WRITE_BIT |
    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
    VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT |
    VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR |
    VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;

static const VkPipelineStageFlags kComputeStages =
    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT |
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT |
    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT |
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
    VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR |
    VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
static const VkAccessFlags kComputeAccess =
    VK_ACCESS_MEMORY_READ_BIT |
    VK_ACCESS_MEMORY_WRITE_BIT |
    VK_ACCESS_INDIRECT_COMMAND_READ_BIT |
    VK_ACCESS_UNIFORM_READ_BIT |
    VK_ACCESS_SHADER_READ_BIT |
    VK_ACCESS_SHADER_WRITE_BIT |
    VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR |
    VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;

static const VkPipelineStageFlags kTransferStages =
    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT |
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT |
    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT |
    VK_PIPELINE_STAGE_TRANSFER_BIT |
    VK_PIPELINE_STAGE_HOST_BIT;
static const VkAccessFlags kTransferAccess =
    VK_ACCESS_MEMORY_READ_BIT |
    VK_ACCESS_MEMORY_WRITE_BIT |
    VK_ACCESS_HOST_READ_BIT |
    VK_ACCESS_HOST_WRITE_BIT |
    VK_ACCESS_TRANSFER_READ_BIT |
    VK_ACCESS_TRANSFER_WRITE_BIT;

// ----------------------------------------------------------------------------

void vkrCreateQueues(void)
{
    ASSERT(g_vkr.phdev);
    ASSERT(g_vkr.window.surface);

    vkrQueueSupport support = vkrQueryQueueSupport(g_vkr.phdev, g_vkr.window.surface);
    for (i32 id = 0; id < vkrQueueId_COUNT; ++id)
    {
        vkrQueue_New(&g_vkr.queues[id], &support, id);
    }
}

void vkrDestroyQueues(void)
{
    vkrDevice_WaitIdle();
    for (i32 id = 0; id < vkrQueueId_COUNT; ++id)
    {
        vkrQueue_Del(&g_vkr.queues[id]);
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
    ASSERT(g_vkr.dev);
    vkGetDeviceQueue(g_vkr.dev, family, index, &handle);
    ASSERT(handle);

    if (handle)
    {
        queue->family = family;
        queue->index = index;
        queue->handle = handle;

        i32 presFamily = support->family[vkrQueueId_Present];
        VkQueueFlags queueFlags = support->properties->queueFlags;

        if (queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            queue->gfx = 1;
            queue->stageMask |= kGraphicsStages;
            queue->accessMask |= kGraphicsAccess;
        }
        if (queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            queue->comp = 1;
            queue->stageMask |= kComputeStages;
            queue->accessMask |= kComputeAccess;
        }
        if (queueFlags & VK_QUEUE_TRANSFER_BIT)
        {
            queue->xfer = 1;
            queue->stageMask |= kTransferStages;
            queue->accessMask |= kTransferAccess;
        }
        if (family == presFamily)
        {
            queue->pres = 1;
            queue->stageMask |= kPresentStages;
            queue->accessMask |= kPresentAccess;
        }
        ASSERT(queue->stageMask != 0);
        ASSERT(queue->accessMask != 0);

        vkrCmdAlloc_New(queue, id);
    }

    return handle != NULL;
}

void vkrQueue_Del(vkrQueue* queue)
{
    if (queue)
    {
        vkrCmdAlloc_Del(queue);
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
    u32 score = 0;
    for (u32 i = 0; i < famCount; ++i)
    {
        if (families[i].queueCount == 0)
        {
            continue;
        }
        const u32 flags = families[i].queueFlags;
        if (flags & VK_QUEUE_GRAPHICS_BIT)
        {
            u32 newScore = 0;
            newScore += (flags & VK_QUEUE_COMPUTE_BIT) ? 1 : 0;
            newScore += (flags & VK_QUEUE_TRANSFER_BIT) ? 1 : 0;
            if (newScore > score)
            {
                score = newScore;
                index = (i32)i;
            }
        }
    }
    return index;
}

static i32 vkrSelectCompFamily(const VkQueueFamilyProperties* families, u32 famCount)
{
    i32 index = -1;
    u32 score = 0;
    for (u32 i = 0; i < famCount; ++i)
    {
        if (families[i].queueCount == 0)
        {
            continue;
        }
        const u32 flags = families[i].queueFlags;
        if (flags & VK_QUEUE_COMPUTE_BIT)
        {
            u32 newScore = 0;
            newScore += (flags & VK_QUEUE_GRAPHICS_BIT) ? 1 : 0;
            newScore += (flags & VK_QUEUE_TRANSFER_BIT) ? 1 : 0;
            if (newScore > score)
            {
                score = newScore;
                index = (i32)i;
            }
        }
    }
    return index;
}

static i32 vkrSelectXferFamily(const VkQueueFamilyProperties* families, u32 famCount)
{
    i32 index = -1;
    u32 score = 0;
    for (u32 i = 0; i < famCount; ++i)
    {
        if (families[i].queueCount == 0)
        {
            continue;
        }
        const u32 flags = families[i].queueFlags;
        if (flags & VK_QUEUE_TRANSFER_BIT)
        {
            u32 newScore = 0;
            newScore += (flags & VK_QUEUE_GRAPHICS_BIT) ? 1 : 0;
            newScore += (flags & VK_QUEUE_COMPUTE_BIT) ? 1 : 0;
            if (newScore > score)
            {
                score = newScore;
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
    u32 score = 0;
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
            u32 newScore = 0;
            newScore += (flags & VK_QUEUE_GRAPHICS_BIT) ? 1 : 0;
            newScore += (flags & VK_QUEUE_COMPUTE_BIT) ? 1 : 0;
            newScore += (flags & VK_QUEUE_TRANSFER_BIT) ? 1 : 0;
            if (newScore > score)
            {
                score = newScore;
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
