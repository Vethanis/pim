#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_sync.h"
#include "allocator/allocator.h"
#include "common/profiler.h"
#include "common/time.h"
#include "math/types.h"
#include "threading/task.h"
#include <string.h>

ProfileMark(pm_cmdpoolnew, vkrCmdPool_New)
VkCommandPool vkrCmdPool_New(i32 family, VkCommandPoolCreateFlags flags)
{
    ProfileBegin(pm_cmdpoolnew);
    const VkCommandPoolCreateInfo poolInfo =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = flags,
        .queueFamilyIndex = family,
    };
    VkCommandPool pool = NULL;
    VkCheck(vkCreateCommandPool(g_vkr.dev, &poolInfo, NULL, &pool));
    ASSERT(pool);
    ProfileEnd(pm_cmdpoolnew);
    return pool;
}

ProfileMark(pm_cmdpooldel, vkrCmdPool_Del)
void vkrCmdPool_Del(VkCommandPool pool)
{
    if (pool)
    {
        ProfileBegin(pm_cmdpooldel);
        vkDestroyCommandPool(g_vkr.dev, pool, NULL);
        ProfileEnd(pm_cmdpooldel);
    }
}

ProfileMark(pm_cmdpoolreset, vkrCmdPool_Reset)
void vkrCmdPool_Reset(VkCommandPool pool, VkCommandPoolResetFlagBits flags)
{
    ProfileBegin(pm_cmdpoolreset);
    ASSERT(pool);
    VkCheck(vkResetCommandPool(g_vkr.dev, pool, flags));
    ProfileEnd(pm_cmdpoolreset);
}

// ----------------------------------------------------------------------------

ProfileMark(pm_cmdallocnew, vkrCmdAlloc_New)
bool vkrCmdAlloc_New(
    vkrCmdAlloc* allocator,
    const vkrQueue* queue,
    VkCommandBufferLevel level)
{
    ProfileBegin(pm_cmdallocnew);
    ASSERT(allocator);
    ASSERT(queue);
    ASSERT(queue->handle);

    bool success = true;
    memset(allocator, 0, sizeof(*allocator));

    allocator->queue = queue->handle;
    if (!allocator->queue)
    {
        success = false;
        goto cleanup;
    }
    allocator->pool = vkrCmdPool_New(
        queue->family,
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    if (!allocator->pool)
    {
        success = false;
        goto cleanup;
    }
    allocator->level = level;

cleanup:
    if (!success)
    {
        vkrCmdAlloc_Del(allocator);
    }
    ProfileEnd(pm_cmdallocnew);
    return success;
}

ProfileMark(pm_cmdallocdel, vkrCmdAlloc_Del)
void vkrCmdAlloc_Del(vkrCmdAlloc* allocator)
{
    ProfileBegin(pm_cmdallocdel);
    if (allocator)
    {
        const u32 capacity = allocator->capacity;
        VkFence* fences = allocator->fences;
        VkCommandBuffer* buffers = allocator->buffers;
        VkCommandPool pool = allocator->pool;
        for (u32 i = 0; i < capacity; ++i)
        {
            vkrFence_Del(fences[i]);
        }
        if (pool)
        {
            vkDestroyCommandPool(g_vkr.dev, pool, NULL);
        }
        pim_free(fences);
        pim_free(buffers);
        memset(allocator, 0, sizeof(*allocator));
    }
    ProfileEnd(pm_cmdallocdel);
}

ProfileMark(pm_cmdallocreserve, vkrCmdAlloc_Reserve)
void vkrCmdAlloc_Reserve(vkrCmdAlloc* allocator, u32 newcap)
{
    ProfileBegin(pm_cmdallocreserve);
    ASSERT(allocator);
    u32 oldcap = allocator->capacity;
    if (newcap > oldcap)
    {
        newcap = (newcap > 4) ? newcap : 4;
        newcap = (newcap > oldcap * 2) ? newcap : oldcap * 2;
        u32 deltacap = newcap - oldcap;
        PermReserve(allocator->fences, newcap);
        PermReserve(allocator->buffers, newcap);
        VkCommandBuffer* newbuffers = allocator->buffers + oldcap;
        VkFence* newfences = allocator->fences + oldcap;
        const VkCommandBufferAllocateInfo bufferInfo =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = allocator->pool,
            .level = allocator->level,
            .commandBufferCount = deltacap,
        };
        VkCheck(vkAllocateCommandBuffers(g_vkr.dev, &bufferInfo, newbuffers));
        for (u32 i = 0; i < deltacap; ++i)
        {
            newfences[i] = vkrFence_New(false);
        }
        allocator->capacity = newcap;
    }
    ProfileEnd(pm_cmdallocreserve);
}

