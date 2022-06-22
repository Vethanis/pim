#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_sync.h"
#include "rendering/vulkan/vkr_swapchain.h"
#include "rendering/vulkan/vkr_image.h"
#include "rendering/vulkan/vkr_bindings.h"
#include "allocator/allocator.h"
#include "common/profiler.h"
#include "common/time.h"
#include "math/types.h"
#include "threading/task.h"
#include <string.h>

// ----------------------------------------------------------------------------

static const VkAccessFlags kWriteAccess =
    VK_ACCESS_SHADER_WRITE_BIT |
    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
    VK_ACCESS_TRANSFER_WRITE_BIT |
    VK_ACCESS_HOST_WRITE_BIT |
    VK_ACCESS_MEMORY_WRITE_BIT |
    VK_ACCESS_TRANSFORM_FEEDBACK_WRITE_BIT_EXT |
    VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT |
    VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR |
    VK_ACCESS_COMMAND_PREPROCESS_WRITE_BIT_NV;
static const VkAccessFlags kReadAccess =
    VK_ACCESS_INDIRECT_COMMAND_READ_BIT |
    VK_ACCESS_INDEX_READ_BIT |
    VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
    VK_ACCESS_UNIFORM_READ_BIT |
    VK_ACCESS_INPUT_ATTACHMENT_READ_BIT |
    VK_ACCESS_SHADER_READ_BIT |
    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
    VK_ACCESS_TRANSFER_READ_BIT |
    VK_ACCESS_HOST_READ_BIT |
    VK_ACCESS_MEMORY_READ_BIT |
    VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT |
    VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT |
    VK_ACCESS_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT |
    VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR |
    VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT |
    VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR |
    VK_ACCESS_COMMAND_PREPROCESS_READ_BIT_NV;

// ----------------------------------------------------------------------------

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
    vkrQueue* queue,
    vkrQueueId queueId)
{
    ProfileBegin(pm_cmdallocnew);
    ASSERT(queue);
    ASSERT(queue->handle);
    ASSERT(queue == vkrGetQueue(queueId));
    ASSERT(!queue->cmdPool);

    bool success = true;

    queue->queueId = queueId;
    queue->cmdPool = vkrCmdPool_New(
        queue->family,
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    if (!queue->cmdPool)
    {
        success = false;
        goto cleanup;
    }

    const VkCommandBufferAllocateInfo bufferInfo =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = queue->cmdPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = NELEM(queue->cmds),
    };
    VkCheck(vkAllocateCommandBuffers(g_vkr.dev, &bufferInfo, queue->cmds));
    if (!queue->cmds[0])
    {
        success = false;
        goto cleanup;
    }

    for (u32 i = 0; i < NELEM(queue->cmds); ++i)
    {
        queue->cmdFences[i] = vkrFence_New(true);
        queue->cmdIds[i] = 0;
    }
    if (!queue->cmdFences[0])
    {
        success = false;
        goto cleanup;
    }

cleanup:
    if (!success)
    {
        vkrCmdAlloc_Del(queue);
    }
    ProfileEnd(pm_cmdallocnew);
    return success;
}

ProfileMark(pm_cmdallocdel, vkrCmdAlloc_Del)
void vkrCmdAlloc_Del(vkrQueue* queue)
{
    ProfileBegin(pm_cmdallocdel);
    ASSERT(g_vkr.dev);
    VkCommandPool pool = queue->cmdPool;
    VkFence* fences = queue->cmdFences;
    VkCommandBuffer* cmds = queue->cmds;
    u32* cmdIds = queue->cmdIds;
    queue->cmdPool = NULL;
    for (u32 i = 0; i < NELEM(queue->cmdFences); ++i)
    {
        vkrFence_Del(fences[i]);
        fences[i] = NULL;
        cmds[i] = NULL;
        cmdIds[i] = 0;
    }
    if (pool)
    {
        vkDestroyCommandPool(g_vkr.dev, pool, NULL);
    }
    ProfileEnd(pm_cmdallocdel);
}

ProfileMark(pm_cmdalloc_alloc, vkrCmdAlloc_Alloc)
void vkrCmdAlloc_Alloc(vkrQueue* queue, vkrCmdBuf* cmdOut)
{
    ProfileBegin(pm_cmdalloc_alloc);

    ASSERT(queue);
    ASSERT(queue->handle);
    ASSERT(queue->cmdPool);
    ASSERT(cmdOut);
    ASSERT(!cmdOut->handle);

    const u32 kRingMask = NELEM(queue->cmds) - 1u;
    SASSERT((NELEM(queue->cmds) & (NELEM(queue->cmds) - 1u)) == 0);

    VkCommandBuffer* const pim_noalias cmds = queue->cmds;
    VkFence* const pim_noalias fences = queue->cmdFences;
    u32* const pim_noalias ids = queue->cmdIds;

    const u32 headId = queue->head++;

    while ((headId - queue->tail) >= NELEM(queue->cmds))
    {
        u32 tailSlot = queue->tail & kRingMask;
        ASSERT(ids[tailSlot] == queue->tail);
        VkFence tailFence = fences[tailSlot];
        vkrFence_Wait(tailFence);
        ids[tailSlot] = 0;
        queue->tail++;
    }

    const u32 headSlot = headId & kRingMask;
    ASSERT(!ids[headSlot]);
    ids[headSlot] = headId;
    VkCommandBuffer cmd = cmds[headSlot];
    VkFence fence = fences[headSlot];

    ASSERT(fence);
    ASSERT(cmd);
    ASSERT(vkrFence_Stat(fence) == vkrFenceState_Signalled);
    vkrFence_Reset(fence);

    memset(cmdOut, 0, sizeof(*cmdOut));
    cmdOut->handle = cmd;
    cmdOut->fence = fence;
    cmdOut->id = headId;
    cmdOut->queueId = queue->queueId;
    cmdOut->gfx = queue->gfx;
    cmdOut->comp = queue->comp;
    cmdOut->xfer = queue->xfer;
    cmdOut->pres = queue->pres;

    ProfileEnd(pm_cmdalloc_alloc);
}

// ----------------------------------------------------------------------------

//vkrCmdBuf* vkrCmdGet_P(void) { return vkrCmdGet(vkrQueueId_Present); }
vkrCmdBuf* vkrCmdGet_G(void) { return vkrCmdGet(vkrQueueId_Graphics); }
//vkrCmdBuf* vkrCmdGet_C(void) { return vkrCmdGet(vkrQueueId_Compute); }
//vkrCmdBuf* vkrCmdGet_T(void) { return vkrCmdGet(vkrQueueId_Transfer); }

vkrCmdBuf* vkrCmdGet(vkrQueueId queueId)
{
    vkrContext* ctx = vkrGetContext();
    ASSERT(queueId < NELEM(ctx->curCmdBuf));
    vkrCmdBuf* cmd = &ctx->curCmdBuf[queueId];
    if (!cmd->handle)
    {
        vkrCmdAlloc_Alloc(vkrGetQueue(queueId), cmd);
    }
    if (!cmd->began)
    {
        const VkCommandBufferBeginInfo beginInfo =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };
        VkCheck(vkBeginCommandBuffer(cmd->handle, &beginInfo));
        cmd->began = 1;
        ctx->mostRecentBegin = queueId;
    }
    ASSERT(cmd->handle);
    ASSERT(cmd->began);
    ASSERT(!cmd->ended);
    ASSERT(!cmd->submit);
    return cmd;
}

