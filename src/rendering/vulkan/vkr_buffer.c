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
    vkrBuffer* buffer,
    i32 size,
    VkBufferUsageFlags bufferUsage,
    vkrMemUsage memUsage,
    const char* tag)
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
        .usage = bufferUsage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    const VmaAllocationCreateInfo allocInfo =
    {
        .flags = VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT,
        .usage = memUsage,
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
void vkrBuffer_Del(vkrBuffer* buffer)
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
    vkrBuffer* buffer,
    i32 size,
    VkBufferUsageFlags bufferUsage,
    vkrMemUsage memUsage,
    VkFence fence,
    const char* tag)
{
    bool success = true;
    i32 oldsize = buffer->size;
    ASSERT(buffer);
    ASSERT(size >= 0);
    ASSERT(oldsize >= 0);
    if (oldsize < size)
    {
        ProfileBegin(pm_bufreserve);
        vkrBuffer_Release(buffer, fence);
        size = (size > oldsize * 2) ? size : oldsize * 2;
        success = vkrBuffer_New(buffer, size, bufferUsage, memUsage, tag);
        ProfileEnd(pm_bufreserve);
    }
    return success;
}

void* vkrBuffer_Map(const vkrBuffer* buffer)
{
    ASSERT(buffer);
    return vkrMem_Map(buffer->allocation);
}

void vkrBuffer_Unmap(const vkrBuffer* buffer)
{
    ASSERT(buffer);
    vkrMem_Unmap(buffer->allocation);
}

void vkrBuffer_Flush(const vkrBuffer* buffer)
{
    ASSERT(buffer);
    vkrMem_Flush(buffer->allocation);
}

ProfileMark(pm_bufferwrite, vkrBuffer_Write)
void vkrBuffer_Write(const vkrBuffer* buffer, const void* src, i32 size)
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
void vkrBuffer_Release(vkrBuffer* buffer, VkFence fence)
{
    ProfileBegin(pm_bufrelease);
    ASSERT(buffer);
    if (buffer->handle)
    {
        if (!fence)
        {
            const VkBufferMemoryBarrier barrier =
            {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .srcAccessMask = 0x0,
                .dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .buffer = buffer->handle,
                .offset = 0,
                .size = VK_WHOLE_SIZE,
            };
            fence = vkrMem_Barrier(
                vkrQueueId_Gfx,
                VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                VK_PIPELINE_STAGE_HOST_BIT,
                NULL,
                &barrier,
                NULL);
        }
        const vkrReleasable releasable =
        {
            .frame = time_framecount(),
            .type = vkrReleasableType_Buffer,
            .fence = fence,
            .buffer = *buffer,
        };
        vkrReleasable_Add(&g_vkr.allocator, &releasable);
    }
    memset(buffer, 0, sizeof(*buffer));
    ProfileEnd(pm_bufrelease);
}

void vkrBuffer_Barrier(
    vkrBuffer* buffer,
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
    vkrBuffer* buffer,
    VkAccessFlags srcAccessMask,
    VkAccessFlags dstAccessMask,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask,
    vkrQueueId srcQueueId,
    vkrQueueId dstQueueId)
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
    vkrThreadContext* ctx = vkrContext_Get();
    {
        VkFence fence = NULL;
        VkQueue queue = NULL;
        VkCommandBuffer cmd = vkrContext_GetTmpCmd(ctx, srcQueueId, &fence, &queue);
        vkrCmdBegin(cmd);
        vkrCmdBufferBarrier(cmd, srcStageMask, dstStageMask, &barrier);
        vkrCmdEnd(cmd);
        vkrCmdSubmit(queue, cmd, fence, NULL, 0x0, NULL);
    }
    {
        VkFence fence = NULL;
        VkQueue queue = NULL;
        VkCommandBuffer cmd = vkrContext_GetTmpCmd(ctx, dstQueueId, &fence, &queue);
        vkrCmdBegin(cmd);
        vkrCmdBufferBarrier(cmd, srcStageMask, dstStageMask, &barrier);
        vkrCmdEnd(cmd);
        vkrCmdSubmit(queue, cmd, fence, NULL, 0x0, NULL);
    }
}