ProfileMark(pm_cmdallocreset, vkrCmdAlloc_Reset)
void vkrCmdAlloc_Reset(vkrCmdAlloc* allocator)
{
    ProfileBegin(pm_cmdallocreset);
    ASSERT(allocator);
    ASSERT(allocator->pool);
    const u32 head = allocator->head;
    if (head > 0)
    {
        VkCheck(vkResetCommandPool(g_vkr.dev, allocator->pool, 0x0));
        VkFence* fences = allocator->fences;
        for (u32 i = 0; i < head; ++i)
        {
            vkrFence_Reset(fences[i]);
        }
    }
    allocator->head = 0;
    allocator->frame = time_framecount();
    ProfileEnd(pm_cmdallocreset);
}

ProfileMark(pm_cmdallocget, vkrCmdAlloc_Get)
void vkrCmdAlloc_Get(vkrCmdAlloc* allocator, VkCommandBuffer* cmdOut, VkFence* fenceOut)
{
    ProfileBegin(pm_cmdallocget);
    ASSERT(allocator);
    ASSERT(allocator->pool);
    if (allocator->frame != time_framecount())
    {
        vkrCmdAlloc_Reset(allocator);
    }
    VkCommandBuffer buffer = NULL;
    VkFence fence = NULL;
    u32 head = allocator->head;
    if (head >= 8)
    {
        // reuse command buffers to avoid making more
        const VkFence* fences = allocator->fences;
        for (u32 i = 0; i < head; ++i)
        {
            if (vkrFence_Stat(fences[i]) == vkrFenceState_Signalled)
            {
                fence = fences[i];
                buffer = allocator->buffers[i];
                vkrFence_Reset(fence);
                VkCheck(vkResetCommandBuffer(buffer, 0x0));
                break;
            }
        }
    }
    if (!buffer)
    {
        vkrCmdAlloc_Reserve(allocator, head + 1);
        allocator->head = head + 1;
        ASSERT(head < allocator->capacity);
        buffer = allocator->buffers[head];
        fence = allocator->fences[head];
    }
    ASSERT(buffer);
    ASSERT(fence);
    ASSERT(cmdOut);
    *cmdOut = buffer;
    if (fenceOut)
    {
        *fenceOut = fence;
    }
    ProfileEnd(pm_cmdallocget);
}

// ----------------------------------------------------------------------------

ProfileMark(pm_cmdsubmit, vkrCmdSubmit)
void vkrCmdSubmit(
    VkQueue queue,
    VkCommandBuffer cmd,
    VkFence fence,
    VkSemaphore waitSema,
    VkPipelineStageFlags waitMask,
    VkSemaphore signalSema)
{
    ProfileBegin(pm_cmdsubmit);
    ASSERT(cmd);
    ASSERT(queue);
    const VkSubmitInfo submitInfo =
    {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = waitSema ? 1 : 0,
        .pWaitSemaphores = &waitSema,
        .pWaitDstStageMask = &waitMask,
        .signalSemaphoreCount = signalSema ? 1 : 0,
        .pSignalSemaphores = &signalSema,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd,
    };
    VkCheck(vkQueueSubmit(queue, 1, &submitInfo, fence));
    ProfileEnd(pm_cmdsubmit);
}

ProfileMark(pm_begin, vkrCmdBegin)
void vkrCmdBegin(VkCommandBuffer cmdbuf)
{
    ProfileBegin(pm_begin);
    ASSERT(cmdbuf);
    const VkCommandBufferBeginInfo beginInfo =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = 0x0,
    };
    VkCheck(vkBeginCommandBuffer(cmdbuf, &beginInfo));
    ProfileEnd(pm_begin);
}