ProfileMark(pm_reset, vkrCmdReset)
void vkrCmdReset(vkrCmdBuf* cmd)
{
    ProfileBegin(pm_reset);
    ASSERT(!cmd->submit);
    if (cmd->handle)
    {
        if (cmd->began && !cmd->ended)
        {
            VkCheck(vkEndCommandBuffer(cmd->handle));
            cmd->ended = 1;
        }
        VkCheck(vkResetCommandBuffer(cmd->handle, 0x0));
        cmd->began = 0;
        cmd->ended = 0;
    }
    ProfileEnd(pm_reset);
}

ProfileMark(pm_cmdsubmit, vkrCmdSubmit)
vkrSubmitId vkrCmdSubmit(
    vkrCmdBuf* cmd,
    VkSemaphore waitSema,
    VkPipelineStageFlags waitMask,
    VkSemaphore signalSema)
{
    ProfileBegin(pm_cmdsubmit);

    ASSERT(cmd);
    ASSERT(cmd->handle);
    ASSERT(cmd->began);
    ASSERT(!cmd->ended);
    ASSERT(!cmd->submit);
    ASSERT(!cmd->inRenderPass);

    vkrContext* ctx = vkrGetContext();
    const vkrQueueId queueId = cmd->queueId;
    vkrQueue* queue = vkrGetQueue(queueId);
    ASSERT(queue->handle);

    vkrSubmitId submitId;
    submitId.counter = cmd->id;
    submitId.queueId = queueId;

    u32 slot = cmd->id & (NELEM(queue->cmds) - 1u);
    VkFence fence = queue->cmdFences[slot];
    ASSERT(fence);

    if (cmd->queueTransferDst)
    {
        ASSERT(!cmd->queueTransferSrc);
        vkrCmdFlushQueueTransfers();
    }
    ASSERT(!cmd->queueTransferDst);

    ASSERT(!cmd->ended);
    VkCheck(vkEndCommandBuffer(cmd->handle));
    cmd->ended = 1;

    ASSERT(vkrFence_Stat(fence) == vkrFenceState_Unsignalled);

    const VkSubmitInfo submitInfo =
    {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = waitSema ? 1 : 0,
        .pWaitSemaphores = &waitSema,
        .pWaitDstStageMask = &waitMask,
        .signalSemaphoreCount = signalSema ? 1 : 0,
        .pSignalSemaphores = &signalSema,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd->handle,
    };
    VkCheck(vkQueueSubmit(queue->handle, 1, &submitInfo, fence));
    cmd->submit = 1;
    ctx->lastSubmitQueue = cmd->queueId;

    // copy to prevCmd for debug purposes
    vkrCmdBuf* prevCmd = &ctx->prevCmdBuf[queueId];
    memcpy(prevCmd, cmd, sizeof(*cmd));

    // reset to blank state
    memset(cmd, 0, sizeof(*cmd));
    cmd->queueId = queueId;
    submitId.valid = 1;

    ProfileEnd(pm_cmdsubmit);

    return submitId;
}

vkrSubmitId vkrGetHeadSubmit(vkrQueueId queueId)
{
    vkrSubmitId id = { 0 };

    const vkrContext* ctx = vkrGetContext();
    ASSERT(queueId < NELEM(ctx->curCmdBuf));
    const vkrCmdBuf* cur = &ctx->curCmdBuf[queueId];
    const vkrCmdBuf* prev = &ctx->prevCmdBuf[queueId];

    const vkrCmdBuf* cmd = (cur->handle) ? cur : prev;
    if (cur->handle && cur->began)
    {
        id.counter = cmd->id;
        id.queueId = cmd->queueId;
        id.valid = (cmd->handle != NULL) ? 1 : 0;
    }
    return id;
}

ProfileMark(pm_submitpoll, vkrSubmit_Poll)
bool vkrSubmit_Poll(vkrSubmitId submit)
{
    ProfileBegin(pm_submitpoll);

    ASSERT(submit.valid);
    bool signalled = true;

    vkrQueue* queue = vkrGetQueue(submit.queueId);
    VkFence* const pim_noalias fences = queue->cmdFences;
    VkCommandBuffer* const pim_noalias cmds = queue->cmds;
    u32* const pim_noalias ids = queue->cmdIds;

    const u32 kRingMask = NELEM(queue->cmds) - 1u;
    while ((submit.counter - queue->tail) < NELEM(queue->cmds))
    {
        u32 i = queue->tail & kRingMask;
        ASSERT(ids[i] == queue->tail);
        if (vkrFence_Stat(fences[i]) != vkrFenceState_Signalled)
        {
            signalled = false;
            goto end;
        }
        VkCheck(vkResetCommandBuffer(cmds[i], 0x0));
        ids[i] = 0;
        queue->tail++;
    }

end:
    ProfileEnd(pm_submitpoll);
    return signalled;
}

ProfileMark(pm_submitawait, vkrSubmit_Await)
void vkrSubmit_Await(vkrSubmitId submit)
{
    ProfileBegin(pm_submitawait);

    ASSERT(submit.valid);

    vkrQueue* queue = vkrGetQueue(submit.queueId);
    VkFence* const pim_noalias fences = queue->cmdFences;
    VkCommandBuffer* const pim_noalias cmds = queue->cmds;
    u32* const pim_noalias ids = queue->cmdIds;

    const u32 kRingMask = NELEM(queue->cmds) - 1u;
    while ((submit.counter - queue->tail) < NELEM(queue->cmds))
    {
        u32 i = queue->tail & kRingMask;
        ASSERT(ids[i] == queue->tail);
        vkrFence_Wait(fences[i]);
        VkCheck(vkResetCommandBuffer(cmds[i], 0x0));
        ids[i] = 0;
        queue->tail++;
    }

    ProfileEnd(pm_submitawait);
}

ProfileMark(pm_submitawaitall, vkrSubmit_AwaitAll)
void vkrSubmit_AwaitAll(void)
{
    ProfileBegin(pm_submitawaitall);

    for (u32 queueId = 0; queueId < vkrQueueId_COUNT; ++queueId)
    {
        vkrQueue* queue = vkrGetQueue(queueId);
        const u32 kRingMask = NELEM(queue->cmds) - 1u;
        VkFence* const pim_noalias fences = queue->cmdFences;
        VkCommandBuffer* const pim_noalias cmds = queue->cmds;
        u32* const pim_noalias ids = queue->cmdIds;
        while (queue->tail != queue->head)
        {
            u32 i = queue->tail & kRingMask;
            ASSERT(ids[i] == queue->tail);
            vkrFence_Wait(fences[i]);
            VkCheck(vkResetCommandBuffer(cmds[i], 0x0));
            ids[i] = 0;
            queue->tail++;
        }
    }

    ProfileEnd(pm_submitawaitall);
}

ProfileMark(pm_cmdflush, vkrCmdFlush)
void vkrCmdFlush(void)
{
    ProfileBegin(pm_cmdflush);

    vkrContext* ctx = vkrGetContext();
    for (i32 id = 0; id < NELEM(ctx->curCmdBuf); ++id)
    {
        vkrCmdBuf* cmd = &ctx->curCmdBuf[id];
        if (cmd->began)
        {
            vkrCmdSubmit(cmd, NULL, 0, NULL);
        }
    }

    ProfileEnd(pm_cmdflush);
}

