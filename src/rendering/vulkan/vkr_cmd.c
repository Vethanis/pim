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
        if (allocator->level == VK_COMMAND_BUFFER_LEVEL_PRIMARY)
        {
            VkFence* fences = allocator->fences;
            for (u32 i = 0; i < capacity; ++i)
            {
                vkrFence_Del(fences[i]);
            }
        }
        if (allocator->pool)
        {
            vkDestroyCommandPool(g_vkr.dev, allocator->pool, NULL);
        }
        Mem_Free(allocator->fences);
        Mem_Free(allocator->buffers);
        memset(allocator, 0, sizeof(*allocator));
    }
    ProfileEnd(pm_cmdallocdel);
}

void vkrCmdAlloc_OnSwapRecreate(vkrCmdAlloc* allocator)
{
    if (allocator->level == VK_COMMAND_BUFFER_LEVEL_SECONDARY)
    {
        memset(allocator->fences, 0, sizeof(allocator->fences[0]) * allocator->capacity);
    }
}

void vkrCmdAlloc_Reserve(vkrCmdAlloc* allocator, u32 newcap)
{
    ASSERT(allocator);
    u32 oldcap = allocator->capacity;
    if (newcap > oldcap)
    {
        newcap = (newcap > 16) ? newcap : 16;
        newcap = (newcap > oldcap * 2) ? newcap : oldcap * 2;
        const u32 deltacap = newcap - oldcap;

        Perm_Reserve(allocator->buffers, newcap);
        Perm_Reserve(allocator->fences, newcap);

        VkCommandBuffer* newbuffers = allocator->buffers + oldcap;
        VkFence* newfences = allocator->fences + oldcap;

        memset(newbuffers, 0, sizeof(newbuffers[0]) * deltacap);
        memset(newfences, 0, sizeof(newfences[0]) * deltacap);

        const VkCommandBufferAllocateInfo bufferInfo =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = allocator->pool,
            .level = allocator->level,
            .commandBufferCount = deltacap,
        };
        VkCheck(vkAllocateCommandBuffers(g_vkr.dev, &bufferInfo, newbuffers));

        if (allocator->level == VK_COMMAND_BUFFER_LEVEL_PRIMARY)
        {
            for (u32 i = 0; i < deltacap; ++i)
            {
                newfences[i] = vkrFence_New(true);
            }
        }

        allocator->capacity = newcap;
    }
}

