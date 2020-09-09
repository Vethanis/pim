#include "rendering/vulkan/vkr_mem.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_sync.h"
#include "rendering/vulkan/vkr_device.h"
#include "rendering/vulkan/vkr_context.h"
#include "rendering/vulkan/vkr_buffer.h"
#include "rendering/vulkan/vkr_image.h"
#include "VulkanMemoryAllocator/src/vk_mem_alloc.h"
#include "allocator/allocator.h"
#include "common/profiler.h"
#include "common/time.h"
#include "threading/task.h"
#include <string.h>

bool vkrAllocator_New(vkrAllocator* allocator)
{
    ASSERT(allocator);
    memset(allocator, 0, sizeof(*allocator));

    const VmaVulkanFunctions vulkanFns =
    {
        .vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties,
        .vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties,
        .vkAllocateMemory = vkAllocateMemory,
        .vkFreeMemory = vkFreeMemory,
        .vkMapMemory = vkMapMemory,
        .vkUnmapMemory = vkUnmapMemory,
        .vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges,
        .vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges,
        .vkBindBufferMemory = vkBindBufferMemory,
        .vkBindImageMemory = vkBindImageMemory,
        .vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements,
        .vkGetImageMemoryRequirements = vkGetImageMemoryRequirements,
        .vkCreateBuffer = vkCreateBuffer,
        .vkDestroyBuffer = vkDestroyBuffer,
        .vkCreateImage = vkCreateImage,
        .vkDestroyImage = vkDestroyImage,
        .vkCmdCopyBuffer = vkCmdCopyBuffer,
        .vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2KHR,
        .vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2KHR,
        .vkBindBufferMemory2KHR = vkBindBufferMemory2KHR,
        .vkBindImageMemory2KHR = vkBindImageMemory2KHR,
        .vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2KHR,
    };
    const VmaAllocatorCreateInfo info =
    {
        .vulkanApiVersion = VK_API_VERSION_1_2,
        .flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT,
        .instance = g_vkr.inst,
        .physicalDevice = g_vkr.phdev,
        .device = g_vkr.dev,
        .pAllocationCallbacks = NULL,
        .frameInUseCount = kFramesInFlight - 1,
        .pVulkanFunctions = &vulkanFns,
    };
    VmaAllocator handle = NULL;
    VkCheck(vmaCreateAllocator(&info, &handle));
    ASSERT(handle);

    if (handle)
    {
        allocator->handle = handle;
        mutex_create(&allocator->releasemtx);
    }

    return handle != NULL;
}

void vkrAllocator_Del(vkrAllocator* allocator)
{
    if (allocator)
    {
        if (allocator->handle)
        {
            vkrDevice_WaitIdle();
            {
                const u32 frame = time_framecount() + kFramesInFlight + 1;
                const i32 len = allocator->numreleasable;
                vkrReleasable* releasables = allocator->releasables;
                for (i32 i = 0; i < len; ++i)
                {
                    VkFence fence = releasables[i].fence;
                    if (fence)
                    {
                        vkrFence_Wait(fence);
                    }
                    if (!vkrReleasable_Del(&releasables[i], frame))
                    {
                        ASSERT(false);
                    }
                }
                pim_free(allocator->releasables);
                mutex_destroy(&allocator->releasemtx);
            }
            vmaDestroyAllocator(allocator->handle);
        }
        memset(allocator, 0, sizeof(*allocator));
    }
}

void vkrAllocator_Finalize(vkrAllocator* allocator)
{
    vkrDevice_WaitIdle();
    ASSERT(allocator);
    ASSERT(allocator->handle);
    // command fences must still be alive
    ASSERT(g_vkr.context.threadcount);

    mutex_lock(&allocator->releasemtx);
    const u32 frame = time_framecount();
    i32 len = allocator->numreleasable;
    vkrReleasable* releasables = allocator->releasables;
    for (i32 i = len - 1; i >= 0; --i)
    {
        VkFence fence = releasables[i].fence;
        if (fence)
        {
            vkrFence_Wait(fence);
            if (vkrReleasable_Del(&releasables[i], frame))
            {
                releasables[i] = releasables[len - 1];
                --len;
            }
            else
            {
                ASSERT(false);
            }
        }
    }
    allocator->numreleasable = len;
    mutex_unlock(&allocator->releasemtx);
}