ProfileMark(pm_cmdflushqueuetransfers, vkrCmdFlushQueueTransfers)
void vkrCmdFlushQueueTransfers(void)
{
    ProfileBegin(pm_cmdflushqueuetransfers);

    vkrContext* ctx = vkrGetContext();
    bool submit[vkrQueueId_COUNT] = { 0 };
    for (i32 id = 0; id < NELEM(ctx->curCmdBuf); ++id)
    {
        vkrCmdBuf* cmd = &ctx->curCmdBuf[id];
        ASSERT(!cmd->inRenderPass);
        if (cmd->queueTransferSrc)
        {
            ASSERT(!cmd->queueTransferDst);
            vkrCmdSubmit(cmd, NULL, 0, NULL);
            ASSERT(!cmd->queueTransferSrc);
            submit[id] = true;
        }
    }
    for (i32 id = 0; id < NELEM(ctx->curCmdBuf); ++id)
    {
        vkrCmdBuf* cmd = &ctx->curCmdBuf[id];
        cmd->queueTransferDst = 0;
        if (submit[id])
        {
            vkrCmdGet(id); // reinitialize cmdbuf
        }
    }

    ProfileEnd(pm_cmdflushqueuetransfers);
}

void vkrCmdBeginRenderPass(
    vkrCmdBuf* cmdbuf,
    VkRenderPass pass,
    VkFramebuffer framebuf,
    VkRect2D rect,
    i32 clearCount,
    const VkClearValue* clearValues)
{
    ASSERT(cmdbuf && cmdbuf->handle);
    ASSERT(pass);
    ASSERT(framebuf);
    ASSERT(!cmdbuf->inRenderPass);
    const VkRenderPassBeginInfo info =
    {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = pass,
        .framebuffer = framebuf,
        .renderArea = rect,
        .clearValueCount = clearCount,
        .pClearValues = clearValues,
    };
    vkCmdBeginRenderPass(cmdbuf->handle, &info, VK_SUBPASS_CONTENTS_INLINE);
    cmdbuf->inRenderPass = 1;
    cmdbuf->subpass = 0;
}

void vkrCmdNextSubpass(vkrCmdBuf* cmdbuf, VkSubpassContents contents)
{
    ASSERT(cmdbuf && cmdbuf->handle);
    ASSERT(cmdbuf->inRenderPass);
    vkCmdNextSubpass(cmdbuf->handle, contents);
    cmdbuf->subpass++;
}

void vkrCmdEndRenderPass(vkrCmdBuf* cmdbuf)
{
    ASSERT(cmdbuf && cmdbuf->handle);
    ASSERT(cmdbuf->inRenderPass);
    vkCmdEndRenderPass(cmdbuf->handle);
    cmdbuf->inRenderPass = 0;
    cmdbuf->subpass = 0;
}

void vkrCmdViewport(
    vkrCmdBuf* cmdbuf,
    VkViewport viewport,
    VkRect2D scissor)
{
    ASSERT(cmdbuf && cmdbuf->handle);
    vkCmdSetViewport(cmdbuf->handle, 0, 1, &viewport);
    vkCmdSetScissor(cmdbuf->handle, 0, 1, &scissor);
}

void vkrCmdDefaultViewport(vkrCmdBuf* cmdbuf)
{
    vkrCmdViewport(cmdbuf, vkrSwapchain_GetViewport(), vkrSwapchain_GetRect());
}

void vkrCmdDraw(vkrCmdBuf* cmdbuf, i32 vertexCount, i32 firstVertex)
{
    ASSERT(cmdbuf && cmdbuf->handle);
    ASSERT(vertexCount > 0);
    ASSERT(firstVertex >= 0);
    ASSERT(cmdbuf->inRenderPass);
    vkCmdDraw(cmdbuf->handle, vertexCount, 1, firstVertex, 0);
}

void vkrCmdTouchBuffer(vkrCmdBuf* cmdbuf, vkrBuffer* buf)
{
    ASSERT(cmdbuf->handle);
    ASSERT(buf->handle);
    buf->state.cmdId = cmdbuf->id;
}

void vkrCmdTouchImage(vkrCmdBuf* cmdbuf, vkrImage* img)
{
    ASSERT(cmdbuf->handle);
    ASSERT(img->handle);
    img->state.cmdId = cmdbuf->id;
}

vkrSubmitId vkrBuffer_GetSubmit(const vkrBuffer* buf)
{
    vkrSubmitId id = { 0 };
    if (buf->handle)
    {
        if (buf->state.stage != 0)
        {
            id.counter = buf->state.cmdId;
            id.queueId = buf->state.owner;
            id.valid = 1;
        }
        else
        {
            id = vkrGetHeadSubmit(buf->state.owner);
        }
        ASSERT(id.valid);
    }
    return id;
}

vkrSubmitId vkrImage_GetSubmit(const vkrImage* img)
{
    vkrSubmitId id = { 0 };
    if (img->handle)
    {
        ASSERT(!img->state.substates);
        if (img->state.stage != 0)
        {
            id.counter = img->state.cmdId;
            id.queueId = img->state.owner;
            id.valid = 1;
        }
        else
        {
            id = vkrGetHeadSubmit(img->state.owner);
        }
        ASSERT(id.valid);
    }
    return id;
}

bool vkrBufferState(
    vkrCmdBuf* cmdbuf,
    vkrBuffer* buf,
    const vkrBufferState_t* next)
{
    ASSERT(buf->handle);
    ASSERT(next->access != 0);
    ASSERT(next->stage != 0);

    bool insertedBarrier = false;
    vkrBufferState_t* prev = &buf->state;
    const vkrQueue* prevQueue = vkrGetQueue(prev->owner);
    const vkrQueue* nextQueue = vkrGetQueue(next->owner);
    ASSERT((next->stage & nextQueue->stageMask) == next->stage);
    ASSERT((next->access & nextQueue->accessMask) == next->access);

    bool newResource = false;
    if (prev->stage == 0)
    {
        prev->stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        newResource = true;
    }
    if (prevQueue != nextQueue)
    {
        // queue ownership transfer
        vkrCmdBuf* srcCmd = vkrCmdGet(prev->owner);
        vkrCmdBuf* dstCmd = vkrCmdGet(next->owner);
        ASSERT(!srcCmd->inRenderPass);
        ASSERT(!dstCmd->inRenderPass);
        if (srcCmd->queueTransferDst || dstCmd->queueTransferSrc)
        {
            vkrCmdFlushQueueTransfers();
        }
        const VkBufferMemoryBarrier barrier =
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask = prev->access,
            .dstAccessMask = next->access,
            .srcQueueFamilyIndex = prevQueue->family,
            .dstQueueFamilyIndex = nextQueue->family,
            .buffer = buf->handle,
            .offset = 0,
            .size = VK_WHOLE_SIZE,
        };
        ASSERT(!srcCmd->queueTransferDst);
        srcCmd->queueTransferSrc = 1;
        vkCmdPipelineBarrier(
            srcCmd->handle,
            prev->stage, next->stage,
            0x0,
            0, NULL,
            1, &barrier,
            0, NULL);
        ASSERT(!dstCmd->queueTransferSrc);
        dstCmd->queueTransferDst = 1;
        vkCmdPipelineBarrier(
            dstCmd->handle,
            prev->stage, next->stage,
            0x0,
            0, NULL,
            1, &barrier,
            0, NULL);
        *prev = *next;
        insertedBarrier = true;
    }
    else
    {
        bool needBarrier = false;
        bool hostOnly = prev->stage == VK_PIPELINE_STAGE_HOST_BIT && next->stage == VK_PIPELINE_STAGE_HOST_BIT;
        if (!hostOnly)
        {
            bool srcRead = (prev->access & kReadAccess) != 0;
            bool srcWrite = (prev->access & kWriteAccess) != 0;
            bool dstRead = (next->access & kReadAccess) != 0;
            bool dstWrite = (next->access & kWriteAccess) != 0;
            bool readAfterWrite = srcWrite && dstRead;
            bool writeAfterRead = srcRead && dstWrite;
            bool writeAfterWrite = srcWrite && dstWrite;
            needBarrier = readAfterWrite || writeAfterRead || writeAfterWrite || newResource;
        }
        if (needBarrier)
        {
            // data hazard, insert barrier
            vkrCmdBuf* cmd = vkrCmdGet(next->owner);
            ASSERT(!cmd->inRenderPass);
            const VkBufferMemoryBarrier barrier =
            {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .srcAccessMask = prev->access,
                .dstAccessMask = next->access,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .buffer = buf->handle,
                .offset = 0,
                .size = VK_WHOLE_SIZE,
            };
            vkCmdPipelineBarrier(
                cmd->handle,
                prev->stage, next->stage,
                0x0,
                0, NULL,
                1, &barrier,
                0, NULL);
            *prev = *next;
            insertedBarrier = true;
        }
        else
        {
            // no data hazard, append usage state
            prev->stage |= next->stage;
            prev->access |= next->access;
        }
    }

    vkrCmdTouchBuffer(vkrCmdGet(next->owner), buf);

    return insertedBarrier;
}

