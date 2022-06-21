#include "rendering/vulkan/vkr_buffer.h"
#include "rendering/vulkan/vkr_mem.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_context.h"
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
            //.frame = vkrGetFrameCount(),
            .submitId = vkrBuffer_GetSubmit(buffer),
            .type = vkrReleasableType_Buffer,
            .buffer.handle = buffer->handle,
            .buffer.allocation = buffer->allocation,
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

const void* vkrBuffer_MapRead(vkrBuffer* buffer)
{
    ASSERT(buffer);
    bool hasStage = (buffer->state.stage & VK_PIPELINE_STAGE_HOST_BIT) != 0;
    bool hasAccess = (buffer->state.access & VK_ACCESS_HOST_READ_BIT) != 0;
    if (!hasStage || !hasAccess)
    {
        // This sucks but im lazy.
        vkrCmdBuf* cmd = vkrCmdGet_G();
        vkrBufferState_HostRead(cmd, buffer);
        vkrSubmitId submitId = vkrCmdSubmit(cmd, NULL, 0x0, NULL);
        vkrSubmit_Await(submitId);
        cmd = vkrCmdGet_G();
        ASSERT(cmd->handle);
    }
    ASSERT(buffer->state.stage & VK_PIPELINE_STAGE_HOST_BIT);
    ASSERT(buffer->state.access & VK_ACCESS_HOST_READ_BIT);
    return vkrMem_Map(buffer->allocation);
}

void vkrBuffer_UnmapRead(vkrBuffer* buffer)
{
    ASSERT(buffer);
    ASSERT(buffer->state.stage & VK_PIPELINE_STAGE_HOST_BIT);
    ASSERT(buffer->state.access & VK_ACCESS_HOST_READ_BIT);
    vkrMem_Unmap(buffer->allocation);
}

void* vkrBuffer_MapWrite(vkrBuffer* buffer)
{
    ASSERT(buffer);
    if (!buffer->state.stage)
    {
        // newly created buffer
        buffer->state.stage = VK_PIPELINE_STAGE_HOST_BIT;
        buffer->state.access = VK_ACCESS_HOST_WRITE_BIT;
    }
    bool hasStage = (buffer->state.stage & VK_PIPELINE_STAGE_HOST_BIT) != 0;
    bool hasAccess = (buffer->state.access & VK_ACCESS_HOST_WRITE_BIT) != 0;
    if (!hasStage || !hasAccess)
    {
        // This sucks but im lazy.
        vkrCmdBuf* cmd = vkrCmdGet_G();
        vkrBufferState_HostWrite(cmd, buffer);
        vkrSubmitId submitId = vkrCmdSubmit(cmd, NULL, 0x0, NULL);
        vkrSubmit_Await(submitId);
        cmd = vkrCmdGet_G();
        ASSERT(cmd->handle);
    }
    ASSERT(buffer->state.stage & VK_PIPELINE_STAGE_HOST_BIT);
    ASSERT(buffer->state.access & VK_ACCESS_HOST_WRITE_BIT);
    return vkrMem_Map(buffer->allocation);
}

void vkrBuffer_UnmapWrite(vkrBuffer* buffer)
{
    ASSERT(buffer);
    ASSERT(buffer->state.stage & VK_PIPELINE_STAGE_HOST_BIT);
    ASSERT(buffer->state.access & VK_ACCESS_HOST_WRITE_BIT);
    vkrMem_Unmap(buffer->allocation);
    vkrMem_Flush(buffer->allocation);
}

ProfileMark(pm_bufferwrite, vkrBuffer_Write)
bool vkrBuffer_Write(
    vkrBuffer* buffer,
    const void* src,
    i32 size)
{
    ProfileBegin(pm_bufferwrite);
    ASSERT(buffer);
    ASSERT(src);
    ASSERT(size >= 0);
    ASSERT(size <= buffer->size);
    bool success = false;
    void* dst = vkrBuffer_MapWrite(buffer);
    if (dst)
    {
        memcpy(dst, src, size);
        success = true;
    }
    vkrBuffer_UnmapWrite(buffer);
    ProfileEnd(pm_bufferwrite);
    return success;
}

ProfileMark(pm_bufferread, vkrBuffer_Read)
bool vkrBuffer_Read(
    vkrBuffer* buffer,
    void* dst,
    i32 size)
{
    ProfileBegin(pm_bufferread);
    ASSERT(buffer);
    ASSERT(dst);
    ASSERT(size == buffer->size);
    bool success = false;
    const void* src = vkrBuffer_MapRead(buffer);
    if (src)
    {
        memcpy(dst, src, size);
        success = true;
    }
    vkrBuffer_UnmapRead(buffer);
    ProfileEnd(pm_bufferread);
    return success;
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
    u32 syncIndex = vkrGetSyncIndex();
    ASSERT(syncIndex < NELEM(set->frames));
    return &set->frames[syncIndex];
}
vkrBuffer *const vkrBufferSet_Next(vkrBufferSet *const set)
{
    u32 syncIndex = vkrGetNextSyncIndex();
    ASSERT(syncIndex < NELEM(set->frames));
    return &set->frames[syncIndex];
}
vkrBuffer *const vkrBufferSet_Prev(vkrBufferSet *const set)
{
    u32 syncIndex = vkrGetPrevSyncIndex();
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