ProfileMark(pm_end, vkrCmdEnd)
void vkrCmdEnd(VkCommandBuffer cmdbuf)
{
    ASSERT(cmdbuf);
    ProfileBegin(pm_end);
    VkCheck(vkEndCommandBuffer(cmdbuf));
    ProfileEnd(pm_end);
}

ProfileMark(pm_reset, vkrCmdReset)
void vkrCmdReset(VkCommandBuffer cmdbuf)
{
    ProfileBegin(pm_reset);
    ASSERT(cmdbuf);
    VkCheck(vkResetCommandBuffer(cmdbuf, 0x0));
    ProfileEnd(pm_reset);
}

ProfileMark(pm_beginrenderpass, vkrCmdBeginRenderPass)
void vkrCmdBeginRenderPass(
    VkCommandBuffer cmdbuf,
    VkRenderPass pass,
    VkFramebuffer framebuf,
    VkRect2D rect,
    i32 clearCount,
    const VkClearValue* clearValues)
{
    ProfileBegin(pm_beginrenderpass);
    ASSERT(cmdbuf);
    ASSERT(pass);
    ASSERT(framebuf);
    const VkRenderPassBeginInfo info =
    {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = pass,
        .framebuffer = framebuf,
        .renderArea = rect,
        .clearValueCount = clearCount,
        .pClearValues = clearValues,
    };
    vkCmdBeginRenderPass(cmdbuf, &info, VK_SUBPASS_CONTENTS_INLINE);
    ProfileEnd(pm_beginrenderpass);
}

ProfileMark(pm_nextsubpass, vkrCmdNextSubpass)
void vkrCmdNextSubpass(VkCommandBuffer cmdbuf)
{
    ProfileBegin(pm_nextsubpass);
    ASSERT(cmdbuf);
    vkCmdNextSubpass(cmdbuf, VK_SUBPASS_CONTENTS_INLINE);
    ProfileEnd(pm_nextsubpass);
}

ProfileMark(pm_endrenderpass, vkrCmdEndRenderPass)
void vkrCmdEndRenderPass(VkCommandBuffer cmdbuf)
{
    ProfileBegin(pm_endrenderpass);
    ASSERT(cmdbuf);
    vkCmdEndRenderPass(cmdbuf);
    ProfileEnd(pm_endrenderpass);
}

ProfileMark(pm_bindpipeline, vkrCmdBindPipeline)
void vkrCmdBindPipeline(VkCommandBuffer cmdbuf, const vkrPipeline* pipeline)
{
    ProfileBegin(pm_bindpipeline);
    ASSERT(cmdbuf);
    ASSERT(pipeline);
    vkCmdBindPipeline(cmdbuf, pipeline->bindpoint, pipeline->handle);
    ProfileEnd(pm_bindpipeline);
}

ProfileMark(pm_viewport, vkrCmdViewport)
void vkrCmdViewport(
    VkCommandBuffer cmdbuf,
    VkViewport viewport,
    VkRect2D scissor)
{
    ProfileBegin(pm_viewport);
    ASSERT(cmdbuf);
    vkCmdSetViewport(cmdbuf, 0, 1, &viewport);
    vkCmdSetScissor(cmdbuf, 0, 1, &scissor);
    ProfileEnd(pm_viewport);
}

ProfileMark(pm_draw, vkrCmdDraw)
void vkrCmdDraw(VkCommandBuffer cmdbuf, i32 vertexCount, i32 firstVertex)
{
    ProfileBegin(pm_draw);
    ASSERT(cmdbuf);
    vkCmdDraw(cmdbuf, vertexCount, 1, firstVertex, 0);
    ProfileEnd(pm_draw);
}