bool vkrBufferState_HostRead(vkrCmdBuf* cmdbuf, vkrBuffer* buf)
{
    ASSERT(cmdbuf->handle);
    ASSERT(cmdbuf->xfer);
    const vkrBufferState_t state =
    {
        .owner = cmdbuf->queueId,
        .stage = VK_PIPELINE_STAGE_HOST_BIT,
        .access = VK_ACCESS_HOST_READ_BIT,
    };
    return vkrBufferState(cmdbuf, buf, &state);
}

bool vkrBufferState_HostWrite(vkrCmdBuf* cmdbuf, vkrBuffer* buf)
{
    ASSERT(cmdbuf->handle);
    ASSERT(cmdbuf->xfer);
    const vkrBufferState_t state =
    {
        .owner = cmdbuf->queueId,
        .stage = VK_PIPELINE_STAGE_HOST_BIT,
        .access = VK_ACCESS_HOST_WRITE_BIT,
    };
    return vkrBufferState(cmdbuf, buf, &state);
}

bool vkrBufferState_TransferSrc(vkrCmdBuf* cmdbuf, vkrBuffer* buf)
{
    ASSERT(cmdbuf->handle);
    ASSERT(cmdbuf->xfer);
    const vkrBufferState_t state =
    {
        .owner = cmdbuf->queueId,
        .stage = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .access = VK_ACCESS_TRANSFER_READ_BIT,
    };
    return vkrBufferState(cmdbuf, buf, &state);
}

bool vkrBufferState_TransferDst(vkrCmdBuf* cmdbuf, vkrBuffer* buf)
{
    ASSERT(cmdbuf->handle);
    ASSERT(cmdbuf->xfer);
    const vkrBufferState_t state =
    {
        .owner = cmdbuf->queueId,
        .stage = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .access = VK_ACCESS_TRANSFER_WRITE_BIT,
    };
    return vkrBufferState(cmdbuf, buf, &state);
}

bool vkrBufferState_UniformBuffer(vkrCmdBuf* cmdbuf, vkrBuffer* buf)
{
    ASSERT(cmdbuf->handle);
    ASSERT(cmdbuf->gfx || cmdbuf->comp);
    vkrBufferState_t state =
    {
        .owner = cmdbuf->queueId,
        .stage =
            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .access = VK_ACCESS_UNIFORM_READ_BIT,
    };
    return vkrBufferState(cmdbuf, buf, &state);
}

bool vkrBufferState_IndirectDraw(vkrCmdBuf* cmdbuf, vkrBuffer* buf)
{
    ASSERT(cmdbuf->handle);
    ASSERT(cmdbuf->gfx);
    const vkrBufferState_t state =
    {
        .owner = cmdbuf->queueId,
        .stage = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
        .access = VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
    };
    return vkrBufferState(cmdbuf, buf, &state);
}

bool vkrBufferState_IndirectDispatch(vkrCmdBuf* cmdbuf, vkrBuffer* buf)
{
    ASSERT(cmdbuf->handle);
    ASSERT(cmdbuf->comp);
    const vkrBufferState_t state =
    {
        .owner = cmdbuf->queueId,
        // not sure if draw indirect stage needed here or not
        .stage = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .access = VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
    };
    return vkrBufferState(cmdbuf, buf, &state);
}

bool vkrBufferState_VertexBuffer(vkrCmdBuf* cmdbuf, vkrBuffer* buf)
{
    ASSERT(cmdbuf->handle);
    ASSERT(cmdbuf->gfx);
    const vkrBufferState_t state =
    {
        .owner = cmdbuf->queueId,
        .stage = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
        .access = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
    };
    return vkrBufferState(cmdbuf, buf, &state);
}

bool vkrBufferState_IndexBuffer(vkrCmdBuf* cmdbuf, vkrBuffer* buf)
{
    ASSERT(cmdbuf->handle);
    ASSERT(cmdbuf->gfx);
    const vkrBufferState_t state =
    {
        .owner = cmdbuf->queueId,
        .stage = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
        .access = VK_ACCESS_INDEX_READ_BIT,
    };
    return vkrBufferState(cmdbuf, buf, &state);
}

bool vkrBufferState_FragLoad(vkrCmdBuf* cmdbuf, vkrBuffer* buf)
{
    ASSERT(cmdbuf->handle);
    ASSERT(cmdbuf->gfx);
    const vkrBufferState_t state =
    {
        .owner = cmdbuf->queueId,
        .stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .access = VK_ACCESS_SHADER_READ_BIT,
    };
    return vkrBufferState(cmdbuf, buf, &state);
}

bool vkrBufferState_FragStore(vkrCmdBuf* cmdbuf, vkrBuffer* buf)
{
    ASSERT(cmdbuf->handle);
    ASSERT(cmdbuf->gfx);
    const vkrBufferState_t state =
    {
        .owner = cmdbuf->queueId,
        .stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .access = VK_ACCESS_SHADER_WRITE_BIT,
    };
    return vkrBufferState(cmdbuf, buf, &state);
}

bool vkrBufferState_FragLoadStore(vkrCmdBuf* cmdbuf, vkrBuffer* buf)
{
    ASSERT(cmdbuf->handle);
    ASSERT(cmdbuf->gfx);
    const vkrBufferState_t state =
    {
        .owner = cmdbuf->queueId,
        .stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
    };
    return vkrBufferState(cmdbuf, buf, &state);
}

bool vkrBufferState_ComputeLoad(vkrCmdBuf* cmdbuf, vkrBuffer* buf)
{
    ASSERT(cmdbuf->handle);
    ASSERT(cmdbuf->comp);
    const vkrBufferState_t state =
    {
        .owner = cmdbuf->queueId,
        .stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .access = VK_ACCESS_SHADER_READ_BIT,
    };
    return vkrBufferState(cmdbuf, buf, &state);
}

bool vkrBufferState_ComputeStore(vkrCmdBuf* cmdbuf, vkrBuffer* buf)
{
    ASSERT(cmdbuf->handle);
    ASSERT(cmdbuf->comp);
    const vkrBufferState_t state =
    {
        .owner = cmdbuf->queueId,
        .stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .access = VK_ACCESS_SHADER_WRITE_BIT,
    };
    return vkrBufferState(cmdbuf, buf, &state);
}

