#include "rendering/vulkan/vkr_mesh.h"
#include "rendering/vulkan/vkr_mem.h"
#include "rendering/vulkan/vkr_cmd.h"
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
        vkrMemUsage_CpuOnly))
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
        vkrMemUsage_GpuOnly))
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

    vkrCmdBuf* cmdbuf = vkrCmdGet(vkrQueueId_Xfer);
    vkrCmdBegin(cmdbuf);
    vkrCmdCopyBuffer(cmdbuf, stagebuf, devbuf);
    const VkBufferMemoryBarrier barrier =
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask =
            VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
            VK_ACCESS_INDEX_READ_BIT,
        .srcQueueFamilyIndex = g_vkr.queues[vkrQueueId_Xfer].family,
        .dstQueueFamilyIndex = g_vkr.queues[vkrQueueId_Gfx].family,
        .buffer = devbuf.handle,
        .offset = 0,
        .size = VK_WHOLE_SIZE,
    };
    vkrCmdBufferBarrier(
        cmdbuf,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
        &barrier);
    vkrCmdEnd(cmdbuf);
    vkrCmdSubmit2(cmdbuf);
    vkrBuffer_Release(&stagebuf, cmdbuf->fence);

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

        vkrBuffer_Release(&mesh->buffer, NULL);
        memset(mesh, 0, sizeof(*mesh));

        ProfileEnd(pm_meshdel);
    }
}
