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

    vkrQueueInfo info = support->info[id];
    ASSERT(info.family >= 0);
    ASSERT(info.index >= 0);

    VkQueue handle = NULL;
    ASSERT(g_vkr.dev);
    vkGetDeviceQueue(g_vkr.dev, info.family, info.index, &handle);
    ASSERT(handle);

    if (handle)
    {
        queue->family = info.family;
        queue->index = info.index;
        queue->handle = handle;

        if (info.gfx)
        {
            queue->gfx = 1;
            queue->stageMask |= kGraphicsStages;
            queue->accessMask |= kGraphicsAccess;
        }
        if (info.compute)
        {
            queue->comp = 1;
            queue->stageMask |= kComputeStages;
            queue->accessMask |= kComputeAccess;
        }
        if (info.transfer)
        {
            queue->xfer = 1;
            queue->stageMask |= kTransferStages;
            queue->accessMask |= kTransferAccess;
        }
        if (info.present)
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

static i32 vkrSelectGfxFamily(
    VkPhysicalDevice phdev,
    VkSurfaceKHR surf,
    const vkrQueueInfo* infos,
    u32 count)
{
    i32 index = -1;
    u32 score = 0;
    for (u32 i = 0; i < count; ++i)
    {
        if (infos[i].properties.queueCount == 0)
        {
            continue;
        }
        if (infos[i].gfx)
        {
            u32 newScore = 0;
            newScore += infos[i].compute;
            newScore += infos[i].transfer;
            newScore += infos[i].present;
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
    vkrQueueInfo* infos = Temp_Calloc(sizeof(infos[0]) * famCount);
    i32* queueCounts = Temp_Calloc(sizeof(queueCounts[0]) * famCount);
    for (u32 i = 0; i < famCount; ++i)
    {
        VkBool32 presentable = false;
        if (properties[i].queueCount != 0)
        {
            VkCheck(vkGetPhysicalDeviceSurfaceSupportKHR(phdev, i, surf, &presentable));
        }
        u32 flags = properties[i].queueFlags;
        infos[i] = (vkrQueueInfo)
        {
            .properties = properties[i],
            .family = i,
            .index = -1,
            .gfx = (flags & VK_QUEUE_GRAPHICS_BIT) ? 1 : 0,
            .compute = (flags & VK_QUEUE_COMPUTE_BIT) ? 1 : 0,
            .transfer = (flags & VK_QUEUE_TRANSFER_BIT) ? 1 : 0,
            .present = presentable ? 1 : 0,
        };
    }

    i32 famPicks[vkrQueueId_COUNT];
    memset(famPicks, 0xff, sizeof(famPicks));
    famPicks[vkrQueueId_Graphics] = vkrSelectGfxFamily(phdev, surf, infos, famCount);

    vkrQueueSupport support = { 0 };
    for (i32 id = 0; id < vkrQueueId_COUNT; ++id)
    {
        i32 family = famPicks[id];
        ASSERT((u32)family < famCount);
        infos[family].index = queueCounts[family]++;
        support.info[id] = infos[family];
    }

    return support;
}