bool vkrBufferState_ComputeLoadStore(vkrCmdBuf* cmdbuf, vkrBuffer* buf)
{
    ASSERT(cmdbuf->handle);
    ASSERT(cmdbuf->comp);
    const vkrBufferState_t state =
    {
        .owner = cmdbuf->queueId,
        .stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
    };
    return vkrBufferState(cmdbuf, buf, &state);
}

bool vkrImageState(
    vkrCmdBuf* cmdbuf,
    vkrImage* img,
    const vkrImageState_t* next)
{
    ASSERT(img->handle);
    ASSERT(next->access != 0);
    ASSERT(next->stage != 0);

    bool insertedBarrier = false;
    vkrImageState_t* prev = &img->state;
    const vkrQueue* prevQueue = vkrGetQueue(prev->owner);
    const vkrQueue* nextQueue = vkrGetQueue(next->owner);
    ASSERT((next->stage & nextQueue->stageMask) == next->stage);
    ASSERT((next->access & nextQueue->accessMask) == next->access);

    if (prev->substates)
    {
        ASSERT(prev->owner == next->owner);
        vkrSubImageState_t subnext = { 0 };
        subnext.stage = next->stage;
        subnext.access = next->access;
        subnext.layout = next->layout;
        insertedBarrier = vkrSubImageState(cmdbuf, img, &subnext, 0, img->arrayLayers, 0, img->mipLevels);
        ASSERT(!img->state.substates);
        return insertedBarrier;
    }

    bool newResource = false;
    if (prev->stage == 0)
    {
        prev->stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        newResource = true;
    }
    VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    if (img->usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
        aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    if (prevQueue != nextQueue)
    {
        // queue ownership transfer
        vkrCmdBuf* srcCmd = vkrCmdGet(prev->owner);
        vkrCmdBuf* dstCmd = vkrCmdGet(next->owner);
        if (srcCmd->queueTransferDst || dstCmd->queueTransferSrc)
        {
            vkrCmdFlushQueueTransfers();
        }
        const VkImageMemoryBarrier barrier =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = prev->access,
            .dstAccessMask = next->access,
            .oldLayout = prev->layout,
            .newLayout = next->layout,
            .srcQueueFamilyIndex = prevQueue->family,
            .dstQueueFamilyIndex = nextQueue->family,
            .image = img->handle,
            .subresourceRange =
            {
                .aspectMask = aspect,
                .baseMipLevel = 0,
                .levelCount = VK_REMAINING_MIP_LEVELS,
                .baseArrayLayer = 0,
                .layerCount = VK_REMAINING_ARRAY_LAYERS,
            },
        };
        ASSERT(!srcCmd->queueTransferDst);
        srcCmd->queueTransferSrc = 1;
        vkCmdPipelineBarrier(
            srcCmd->handle,
            prev->stage, next->stage,
            0x0,
            0, NULL,
            0, NULL,
            1, &barrier);
        ASSERT(!dstCmd->queueTransferSrc);
        dstCmd->queueTransferDst = 1;
        vkCmdPipelineBarrier(
            dstCmd->handle,
            prev->stage, next->stage,
            0x0,
            0, NULL,
            0, NULL,
            1, &barrier);
        *prev = *next;
        insertedBarrier = true;
    }
    else
    {
        bool layoutChange = prev->layout != next->layout;
        bool srcRead = (prev->access & kReadAccess) != 0;
        bool srcWrite = (prev->access & kWriteAccess) != 0;
        bool dstRead = (next->access & kReadAccess) != 0;
        bool dstWrite = (next->access & kWriteAccess) != 0;
        bool readAfterWrite = srcWrite && dstRead;
        bool writeAfterRead = srcRead && dstWrite;
        bool writeAfterWrite = srcWrite && dstWrite;
        if (layoutChange || readAfterWrite || writeAfterRead || writeAfterWrite || newResource)
        {
            // data hazard, insert barrier
            vkrCmdBuf* cmd = vkrCmdGet(next->owner);
            const VkImageMemoryBarrier barrier =
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = prev->access,
                .dstAccessMask = next->access,
                .oldLayout = prev->layout,
                .newLayout = next->layout,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = img->handle,
                .subresourceRange =
                {
                    .aspectMask = aspect,
                    .baseMipLevel = 0,
                    .levelCount = VK_REMAINING_MIP_LEVELS,
                    .baseArrayLayer = 0,
                    .layerCount = VK_REMAINING_ARRAY_LAYERS,
                },
            };
            vkCmdPipelineBarrier(
                cmd->handle,
                prev->stage, next->stage,
                0x0,
                0, NULL,
                0, NULL,
                1, &barrier);
            *prev = *next;
            insertedBarrier = true;
        }
        else
        {
            // no data hazard, append usage state
            prev->stage |= next->stage;
            prev->access |= next->access;
        }
    }

    vkrCmdTouchImage(vkrCmdGet(next->owner), img);
    ASSERT(img->state.layout == next->layout);

    return insertedBarrier;
}

bool vkrImageState_TransferSrc(vkrCmdBuf* cmdbuf, vkrImage* img)
{
    ASSERT(cmdbuf->xfer);
    const vkrImageState_t state =
    {
        .owner = cmdbuf->queueId,
        .stage = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .access = VK_ACCESS_TRANSFER_READ_BIT,
        .layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    };
    return vkrImageState(cmdbuf, img, &state);
}

bool vkrImageState_TransferDst(vkrCmdBuf* cmdbuf, vkrImage* img)
{
    ASSERT(cmdbuf->xfer);
    const vkrImageState_t state =
    {
        .owner = cmdbuf->queueId,
        .stage = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .access = VK_ACCESS_TRANSFER_WRITE_BIT,
        .layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    };
    return vkrImageState(cmdbuf, img, &state);
}