VkCommandBuffer vkrCmdAlloc_GetTemp(vkrCmdAlloc* allocator, VkFence* fenceOut)
{
    ASSERT(allocator);
    ASSERT(allocator->pool);
    ASSERT(allocator->level == VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    ASSERT(fenceOut);

    VkFence fence = NULL;
    VkCommandBuffer buffer = NULL;
    while (!buffer)
    {
        const u32 capacity = allocator->capacity;
        VkCommandBuffer* const pim_noalias buffers = allocator->buffers;
        VkFence* const pim_noalias fences = allocator->fences;
        for (u32 i = 0; i < capacity; ++i)
        {
            ASSERT(fences[i]);
            if (vkrFence_Stat(fences[i]) == vkrFenceState_Signalled)
            {
                fence = fences[i];
                buffer = buffers[i];
                break;
            }
        }
        if (!buffer)
        {
            u32 newcap = capacity ? capacity * 2 : 16;
            vkrCmdAlloc_Reserve(allocator, newcap);
        }
    }

    ASSERT(fence);
    ASSERT(buffer);

    vkrFence_Reset(fence);
    *fenceOut = fence;

    return buffer;
}

VkCommandBuffer vkrCmdAlloc_GetSecondary(vkrCmdAlloc* allocator, VkFence primaryFence)
{
    ASSERT(allocator);
    ASSERT(allocator->pool);
    ASSERT(primaryFence);
    ASSERT(allocator->level == VK_COMMAND_BUFFER_LEVEL_SECONDARY);

    VkCommandBuffer buffer = NULL;
    while (!buffer)
    {
        const u32 capacity = allocator->capacity;
        VkCommandBuffer* const pim_noalias buffers = allocator->buffers;
        VkFence* const pim_noalias fences = allocator->fences;

        {
            VkFence prevFence = NULL;
            bool prevSignalled = false;
            for (u32 i = 0; i < capacity; ++i)
            {
                VkFence fence = fences[i];
                if (fence && (fence != primaryFence))
                {
                    if (fence == prevFence)
                    {
                        if (prevSignalled)
                        {
                            fences[i] = NULL;
                        }
                    }
                    else
                    {
                        prevFence = fence;
                        prevSignalled = false;
                        if (vkrFence_Stat(fence) == vkrFenceState_Signalled)
                        {
                            prevSignalled = true;
                            fences[i] = NULL;
                        }
                    }
                }
            }
        }

        for (u32 i = 0; i < capacity; ++i)
        {
            if (!fences[i])
            {
                fences[i] = primaryFence;
                buffer = buffers[i];
                break;
            }
        }

        if (!buffer)
        {
            u32 newcap = capacity ? capacity * 2 : 16;
            vkrCmdAlloc_Reserve(allocator, newcap);
        }
    }

    ASSERT(buffer);

    return buffer;
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

void vkrCmdBegin(VkCommandBuffer cmdbuf)
{
    ASSERT(cmdbuf);
    const VkCommandBufferBeginInfo beginInfo =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    VkCheck(vkBeginCommandBuffer(cmdbuf, &beginInfo));
}

void vkrCmdBeginSec(
    VkCommandBuffer cmd,
    VkRenderPass renderPass,
    i32 subpass,
    VkFramebuffer framebuffer)
{
    ASSERT(cmd);
    ASSERT(renderPass);
    ASSERT(subpass >= 0);
    const VkCommandBufferInheritanceInfo inherInfo =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
        .renderPass = renderPass,
        .subpass = subpass,
        .framebuffer = framebuffer,
    };
    const VkCommandBufferBeginInfo beginInfo =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT | VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = &inherInfo,
    };
    VkCheck(vkBeginCommandBuffer(cmd, &beginInfo));
}

void vkrCmdEnd(VkCommandBuffer cmdbuf)
{
    ASSERT(cmdbuf);
    VkCheck(vkEndCommandBuffer(cmdbuf));
}

ProfileMark(pm_reset, vkrCmdReset)
void vkrCmdReset(VkCommandBuffer cmdbuf)
{
    ProfileBegin(pm_reset);
    ASSERT(cmdbuf);
    VkCheck(vkResetCommandBuffer(cmdbuf, 0x0));
    ProfileEnd(pm_reset);
}

void vkrCmdBeginRenderPass(
    VkCommandBuffer cmdbuf,
    VkRenderPass pass,
    VkFramebuffer framebuf,
    VkRect2D rect,
    i32 clearCount,
    const VkClearValue* clearValues,
    VkSubpassContents contents)
{
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
    vkCmdBeginRenderPass(cmdbuf, &info, contents);
}

void vkrCmdExecCmds(VkCommandBuffer cmd, i32 count, const VkCommandBuffer* pSecondaries)
{
    ASSERT(cmd);
    ASSERT(count >= 0);
    ASSERT(pSecondaries || !count);
    if (count > 0)
    {
        vkCmdExecuteCommands(cmd, count, pSecondaries);
    }
}

void vkrCmdNextSubpass(VkCommandBuffer cmdbuf, VkSubpassContents contents)
{
    ASSERT(cmdbuf);
    vkCmdNextSubpass(cmdbuf, contents);
}

void vkrCmdEndRenderPass(VkCommandBuffer cmdbuf)
{
    ASSERT(cmdbuf);
    vkCmdEndRenderPass(cmdbuf);
}

