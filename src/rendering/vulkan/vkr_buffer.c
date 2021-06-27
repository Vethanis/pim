#include "rendering/vulkan/vkr_buffer.h"
#include "rendering/vulkan/vkr_mem.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_context.h"
#include "VulkanMemoryAllocator/src/vk_mem_alloc.h"
#include "allocator/allocator.h"
#include "common/profiler.h"
#include "common/time.h"
#include <string.h>

bool vkrBuffer_New(
    vkrBuffer *const buffer,
    i32 size,
    VkBufferUsageFlags usage,
    vkrMemUsage memUsage)
{
    return vkrMem_BufferNew(buffer, size, usage, memUsage);
}

ProfileMark(pm_bufrelease, vkrBuffer_Release)
void vkrBuffer_Release(vkrBuffer *const buffer)
{
    if (buffer->handle)
    {
        const vkrReleasable releasable =
        {
            .frame = vkrSys_FrameIndex(),
            .type = vkrReleasableType_Buffer,
            .buffer = *buffer,
        };
        vkrReleasable_Add(&releasable);
    }
    memset(buffer, 0, sizeof(*buffer));
}

ProfileMark(pm_bufreserve, vkrBuffer_Reserve)
bool vkrBuffer_Reserve(
    vkrBuffer *const buffer,
    i32 size,
    VkBufferUsageFlags bufferUsage,
    vkrMemUsage memUsage)
{
    ProfileBegin(pm_bufreserve);
    bool success = true;
    i32 oldsize = buffer->size;
    ASSERT(buffer);
    ASSERT(size >= 0);
    ASSERT(oldsize >= 0);
    if (oldsize < size)
    {
        vkrBuffer_Release(buffer);
        size = (size > oldsize * 2) ? size : oldsize * 2;
        success = vkrBuffer_New(buffer, size, bufferUsage, memUsage);
    }
    ProfileEnd(pm_bufreserve);
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
bool vkrBuffer_Write(
    vkrBuffer const *const buffer,
    void const *const src,
    i32 size)
{
    ProfileBegin(pm_bufferwrite);
    ASSERT(buffer);
    ASSERT(src);
    ASSERT(size >= 0);
    ASSERT(size <= buffer->size);
    bool success = false;
    void* dst = vkrBuffer_Map(buffer);
    if (dst)
    {
        memcpy(dst, src, size);
        success = true;
    }
    vkrBuffer_Unmap(buffer);
    vkrBuffer_Flush(buffer);
    ProfileEnd(pm_bufferwrite);
    return success;
}

ProfileMark(pm_bufferread, vkrBuffer_Read)
bool vkrBuffer_Read(
    vkrBuffer const *const buffer,
    void* dst,
    i32 size)
{
    ProfileBegin(pm_bufferread);
    ASSERT(buffer);
    ASSERT(dst);
    ASSERT(size == buffer->size);
    bool success = false;
    const void* src = vkrBuffer_Map(buffer);
    if (src)
    {
        memcpy(dst, src, size);
        success = true;
    }
    vkrBuffer_Unmap(buffer);
    ProfileEnd(pm_bufferread);
    return success;
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

// ----------------------------------------------------------------------------

bool vkrBufferSet_New(
    vkrBufferSet *const set,
    i32 size,
    VkBufferUsageFlags usage,
    vkrMemUsage memUsage)
{
    memset(set, 0, sizeof(*set));
    for (i32 i = 0; i < NELEM(set->frames); ++i)
    {
        if (!vkrBuffer_New(&set->frames[i], size, usage, memUsage))
        {
            vkrBufferSet_Release(set);
            return false;
        }
    }
    return true;
}

void vkrBufferSet_Release(vkrBufferSet *const set)
{
    for (i32 i = 0; i < NELEM(set->frames); ++i)
    {
        vkrBuffer_Release(&set->frames[i]);
    }
    memset(set, 0, sizeof(*set));
}

vkrBuffer *const vkrBufferSet_Current(vkrBufferSet *const set)
{
    u32 syncIndex = vkrSys_SyncIndex();
    ASSERT(syncIndex < NELEM(set->frames));
    return &set->frames[syncIndex];
}

bool vkrBufferSet_Reserve(
    vkrBufferSet *const set,
    i32 size,
    VkBufferUsageFlags bufferUsage,
    vkrMemUsage memUsage)
{
    return vkrBuffer_Reserve(vkrBufferSet_Current(set), size, bufferUsage, memUsage);
}

bool vkrBufferSet_Write(vkrBufferSet *const set, const void* src, i32 size)
{
    return vkrBuffer_Write(vkrBufferSet_Current(set), src, size);
}

bool vkrBufferSet_Read(
    vkrBufferSet *const set,
    void* dst,
    i32 size)
{
    return vkrBuffer_Read(vkrBufferSet_Current(set), dst, size);
}