bool vkrImageState_FragSample(vkrCmdBuf* cmdbuf, vkrImage* img)
{
    ASSERT(cmdbuf->gfx);
    const vkrImageState_t state =
    {
        .owner = cmdbuf->queueId,
        .stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .access = VK_ACCESS_SHADER_READ_BIT,
        .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    return vkrImageState(cmdbuf, img, &state);
}

bool vkrImageState_ComputeSample(vkrCmdBuf* cmdbuf, vkrImage* img)
{
    ASSERT(cmdbuf->comp);
    const vkrImageState_t state =
    {
        .owner = cmdbuf->queueId,
        .stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .access = VK_ACCESS_SHADER_READ_BIT,
        .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    return vkrImageState(cmdbuf, img, &state);
}

bool vkrImageState_FragLoad(vkrCmdBuf* cmdbuf, vkrImage* img)
{
    ASSERT(cmdbuf->gfx);
    const vkrImageState_t state =
    {
        .owner = cmdbuf->queueId,
        .stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .access = VK_ACCESS_SHADER_READ_BIT,
        .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    return vkrImageState(cmdbuf, img, &state);
}

bool vkrImageState_FragStore(vkrCmdBuf* cmdbuf, vkrImage* img)
{
    ASSERT(cmdbuf->gfx);
    const vkrImageState_t state =
    {
        .owner = cmdbuf->queueId,
        .stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .access = VK_ACCESS_SHADER_WRITE_BIT,
        .layout = VK_IMAGE_LAYOUT_GENERAL,
    };
    return vkrImageState(cmdbuf, img, &state);
}

bool vkrImageState_FragLoadStore(vkrCmdBuf* cmdbuf, vkrImage* img)
{
    ASSERT(cmdbuf->gfx);
    const vkrImageState_t state =
    {
        .owner = cmdbuf->queueId,
        .stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
        .layout = VK_IMAGE_LAYOUT_GENERAL,
    };
    return vkrImageState(cmdbuf, img, &state);
}

bool vkrImageState_ComputeLoad(vkrCmdBuf* cmdbuf, vkrImage* img)
{
    ASSERT(cmdbuf->comp);
    const vkrImageState_t state =
    {
        .owner = cmdbuf->queueId,
        .stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .access = VK_ACCESS_SHADER_READ_BIT,
        .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    return vkrImageState(cmdbuf, img, &state);
}

bool vkrImageState_ComputeStore(vkrCmdBuf* cmdbuf, vkrImage* img)
{
    ASSERT(cmdbuf->comp);
    const vkrImageState_t state =
    {
        .owner = cmdbuf->queueId,
        .stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .access = VK_ACCESS_SHADER_WRITE_BIT,
        .layout = VK_IMAGE_LAYOUT_GENERAL,
    };
    return vkrImageState(cmdbuf, img, &state);
}

bool vkrImageState_ComputeLoadStore(vkrCmdBuf* cmdbuf, vkrImage* img)
{
    ASSERT(cmdbuf->comp);
    const vkrImageState_t state =
    {
        .owner = cmdbuf->queueId,
        .stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
        .layout = VK_IMAGE_LAYOUT_GENERAL,
    };
    return vkrImageState(cmdbuf, img, &state);
}

bool vkrImageState_ColorAttachWrite(vkrCmdBuf* cmdbuf, vkrImage* img)
{
    ASSERT(cmdbuf->gfx);
    const vkrImageState_t state =
    {
        .owner = cmdbuf->queueId,
        .stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    return vkrImageState(cmdbuf, img, &state);
}

bool vkrImageState_DepthAttachWrite(vkrCmdBuf* cmdbuf, vkrImage* img)
{
    ASSERT(cmdbuf->gfx);
    const vkrImageState_t state =
    {
        .owner = cmdbuf->queueId,
        .stage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };
    return vkrImageState(cmdbuf, img, &state);
}

bool vkrImageState_PresentSrc(vkrCmdBuf* cmdbuf, vkrImage* img)
{
    ASSERT(cmdbuf->pres);
    const vkrImageState_t state =
    {
        .owner = cmdbuf->queueId,
        .stage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
        .access = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };
    return vkrImageState(cmdbuf, img, &state);
}

bool vkrSubImageState(
    vkrCmdBuf* cmd,
    vkrImage* img,
    const vkrSubImageState_t* next,
    i32 baseLayer, i32 layerCount,
    i32 baseMip, i32 mipCount)
{
    {
        ASSERT(img->handle);
        ASSERT(next->access != 0);
        ASSERT(next->stage != 0);
        ASSERT(layerCount > 0);
        ASSERT(mipCount > 0);
        ASSERT(cmd == vkrCmdGet(img->state.owner));
        ASSERT(cmd->handle);
        ASSERT_ONLY(const vkrQueue* queue = vkrGetQueue(cmd->queueId));
        ASSERT((next->stage & queue->stageMask) == next->stage);
        ASSERT((next->access & queue->accessMask) == next->access);
    }

    bool insertedBarrier = false;
    const i32 L = img->arrayLayers;
    const i32 M = img->mipLevels;
    ASSERT(baseLayer + layerCount <= L);
    ASSERT(baseMip + mipCount <= M);
    const i32 substatecount = L * M;

    vkrImageState_t* prev = &img->state;
    vkrSubImageState_t* substates = prev->substates;
    if (!substates)
    {
        substates = Perm_Calloc(sizeof(prev->substates[0]) * substatecount);
        prev->substates = substates;
        if (!prev->stage)
        {
            prev->stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        }
        for (i32 i = 0; i < substatecount; ++i)
        {
            substates[i].stage = prev->stage;
            substates[i].access = prev->access;
            substates[i].layout = prev->layout;
        }
    }
    else
    {
        ASSERT(prev->layout == VK_IMAGE_LAYOUT_UNDEFINED);
        ASSERT(!prev->access);
        ASSERT(!prev->stage);
    }

    VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    if (img->usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
        aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    const i32 maxBarriers = layerCount * mipCount;
    VkImageMemoryBarrier* barriers = Perm_Calloc(sizeof(barriers[0]) * maxBarriers);

    prev->layout = VK_IMAGE_LAYOUT_UNDEFINED;
    prev->access = 0;
    prev->stage = 0;

    i32 b = 0;
    for (i32 l = 0; l < layerCount; ++l)
    {
        i32 iLayer = baseLayer + l;
        for (i32 m = 0; m < mipCount; ++m)
        {
            i32 iMip = baseMip + m;
            vkrSubImageState_t* prevsub = &substates[iLayer * M + iMip];
            prev->stage |= prevsub->stage;
            prev->access |= prevsub->access;

            bool layoutChange = prevsub->layout != next->layout;
            bool srcRead = (prevsub->access & kReadAccess) != 0;
            bool srcWrite = (prevsub->access & kWriteAccess) != 0;
            bool dstRead = (next->access & kReadAccess) != 0;
            bool dstWrite = (next->access & kWriteAccess) != 0;
            bool readAfterWrite = srcWrite && dstRead;
            bool writeAfterRead = srcRead && dstWrite;
            bool writeAfterWrite = srcWrite && dstWrite;

            if (layoutChange || readAfterWrite || writeAfterRead || writeAfterWrite)
            {
                ASSERT(b < maxBarriers);
                VkImageMemoryBarrier* barrier = &barriers[b++];
                barrier->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier->srcAccessMask = prevsub->access;
                barrier->dstAccessMask = next->access;
                barrier->oldLayout = prevsub->layout;
                barrier->newLayout = next->layout;
                barrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier->image = img->handle;
                barrier->subresourceRange.aspectMask = aspect;
                barrier->subresourceRange.baseMipLevel = iMip;
                barrier->subresourceRange.levelCount = 1;
                barrier->subresourceRange.baseArrayLayer = iLayer;
                barrier->subresourceRange.layerCount = 1;
                *prevsub = *next;
            }
            else
            {
                prevsub->access |= next->access;
                prevsub->stage |= next->stage;
            }
        }
    }

    // merge mips
    for (i32 i = 0; (i + 1) < b; ++i)
    {
        VkImageMemoryBarrier* dst = &barriers[i];
        VkImageMemoryBarrier* src = &barriers[i + 1];
        VkImageSubresourceRange* dstRange = &dst->subresourceRange;
        VkImageSubresourceRange* srcRange = &src->subresourceRange;
        if (dst->oldLayout != src->oldLayout)
            continue;
        if (dstRange->baseArrayLayer != srcRange->baseArrayLayer)
            continue;

        if (dstRange->baseMipLevel + dstRange->levelCount == srcRange->baseMipLevel)
        {
            dstRange->levelCount += srcRange->levelCount;
            dst->srcAccessMask |= src->srcAccessMask;
            ASSERT(dstRange->levelCount <= img->mipLevels);
            for (i32 j = i + 1; (j + 1) < b; ++j)
            {
                barriers[j] = barriers[j + 1];
            }
            --b;
            ASSERT(b >= 1);
            --i;
        }
    }

    // merge layers
    for (i32 i = 0; (i + 1) < b; ++i)
    {
        VkImageMemoryBarrier* dst = &barriers[i];
        VkImageMemoryBarrier* src = &barriers[i + 1];
        VkImageSubresourceRange* dstRange = &dst->subresourceRange;
        VkImageSubresourceRange* srcRange = &src->subresourceRange;
        if (dst->oldLayout != src->oldLayout)
            continue;
        if (dstRange->baseMipLevel != srcRange->baseMipLevel)
            continue;
        if (dstRange->levelCount != srcRange->levelCount)
            continue;

        if (dstRange->baseArrayLayer + dstRange->layerCount == srcRange->baseArrayLayer)
        {
            dstRange->layerCount += srcRange->layerCount;
            dst->srcAccessMask |= src->srcAccessMask;
            ASSERT(dstRange->layerCount <= img->arrayLayers);
            for (i32 j = i + 1; (j + 1) < b; ++j)
            {
                barriers[j] = barriers[j + 1];
            }
            --b;
            ASSERT(b >= 1);
            --i;
        }
    }

    ASSERT(b >= 1);
    vkCmdPipelineBarrier(
        cmd->handle,
        prev->stage, next->stage,
        0x0,
        0, NULL,
        0, NULL,
        b, barriers);
    insertedBarrier = true;

    Mem_Free(barriers);
    barriers = NULL;
    b = 0;

    prev->access = substates[0].access;
    prev->stage = substates[0].stage;
    prev->layout = substates[0].layout;
    bool isUniform = true;
    for (i32 i = 1; i < substatecount; ++i)
    {
        if (substates[0].layout != substates[i].layout)
        {
            isUniform = false;
            break;
        }
        prev->access |= substates[i].access;
        prev->stage |= substates[i].stage;
    }
    if (isUniform)
    {
        prev->substates = NULL;
        Mem_Free(substates);
        substates = NULL;
    }
    else
    {
        prev->layout = VK_IMAGE_LAYOUT_UNDEFINED;
        prev->access = 0;
        prev->stage = 0;
    }

    vkrCmdTouchImage(cmd, img);

    return insertedBarrier;
}

bool vkrSubImageState_TransferSrc(
    vkrCmdBuf* cmdbuf, vkrImage* img,
    i32 baseLayer, i32 layerCount,
    i32 baseMip, i32 mipCount)
{
    ASSERT(cmdbuf->xfer);
    const vkrSubImageState_t state =
    {
        .stage = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .access = VK_ACCESS_TRANSFER_READ_BIT,
        .layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    };
    return vkrSubImageState(cmdbuf, img, &state, baseLayer, layerCount, baseMip, mipCount);
}

bool vkrSubImageState_TransferDst(
    vkrCmdBuf* cmdbuf, vkrImage* img,
    i32 baseLayer, i32 layerCount,
    i32 baseMip, i32 mipCount)
{
    ASSERT(cmdbuf->xfer);
    const vkrSubImageState_t state =
    {
        .stage = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .access = VK_ACCESS_TRANSFER_WRITE_BIT,
        .layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    };
    return vkrSubImageState(cmdbuf, img, &state, baseLayer, layerCount, baseMip, mipCount);
}

bool vkrSubImageState_FragSample(
    vkrCmdBuf* cmdbuf, vkrImage* img,
    i32 baseLayer, i32 layerCount,
    i32 baseMip, i32 mipCount)
{
    ASSERT(cmdbuf->gfx);
    const vkrSubImageState_t state =
    {
        .stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .access = VK_ACCESS_SHADER_READ_BIT,
        .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    return vkrSubImageState(cmdbuf, img, &state, baseLayer, layerCount, baseMip, mipCount);
}

bool vkrSubImageState_ComputeSample(
    vkrCmdBuf* cmdbuf, vkrImage* img,
    i32 baseLayer, i32 layerCount,
    i32 baseMip, i32 mipCount)
{
    ASSERT(cmdbuf->comp);
    const vkrSubImageState_t state =
    {
        .stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .access = VK_ACCESS_SHADER_READ_BIT,
        .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    return vkrSubImageState(cmdbuf, img, &state, baseLayer, layerCount, baseMip, mipCount);
}

bool vkrSubImageState_FragLoad(
    vkrCmdBuf* cmdbuf, vkrImage* img,
    i32 baseLayer, i32 layerCount,
    i32 baseMip, i32 mipCount)
{
    ASSERT(cmdbuf->gfx);
    const vkrSubImageState_t state =
    {
        .stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .access = VK_ACCESS_SHADER_READ_BIT,
        .layout = VK_IMAGE_LAYOUT_GENERAL,
    };
    return vkrSubImageState(cmdbuf, img, &state, baseLayer, layerCount, baseMip, mipCount);
}

bool vkrSubImageState_FragStore(
    vkrCmdBuf* cmdbuf, vkrImage* img,
    i32 baseLayer, i32 layerCount,
    i32 baseMip, i32 mipCount)
{
    ASSERT(cmdbuf->gfx);
    const vkrSubImageState_t state =
    {
        .stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .access = VK_ACCESS_SHADER_WRITE_BIT,
        .layout = VK_IMAGE_LAYOUT_GENERAL,
    };
    return vkrSubImageState(cmdbuf, img, &state, baseLayer, layerCount, baseMip, mipCount);
}

bool vkrSubImageState_FragLoadStore(
    vkrCmdBuf* cmdbuf, vkrImage* img,
    i32 baseLayer, i32 layerCount,
    i32 baseMip, i32 mipCount)
{
    ASSERT(cmdbuf->gfx);
    const vkrSubImageState_t state =
    {
        .stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
        .layout = VK_IMAGE_LAYOUT_GENERAL,
    };
    return vkrSubImageState(cmdbuf, img, &state, baseLayer, layerCount, baseMip, mipCount);
}

bool vkrSubImageState_ComputeLoad(
    vkrCmdBuf* cmdbuf, vkrImage* img,
    i32 baseLayer, i32 layerCount,
    i32 baseMip, i32 mipCount)
{
    ASSERT(cmdbuf->comp);
    const vkrSubImageState_t state =
    {
        .stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .access = VK_ACCESS_SHADER_READ_BIT,
        .layout = VK_IMAGE_LAYOUT_GENERAL,
    };
    return vkrSubImageState(cmdbuf, img, &state, baseLayer, layerCount, baseMip, mipCount);
}

bool vkrSubImageState_ComputeStore(
    vkrCmdBuf* cmdbuf, vkrImage* img,
    i32 baseLayer, i32 layerCount,
    i32 baseMip, i32 mipCount)
{
    ASSERT(cmdbuf->comp);
    const vkrSubImageState_t state =
    {
        .stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .access = VK_ACCESS_SHADER_WRITE_BIT,
        .layout = VK_IMAGE_LAYOUT_GENERAL,
    };
    return vkrSubImageState(cmdbuf, img, &state, baseLayer, layerCount, baseMip, mipCount);
}

bool vkrSubImageState_ComputeLoadStore(
    vkrCmdBuf* cmdbuf, vkrImage* img,
    i32 baseLayer, i32 layerCount,
    i32 baseMip, i32 mipCount)
{
    ASSERT(cmdbuf->comp);
    const vkrSubImageState_t state =
    {
        .stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
        .layout = VK_IMAGE_LAYOUT_GENERAL,
    };
    return vkrSubImageState(cmdbuf, img, &state, baseLayer, layerCount, baseMip, mipCount);
}

bool vkrSubImageState_ColorAttachWrite(
    vkrCmdBuf* cmdbuf, vkrImage* img,
    i32 baseLayer, i32 layerCount,
    i32 baseMip, i32 mipCount)
{
    ASSERT(cmdbuf->gfx);
    const vkrSubImageState_t state =
    {
        .stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    return vkrSubImageState(cmdbuf, img, &state, baseLayer, layerCount, baseMip, mipCount);
}

bool vkrSubImageState_DepthAttachWrite(
    vkrCmdBuf* cmdbuf, vkrImage* img,
    i32 baseLayer, i32 layerCount,
    i32 baseMip, i32 mipCount)
{
    ASSERT(cmdbuf->gfx);
    const vkrSubImageState_t state =
    {
        .stage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };
    return vkrSubImageState(cmdbuf, img, &state, baseLayer, layerCount, baseMip, mipCount);
}

void vkrCmdCopyBuffer(
    vkrCmdBuf* cmdbuf,
    vkrBuffer* src,
    vkrBuffer* dst)
{
    ASSERT(cmdbuf->handle);
    ASSERT(src->handle);
    ASSERT(dst->handle);
    i32 size = pim_min(src->size, dst->size);
    ASSERT(size >= 0);
    if (size > 0)
    {
        vkrBufferState_TransferSrc(cmdbuf, src);
        vkrBufferState_TransferDst(cmdbuf, dst);
        const VkBufferCopy region =
        {
            .srcOffset = 0,
            .dstOffset = 0,
            .size = size,
        };
        ASSERT(cmdbuf->handle);
        vkCmdCopyBuffer(cmdbuf->handle, src->handle, dst->handle, 1, &region);
    }
}

void vkrCmdCopyImage(
    vkrCmdBuf* cmdbuf,
    vkrImage* src,
    vkrImage* dst,
    const VkImageCopy* region)
{
    vkrSubImageState_TransferSrc(cmdbuf, src,
        region->srcSubresource.baseArrayLayer,
        region->srcSubresource.layerCount,
        region->srcSubresource.mipLevel,
        1);
    vkrSubImageState_TransferDst(cmdbuf, dst,
        region->dstSubresource.baseArrayLayer,
        region->dstSubresource.layerCount,
        region->dstSubresource.mipLevel,
        1);
    ASSERT(cmdbuf->handle);
    ASSERT(src->handle);
    ASSERT(dst->handle);
    ASSERT(region);
    vkCmdCopyImage(cmdbuf->handle, src->handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst->handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, region);
}

void vkrCmdBlitImage(
    vkrCmdBuf* cmdbuf,
    vkrImage* src,
    vkrImage* dst,
    const VkImageBlit* region)
{
    vkrSubImageState_TransferSrc(cmdbuf, src,
        region->srcSubresource.baseArrayLayer,
        region->srcSubresource.layerCount,
        region->srcSubresource.mipLevel,
        1);
    vkrSubImageState_TransferDst(cmdbuf, dst,
        region->dstSubresource.baseArrayLayer,
        region->dstSubresource.layerCount,
        region->dstSubresource.mipLevel,
        1);
    ASSERT(cmdbuf->handle);
    ASSERT(src->handle);
    ASSERT(dst->handle);
    vkCmdBlitImage(
        cmdbuf->handle,
        src->handle,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dst->handle,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, region,
        VK_FILTER_LINEAR);
}

void vkrCmdCopyBufferToImage(
    vkrCmdBuf* cmdbuf,
    vkrBuffer* src,
    vkrImage* dst,
    const VkBufferImageCopy* region)
{
    vkrBufferState_TransferSrc(cmdbuf, src);
    vkrSubImageState_TransferDst(cmdbuf, dst,
        region->imageSubresource.baseArrayLayer,
        region->imageSubresource.layerCount,
        region->imageSubresource.mipLevel,
        1);
    ASSERT(cmdbuf->handle);
    ASSERT(src->handle);
    ASSERT(dst->handle);
    vkCmdCopyBufferToImage(cmdbuf->handle, src->handle, dst->handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, region);
}

void vkrCmdCopyImageToBuffer(
    vkrCmdBuf* cmdbuf,
    vkrImage* src,
    vkrBuffer* dst,
    const VkBufferImageCopy* region)
{
    vkrSubImageState_TransferSrc(cmdbuf, src,
        region->imageSubresource.baseArrayLayer,
        region->imageSubresource.layerCount,
        region->imageSubresource.mipLevel,
        1);
    vkrBufferState_TransferDst(cmdbuf, dst);
    ASSERT(cmdbuf->handle);
    ASSERT(src->handle);
    ASSERT(dst->handle);
    vkCmdCopyImageToBuffer(cmdbuf->handle, src->handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst->handle, 1, region);
}

void vkrCmdUpdateBuffer(
    vkrCmdBuf* cmdbuf,
    const void* src,
    i32 srcSize,
    vkrBuffer* dst)
{
    const i32 kSizeLimit = 1 << 16;
    vkrBufferState_TransferDst(cmdbuf, dst);
    ASSERT(cmdbuf->handle);
    ASSERT(dst->handle);
    ASSERT((srcSize & 15) == 0);
    if (src && (u32)srcSize < (u32)kSizeLimit && srcSize == dst->size)
    {
        vkCmdUpdateBuffer(cmdbuf->handle, dst->handle, 0, srcSize, src);
    }
    else
    {
        ASSERT(false);
    }
}

void vkrCmdFillBuffer(
    vkrCmdBuf* cmdbuf,
    vkrBuffer* dst,
    u32 fillValue)
{
    vkrBufferState_TransferDst(cmdbuf, dst);
    ASSERT(cmdbuf->handle);
    ASSERT(dst->handle);
    vkCmdFillBuffer(cmdbuf->handle, dst->handle, 0, VK_WHOLE_SIZE, fillValue);
}

void vkrCmdBindDescSets(
    vkrCmdBuf* cmdbuf,
    VkPipelineBindPoint bindpoint,
    VkPipelineLayout layout,
    i32 setCount,
    const VkDescriptorSet* sets)
{
    ASSERT(cmdbuf && cmdbuf->handle);
    ASSERT(cmdbuf->gfx || cmdbuf->comp);
    ASSERT(layout);
    ASSERT(sets || !setCount);
    ASSERT(setCount >= 0);
    if (setCount > 0)
    {
        vkCmdBindDescriptorSets(
            cmdbuf->handle,
            bindpoint,
            layout,
            0,
            setCount, sets,
            0, NULL);
    }
}

void vkrCmdBindPass(
    vkrCmdBuf* cmdbuf,
    const vkrPass* pass)
{
    ASSERT(cmdbuf && cmdbuf->handle);
    ASSERT(cmdbuf->gfx || cmdbuf->comp);
    ASSERT(pass->pipeline);
    vkCmdBindPipeline(cmdbuf->handle, pass->bindpoint, pass->pipeline);
    VkDescriptorSet set = vkrBindings_GetSet();
    vkrCmdBindDescSets(cmdbuf, pass->bindpoint, pass->layout, 1, &set);
}

void vkrCmdPushConstants(
    vkrCmdBuf* cmdbuf,
    const vkrPass* pass,
    const void* src, i32 bytes)
{
    ASSERT(cmdbuf && cmdbuf->handle);
    ASSERT(cmdbuf->gfx || cmdbuf->comp);
    ASSERT(src);
    ASSERT(bytes == pass->pushConstantBytes);
    ASSERT(bytes > 0);
    ASSERT(pass->layout);
    ASSERT(pass->stageFlags);
    vkCmdPushConstants(
        cmdbuf->handle,
        pass->layout,
        pass->stageFlags,
        0,
        bytes,
        src);
}

void vkrCmdDispatch(vkrCmdBuf* cmdbuf, i32 x, i32 y, i32 z)
{
    ASSERT(cmdbuf && cmdbuf->handle);
    ASSERT(cmdbuf->comp);
    ASSERT(x > 0);
    ASSERT(y > 0);
    ASSERT(z > 0);
    vkCmdDispatch(cmdbuf->handle, x, y, z);
}
