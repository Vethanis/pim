#include "rendering/vulkan/vkr_mesh.h"
#include "rendering/vulkan/vkr_mem.h"
#include "rendering/vulkan/vkr_buffer.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_context.h"
#include "allocator/allocator.h"
#include "common/profiler.h"
#include <string.h>

bool vkrMesh_New(
    vkrMesh *const mesh,
    i32 vertCount,
    const float4* pim_noalias positions,
    const float4* pim_noalias normals,
    const float4* pim_noalias uv01,
    const int4* pim_noalias texIndices)
{
    ASSERT(mesh);
    memset(mesh, 0, sizeof(*mesh));
    if (vertCount <= 0)
    {
        ASSERT(false);
        return false;
    }
    if (!positions || !normals || !uv01)
    {
        ASSERT(false);
        return false;
    }

    SASSERT(sizeof(positions[0]) == sizeof(normals[0]));
    SASSERT(sizeof(positions[0]) == sizeof(uv01[0]));
    SASSERT(sizeof(positions[0]) == sizeof(texIndices[0]));

    const i32 streamSize = sizeof(positions[0]) * vertCount;
    const i32 totalSize = streamSize * vkrMeshStream_COUNT;

    if (!vkrBuffer_New(
        &mesh->buffer,
        totalSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        vkrMemUsage_GpuOnly))
    {
        ASSERT(false);
        return false;
    }

    mesh->vertCount = vertCount;
    if (!vkrMesh_Upload(mesh, vertCount, positions, normals, uv01, texIndices))
    {
        ASSERT(false);
        goto cleanup;
    }

    return true;
cleanup:
    vkrMesh_Del(mesh);
    return false;
}

void vkrMesh_Del(vkrMesh *const mesh)
{
    if (mesh)
    {
        vkrBuffer_Del(&mesh->buffer);
        memset(mesh, 0, sizeof(*mesh));
    }
}

void vkrMesh_Release(vkrMesh *const mesh)
{
    if (mesh)
    {
        vkrBuffer_Release(&mesh->buffer);
        memset(mesh, 0, sizeof(*mesh));
    }
}

bool vkrMesh_Upload(
    vkrMesh *const mesh,
    i32 vertCount,
    const float4* pim_noalias positions,
    const float4* pim_noalias normals,
    const float4* pim_noalias uv01,
    const int4* pim_noalias texIndices)
{
    if (vertCount <= 0)
    {
        ASSERT(false);
        return false;
    }
    if (!positions || !normals || !uv01)
    {
        ASSERT(false);
        return false;
    }

    SASSERT(sizeof(positions[0]) == sizeof(normals[0]));
    SASSERT(sizeof(positions[0]) == sizeof(uv01[0]));
    SASSERT(sizeof(positions[0]) == sizeof(texIndices[0]));

    const i32 streamSize = sizeof(positions[0]) * vertCount;
    const i32 totalSize = streamSize * vkrMeshStream_COUNT;

    vkrBuffer stagebuf = { 0 };
    if (!vkrBuffer_New(
        &stagebuf,
        totalSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        vkrMemUsage_CpuOnly))
    {
        ASSERT(false);
        return false;
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
            memcpy(dst, texIndices, streamSize);
            dst += streamSize;
        }
        vkrBuffer_Unmap(&stagebuf);
        vkrBuffer_Flush(&stagebuf);
    }

    VkFence fence = NULL;
    VkQueue queue = NULL;
    VkCommandBuffer cmd = vkrContext_GetTmpCmd(vkrQueueId_Gfx, &fence, &queue);
    vkrCmdBegin(cmd);
    vkrCmdCopyBuffer(cmd, &stagebuf, &mesh->buffer);
    vkrBuffer_Barrier(
        &mesh->buffer,
        cmd,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT);
    vkrCmdEnd(cmd);
    vkrCmdSubmit(queue, cmd, fence, NULL, 0x0, NULL);
    vkrBuffer_Release(&stagebuf);

    return true;
}