void vkrCmdDrawMesh(VkCommandBuffer cmdbuf, const vkrMesh* mesh)
{
    ASSERT(mesh);
    ASSERT(cmdbuf);
    ASSERT(cmdbuf);
    ASSERT(mesh->vertCount >= 0);
    ASSERT(mesh->indexCount >= 0);
    ASSERT(mesh->buffer.handle);

    if (mesh->vertCount > 0)
    {
        const VkDeviceSize streamSize = sizeof(float4) * mesh->vertCount;
        const VkDeviceSize indexOffset = streamSize * vkrMeshStream_COUNT;
        const VkBuffer buffers[] =
        {
            mesh->buffer.handle,
            mesh->buffer.handle,
            mesh->buffer.handle,
        };
        const VkDeviceSize offsets[] =
        {
            streamSize * 0,
            streamSize * 1,
            streamSize * 2,
        };
        vkCmdBindVertexBuffers(cmdbuf, 0, NELEM(buffers), buffers, offsets);
        if (mesh->indexCount > 0)
        {
            vkCmdBindIndexBuffer(cmdbuf, mesh->buffer.handle, indexOffset, VK_INDEX_TYPE_UINT16);
            vkCmdDrawIndexed(cmdbuf, mesh->indexCount, 1, 0, 0, 0);
        }
        else
        {
            vkCmdDraw(cmdbuf, mesh->vertCount, 1, 0, 0);
        }
    }
}

ProfileMark(pm_copybuffer, vkrCmdCopyBuffer)
void vkrCmdCopyBuffer(VkCommandBuffer cmdbuf, vkrBuffer src, vkrBuffer dst)
{
    ASSERT(cmdbuf);
    ASSERT(src.handle);
    ASSERT(dst.handle);
    i32 size = src.size < dst.size ? src.size : dst.size;
    ASSERT(size >= 0);
    if (size > 0)
    {
        ProfileBegin(pm_copybuffer);
        const VkBufferCopy region =
        {
            .size = size,
        };
        vkCmdCopyBuffer(cmdbuf, src.handle, dst.handle, 1, &region);
        ProfileEnd(pm_copybuffer);
    }
}

ProfileMark(pm_bufferbarrier, vkrCmdBufferBarrier)
void vkrCmdBufferBarrier(
    VkCommandBuffer cmdbuf,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask,
    const VkBufferMemoryBarrier* barrier)
{
    ProfileBegin(pm_bufferbarrier);
    ASSERT(cmdbuf);
    ASSERT(barrier);
    vkCmdPipelineBarrier(
        cmdbuf,
        srcStageMask, dstStageMask,
        0x0,
        0, NULL,
        1, barrier,
        0, NULL);
    ProfileEnd(pm_bufferbarrier);
}

ProfileMark(pm_imagebarrier, vkrCmdImageBarrier)
void vkrCmdImageBarrier(
    VkCommandBuffer cmdbuf,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask,
    const VkImageMemoryBarrier* barrier)
{
    ProfileBegin(pm_imagebarrier);
    ASSERT(cmdbuf);
    ASSERT(barrier);
    vkCmdPipelineBarrier(
        cmdbuf,
        srcStageMask, dstStageMask,
        0x0,
        0, NULL,
        0, NULL,
        1, barrier);
    ProfileEnd(pm_imagebarrier);
}

void vkrCmdPushConstants(
    VkCommandBuffer cmdbuf,
    const vkrPipeline* pipeline,
    const void* dwords,
    i32 bytes)
{
    ASSERT(cmdbuf);
    ASSERT(pipeline);
    ASSERT(pipeline->layout.handle);
    ASSERT(dwords || !bytes);
    ASSERT(bytes >= 0);
    if (bytes > 0)
    {
        const u32 stages = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;
        vkCmdPushConstants(cmdbuf, pipeline->layout.handle, stages, 0, bytes, dwords);
    }
}

ProfileMark(pm_binddescsets, vkrCmdBindDescSets)
void vkrCmdBindDescSets(
    VkCommandBuffer cmdbuf,
    const vkrPipeline* pipeline,
    i32 setCount,
    const VkDescriptorSet* sets)
{
    ProfileBegin(pm_binddescsets);
    ASSERT(cmdbuf);
    ASSERT(pipeline);
    ASSERT(sets || !setCount);
    ASSERT(setCount >= 0);
    if (setCount > 0)
    {
        vkCmdBindDescriptorSets(
            cmdbuf,
            pipeline->bindpoint,
            pipeline->layout.handle,
            0,
            setCount, sets,
            0, NULL);
    }
    ProfileEnd(pm_binddescsets);
}
