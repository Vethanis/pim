#include "rendering/vulkan/vkr_buffer.h"
#include "rendering/vulkan/vkr_mem.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_context.h"
#include "VulkanMemoryAllocator/src/vk_mem_alloc.h"
#include "allocator/allocator.h"
#include "common/profiler.h"
#include "common/time.h"
#include <string.h>

ProfileMark(pm_bufnew, vkrBuffer_New)
bool vkrBuffer_New(
    vkrBuffer *const buffer,
    i32 size,
    VkBufferUsageFlags usage,
    vkrMemUsage memUsage)
{
    ProfileBegin(pm_bufnew);
    ASSERT(g_vkr.allocator.handle);
    ASSERT(size >= 0);
    ASSERT(buffer);
    memset(buffer, 0, sizeof(*buffer));
    const VkBufferCreateInfo bufferInfo =
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    VmaPool pool = NULL;
    switch (memUsage)
    {
    default:
        ASSERT(false);
        break;
    case vkrMemUsage_CpuOnly:
        pool = g_vkr.allocator.stagePool;
        break;
    case vkrMemUsage_CpuToGpu:
        if (usage & (VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT))
        {
            pool = g_vkr.allocator.cpuMeshPool;
        }
        else if (usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
        {
            pool = g_vkr.allocator.uavPool;
        }
        break;
    case vkrMemUsage_GpuOnly:
        if (usage & (VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT))
        {
            pool = g_vkr.allocator.gpuMeshPool;
        }
        break;
    }
    ASSERT(pool);
    const VmaAllocationCreateInfo allocInfo =
    {
        .flags = VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT,
        .usage = memUsage,
        .pool = pool,
    };
    VkBuffer handle = NULL;
    VmaAllocation allocation = NULL;
    VkCheck(vmaCreateBuffer(
        g_vkr.allocator.handle,
        &bufferInfo,
        &allocInfo,
        &handle,
        &allocation,
        NULL));
    ASSERT(handle);
    ProfileEnd(pm_bufnew);
    if (handle)
    {
        buffer->handle = handle;
        buffer->allocation = allocation;
        buffer->size = size;
    }
    return handle != NULL;
}

ProfileMark(pm_bufdel, vkrBuffer_Del)
void vkrBuffer_Del(vkrBuffer *const buffer)
{
    ProfileBegin(pm_bufdel);
    ASSERT(g_vkr.allocator.handle);
    if (buffer)
    {
        if (buffer->handle)
        {
            vmaDestroyBuffer(
                g_vkr.allocator.handle,
                buffer->handle,
                buffer->allocation);
        }
        memset(buffer, 0, sizeof(*buffer));
    }
    ProfileEnd(pm_bufdel);
}

ProfileMark(pm_bufreserve, vkrBuffer_Reserve)
bool vkrBuffer_Reserve(
    vkrBuffer *const buffer,
    i32 size,
    VkBufferUsageFlags bufferUsage,
    vkrMemUsage memUsage)
{
    bool success = true;
    i32 oldsize = buffer->size;
    ASSERT(buffer);
    ASSERT(size >= 0);
    ASSERT(oldsize >= 0);
    if (oldsize < size)
    {
        ProfileBegin(pm_bufreserve);
        vkrBuffer_Release(buffer);
        size = (size > oldsize * 2) ? size : oldsize * 2;
        success = vkrBuffer_New(buffer, size, bufferUsage, memUsage);
        ProfileEnd(pm_bufreserve);
    }
    return success;
}

void *const vkrBuffer_Map(vkrBuffer const *const buffer)
{
    ASSERT(buffer);
    return vkrMem_Map(buffer->allocation);
}

void vkrBuffer_Unmap(vkrBuffer const *const buffer)
{
    ASSERT(buffer);
    vkrMem_Unmap(buffer->allocation);
}

void vkrBuffer_Flush(vkrBuffer const *const buffer)
{
    ASSERT(buffer);
    vkrMem_Flush(buffer->allocation);
}

ProfileMark(pm_bufferwrite, vkrBuffer_Write)
void vkrBuffer_Write(
    vkrBuffer const *const buffer,
    void const *const src,
    i32 size)
{
    ProfileBegin(pm_bufferwrite);
    ASSERT(buffer);
    ASSERT(src);
    ASSERT(size <= buffer->size);
    ASSERT(size >= 0);
    void* dst = vkrBuffer_Map(buffer);
    if (dst)
    {
        memcpy(dst, src, size);
    }
    vkrBuffer_Unmap(buffer);
    vkrBuffer_Flush(buffer);
    ProfileEnd(pm_bufferwrite);
}

ProfileMark(pm_bufrelease, vkrBuffer_Release)
void vkrBuffer_Release(vkrBuffer *const buffer)
{
    ProfileBegin(pm_bufrelease);
    ASSERT(buffer);
    if (buffer->handle)
    {
        const vkrReleasable releasable =
        {
            .frame = vkr_frameIndex(),
            .type = vkrReleasableType_Buffer,
            .buffer = *buffer,
        };
        vkrReleasable_Add(&g_vkr.allocator, &releasable);
    }
    memset(buffer, 0, sizeof(*buffer));
    ProfileEnd(pm_bufrelease);
}

bool vkrBufferSet_New(
    vkrBufferSet *const set,
    i32 size,
    VkBufferUsageFlags usage,
    vkrMemUsage memUsage)
{
    memset(set, 0, sizeof(*set));
    for (i32 i = 0; i < kFramesInFlight; ++i)
    {
        if (!vkrBuffer_New(&set->frames[i], size, usage, memUsage))
        {
            vkrBufferSet_Del(set);
            return false;
        }
    }
    return true;
}

void vkrBufferSet_Del(vkrBufferSet *const set)
{
    if (set)
    {
        for (i32 i = 0; i < kFramesInFlight; ++i)
        {
            vkrBuffer_Del(&set->frames[i]);
        }
        memset(set, 0, sizeof(*set));
    }
}

void vkrBufferSet_Release(vkrBufferSet *const set)
{
    if (set)
    {
        for (i32 i = 0; i < kFramesInFlight; ++i)
        {
            vkrBuffer_Release(&set->frames[i]);
        }
        memset(set, 0, sizeof(*set));
    }
}

vkrBuffer *const vkrBufferSet_Current(vkrBufferSet *const set)
{
    u32 syncIndex = vkr_syncIndex();
    return &set->frames[syncIndex];
}

vkrBuffer *const vkrBufferSet_Prev(vkrBufferSet *const set)
{
    u32 prevIndex = (vkr_syncIndex() + (kFramesInFlight - 1u)) % kFramesInFlight;
    return &set->frames[prevIndex];
}

void vkrBuffer_Barrier(
    vkrBuffer *const buffer,
    VkCommandBuffer cmd,
    VkAccessFlags srcAccessMask,
    VkAccessFlags dstAccessMask,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask)
{
    const VkBufferMemoryBarrier barrier =
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .srcAccessMask = srcAccessMask,
        .dstAccessMask = dstAccessMask,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer = buffer->handle,
        .size = VK_WHOLE_SIZE,
    };
    vkrCmdBufferBarrier(cmd, srcStageMask, dstStageMask, &barrier);
}

void vkrBuffer_Transfer(
    vkrBuffer *const buffer,
    vkrQueueId srcQueueId,
    vkrQueueId dstQueueId,
    VkCommandBuffer srcCmd,
    VkCommandBuffer dstCmd,
    VkAccessFlags srcAccessMask,
    VkAccessFlags dstAccessMask,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask)
{
    u32 srcFamily = g_vkr.queues[srcQueueId].family;
    u32 dstFamily = g_vkr.queues[dstQueueId].family;
    const VkBufferMemoryBarrier barrier =
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .srcAccessMask = srcAccessMask,
        .dstAccessMask = dstAccessMask,
        .srcQueueFamilyIndex = srcFamily,
        .dstQueueFamilyIndex = dstFamily,
        .buffer = buffer->handle,
        .size = VK_WHOLE_SIZE,
    };
    vkrCmdBufferBarrier(srcCmd, srcStageMask, dstStageMask, &barrier);
    vkrCmdBufferBarrier(dstCmd, srcStageMask, dstStageMask, &barrier);
}