ProfileMark(pm_allocupdate, vkrAllocator_Update)
void vkrAllocator_Update(vkrAllocator* allocator)
{
    ProfileBegin(pm_allocupdate);

    ASSERT(allocator);
    ASSERT(allocator->handle);
    // command fences must still be alive
    ASSERT(g_vkr.context.threadcount);

    const u32 frame = time_framecount();
    vmaSetCurrentFrameIndex(allocator->handle, frame);
    {
        mutex_lock(&allocator->releasemtx);
        i32 len = allocator->numreleasable;
        vkrReleasable* releasables = allocator->releasables;
        for (i32 i = len - 1; i >= 0; --i)
        {
            if (vkrReleasable_Del(&releasables[i], frame))
            {
                releasables[i] = releasables[len - 1];
                --len;
            }
        }
        allocator->numreleasable = len;
        mutex_unlock(&allocator->releasemtx);
    }

    ProfileEnd(pm_allocupdate);
}

ProfileMark(pm_releasableadd, vkrReleasable_Add)
void vkrReleasable_Add(vkrAllocator* allocator, const vkrReleasable* releasable)
{
    ProfileBegin(pm_releasableadd);
    ASSERT(allocator);
    ASSERT(allocator->handle);
    // command fences must still be alive
    ASSERT(g_vkr.context.threadcount);
    ASSERT(releasable);
    mutex_lock(&allocator->releasemtx);
    i32 back = allocator->numreleasable++;
    PermReserve(allocator->releasables, back + 1);
    allocator->releasables[back] = *releasable;
    mutex_unlock(&allocator->releasemtx);
    ProfileEnd(pm_releasableadd);
}

ProfileMark(pm_releasabledel, vkrReleasable_Del)
bool vkrReleasable_Del(vkrReleasable* releasable, u32 frame)
{
    ProfileBegin(pm_releasabledel);
    ASSERT(releasable);
    bool ready = false;
    VkFence fence = releasable->fence;
    if (fence)
    {
        vkrFenceState state = vkrFence_Stat(fence);
        ready = state != vkrFenceState_Unsignalled;
    }
    else
    {
        u32 duration = frame - releasable->frame;
        ready = duration > kFramesInFlight;
    }
    if (ready)
    {
        switch (releasable->type)
        {
        default:
            ASSERT(false);
            break;
        case vkrReleasableType_Buffer:
            vkrBuffer_Del(&releasable->buffer);
            break;
        case vkrReleasableType_Image:
            vkrImage_Del(&releasable->image);
            break;
        case vkrReleasableType_ImageView:
        {
            vkrImageView_Del(releasable->view);
        }
        break;
        case vkrReleasableType_Sampler:
        {
            if (releasable->sampler)
            {
                vkDestroySampler(g_vkr.dev, releasable->sampler, NULL);
            }
        }
        break;
        }
        memset(releasable, 0, sizeof(*releasable));
    }
    ProfileEnd(pm_releasabledel);
    return ready;
}

VkFence vkrMem_Barrier(
    vkrQueueId id,
    VkPipelineStageFlags srcStage,
    VkPipelineStageFlags dstStage,
    const VkMemoryBarrier* glob,
    const VkBufferMemoryBarrier* buffer,
    const VkImageMemoryBarrier* img)
{
    vkrFrameContext* ctx = vkrContext_Get();
    VkCommandBuffer cmd = NULL;
    VkFence fence = NULL;
    VkQueue queue = NULL;
    vkrContext_GetCmd(ctx, id, &cmd, &fence, &queue);
    vkrCmdBegin(cmd);
    {
        vkCmdPipelineBarrier(
            cmd,
            srcStage, dstStage, 0x0,
            glob ? 1 : 0, glob,
            buffer ? 1 : 0, buffer,
            img ? 1 : 0, img);
    }
    vkrCmdEnd(cmd);
    vkrCmdSubmit(queue, cmd, fence, NULL, 0x0, NULL);
    ASSERT(fence);
    return fence;
}

ProfileMark(pm_memmap, vkrMem_Map)
void* vkrMem_Map(VmaAllocation allocation)
{
    ProfileBegin(pm_memmap);
    void* result = NULL;
    ASSERT(allocation);
    VkCheck(vmaMapMemory(g_vkr.allocator.handle, allocation, &result));
    ASSERT(result);
    ProfileEnd(pm_memmap);
    return result;
}

ProfileMark(pm_memunmap, vkrMem_Unmap)
void vkrMem_Unmap(VmaAllocation allocation)
{
    ProfileBegin(pm_memunmap);
    ASSERT(allocation);
    vmaUnmapMemory(g_vkr.allocator.handle, allocation);
    ProfileEnd(pm_memunmap);
}

ProfileMark(pm_memflush, vkrMem_Flush)
void vkrMem_Flush(VmaAllocation allocation)
{
    ProfileBegin(pm_memflush);
    ASSERT(allocation);
    const VkDeviceSize offset = 0;
    const VkDeviceSize size = VK_WHOLE_SIZE;
    VkCheck(vmaFlushAllocation(g_vkr.allocator.handle, allocation, offset, size));
    VkCheck(vmaInvalidateAllocation(g_vkr.allocator.handle, allocation, offset, size));
    ProfileEnd(pm_memflush);
}