void vkrCmdViewport(
    VkCommandBuffer cmdbuf,
    VkViewport viewport,
    VkRect2D scissor)
{
    ASSERT(cmdbuf);
    vkCmdSetViewport(cmdbuf, 0, 1, &viewport);
    vkCmdSetScissor(cmdbuf, 0, 1, &scissor);
}

void vkrCmdDraw(VkCommandBuffer cmdbuf, i32 vertexCount, i32 firstVertex)
{
    ASSERT(cmdbuf);
    vkCmdDraw(cmdbuf, vertexCount, 1, firstVertex, 0);
}

void vkrCmdDrawMesh(VkCommandBuffer cmdbuf, const vkrMesh* mesh)
{
    ASSERT(mesh);
    ASSERT(cmdbuf);
    ASSERT(mesh->vertCount >= 0);
    ASSERT(mesh->buffer.handle);

    if (mesh->vertCount > 0)
    {
        const VkDeviceSize streamSize = sizeof(float4) * mesh->vertCount;
        const VkBuffer buffers[] =
        {
            mesh->buffer.handle,
            mesh->buffer.handle,
            mesh->buffer.handle,
            mesh->buffer.handle,
        };
        const VkDeviceSize offsets[] =
        {
            streamSize * 0,
            streamSize * 1,
            streamSize * 2,
            streamSize * 3,
        };
        vkCmdBindVertexBuffers(cmdbuf, 0, NELEM(buffers), buffers, offsets);
        vkCmdDraw(cmdbuf, mesh->vertCount, 1, 0, 0);
    }
}

void vkrCmdCopyBuffer(
    VkCommandBuffer cmdbuf,
    const vkrBuffer* src,
    const vkrBuffer* dst)
{
    ASSERT(cmdbuf);
    ASSERT(src->handle);
    ASSERT(dst->handle);
    i32 size = pim_min(src->size, dst->size);
    ASSERT(size >= 0);
    if (size > 0)
    {
        const VkBufferCopy region =
        {
            .size = size,
        };
        vkCmdCopyBuffer(cmdbuf, src->handle, dst->handle, 1, &region);
    }
}

void vkrCmdBufferBarrier(
    VkCommandBuffer cmdbuf,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask,
    const VkBufferMemoryBarrier* barrier)
{
    ASSERT(cmdbuf);
    ASSERT(barrier);
    vkCmdPipelineBarrier(
        cmdbuf,
        srcStageMask, dstStageMask,
        0x0,
        0, NULL,
        1, barrier,
        0, NULL);
}

void vkrCmdImageBarrier(
    VkCommandBuffer cmdbuf,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask,
    const VkImageMemoryBarrier* barrier)
{
    ASSERT(cmdbuf);
    ASSERT(barrier);
    vkCmdPipelineBarrier(
        cmdbuf,
        srcStageMask, dstStageMask,
        0x0,
        0, NULL,
        0, NULL,
        1, barrier);
}

void vkrCmdPushConstants(
    VkCommandBuffer cmdbuf,
    VkPipelineLayout layout,
    VkShaderStageFlags stages,
    const void* dwords,
    i32 bytes)
{
    ASSERT(cmdbuf);
    ASSERT(layout);
    ASSERT(stages);
    ASSERT(dwords || !bytes);
    ASSERT(bytes >= 0);
    if (bytes > 0)
    {
        vkCmdPushConstants(cmdbuf, layout, stages, 0, bytes, dwords);
    }
}

void vkrCmdBindDescSets(
    VkCommandBuffer cmdbuf,
    VkPipelineBindPoint bindpoint,
    VkPipelineLayout layout,
    i32 setCount,
    const VkDescriptorSet* sets)
{
    ASSERT(cmdbuf);
    ASSERT(layout);
    ASSERT(sets || !setCount);
    ASSERT(setCount >= 0);
    if (setCount > 0)
    {
        vkCmdBindDescriptorSets(
            cmdbuf,
            bindpoint,
            layout,
            0,
            setCount, sets,
            0, NULL);
    }
}
