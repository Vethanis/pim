#include "rendering/vulkan/vkr_mesh.h"
#include "rendering/vulkan/vkr_mem.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_context.h"
#include "allocator/allocator.h"
#include "common/profiler.h"
#include <string.h>

ProfileMark(pm_meshnew, vkrMesh_New)
bool vkrMesh_New(
    vkrMesh* mesh,
    i32 vertCount,
    const float4* pim_noalias positions,
    const float4* pim_noalias normals,
    const float4* pim_noalias uv01,
    i32 indexCount,
    const u16* pim_noalias indices)
{
    ASSERT(mesh);
    memset(mesh, 0, sizeof(*mesh));
    if (vertCount <= 0)
    {
        ASSERT(false);
        return false;
    }
    if ((indexCount % 3))
    {
        ASSERT(false);
        return false;
    }
    if (!positions || !normals || !uv01)
    {
        ASSERT(false);
        return false;
    }

    ProfileBegin(pm_meshnew);

    SASSERT(sizeof(positions[0]) == sizeof(normals[0]));
    SASSERT(sizeof(positions[0]) == sizeof(uv01[0]));

    const i32 streamSize = sizeof(positions[0]) * vertCount;
    const i32 indicesSize = sizeof(indices[0]) * indexCount;
    const i32 totalSize = streamSize * vkrMeshStream_COUNT + indicesSize;

    vkrBuffer stagebuf = { 0 };
    vkrBuffer devbuf = { 0 };

    if (!vkrBuffer_New(
        &stagebuf,
        totalSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        vkrMemUsage_CpuOnly, PIM_FILELINE))
    {
        ASSERT(false);
        goto cleanup;
    }
    if (!vkrBuffer_New(
        &devbuf,
        totalSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        vkrMemUsage_GpuOnly, PIM_FILELINE))
    {
        ASSERT(false);
        goto cleanup;
    }

    {
        u8* pim_noalias dst = vkrBuffer_Map(&stagebuf);
        ASSERT(dst);
        if (dst)
        {
            memcpy(dst, positions, streamSize);
            dst += streamSize;
            memcpy(dst, normals, streamSize);
            dst += streamSize;
            memcpy(dst, uv01, streamSize);
            dst += streamSize;
            if (indices)
            {
                memcpy(dst, indices, indicesSize);
                dst += indicesSize;
            }
        }
        vkrBuffer_Unmap(&stagebuf);
        vkrBuffer_Flush(&stagebuf);
    }

    vkrFrameContext* ctx = vkrContext_Get();
    VkCommandBuffer cmd = NULL;
    VkFence fence = NULL;
    VkQueue queue = NULL;
    vkrContext_GetCmd(ctx, vkrQueueId_Gfx, &cmd, &fence, &queue);
    vkrCmdBegin(cmd);
    vkrCmdCopyBuffer(cmd, stagebuf, devbuf);
    const VkBufferMemoryBarrier barrier =
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask =
            VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
            VK_ACCESS_INDEX_READ_BIT,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer = devbuf.handle,
        .offset = 0,
        .size = devbuf.size,
    };
    vkrCmdBufferBarrier(
        cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
        &barrier);
    vkrCmdEnd(cmd);
    vkrCmdSubmit(queue, cmd, fence, NULL, 0x0, NULL);
    vkrBuffer_Release(&stagebuf, fence);

    mesh->buffer = devbuf;
    mesh->vertCount = vertCount;
    mesh->indexCount = indexCount;

    ProfileEnd(pm_meshnew);
    return true;
cleanup:
    vkrBuffer_Del(&stagebuf);
    vkrBuffer_Del(&devbuf);
    ProfileEnd(pm_meshnew);
    return false;
}

ProfileMark(pm_meshdel, vkrMesh_Del)
void vkrMesh_Del(vkrMesh* mesh)
{
    if (mesh)
    {
        ProfileBegin(pm_meshdel);

        if (mesh->buffer.handle)
        {
            const VkBufferMemoryBarrier barrier =
            {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .srcAccessMask = 0x0,
                .dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .buffer = mesh->buffer.handle,
                .offset = 0,
                .size = VK_WHOLE_SIZE,
            };
            VkFence fence = vkrMem_Barrier(
                vkrQueueId_Gfx,
                VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                VK_PIPELINE_STAGE_HOST_BIT,
                NULL,
                &barrier,
                NULL);
            vkrBuffer_Release(&mesh->buffer, fence);
        }

        memset(mesh, 0, sizeof(*mesh));

        ProfileEnd(pm_meshdel);
    }
}
