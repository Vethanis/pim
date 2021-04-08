#include "rendering/vulkan/vkr_mesh.h"
#include "rendering/vulkan/vkr_mem.h"
#include "rendering/vulkan/vkr_buffer.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_context.h"
#include "containers/idalloc.h"
#include "allocator/allocator.h"
#include "common/profiler.h"
#include <string.h>

typedef struct vkrMesh_s
{
    vkrBuffer buffer;
    i32 vertCount;
} vkrMesh;

static IdAlloc ms_ids;
static vkrMesh* ms_meshes;

static vkrMeshId ToMeshId(GenId gid)
{
    vkrMeshId mid;
    mid.index = gid.index;
    mid.version = gid.version;
    return mid;
}

static GenId ToGenId(vkrMeshId mid)
{
    GenId gid;
    gid.index = mid.index;
    gid.version = mid.version;
    return gid;
}

bool vkrMeshSys_Init(void)
{
    IdAlloc_New(&ms_ids);
    return true;
}

void vkrMeshSys_Update(void)
{

}

void vkrMeshSys_Shutdown(void)
{
    const i32 len = ms_ids.length;
    for (i32 i = 0; i < len; ++i)
    {
        if (ms_ids.versions[i] & 1)
        {
            vkrBuffer_Release(&ms_meshes[i].buffer);
        }
    }
    IdAlloc_Del(&ms_ids);
    Mem_Free(ms_meshes); ms_meshes = NULL;
}

bool vkrMesh_Exists(vkrMeshId id)
{
    return IdAlloc_Exists(&ms_ids, ToGenId(id));
}

vkrMeshId vkrMesh_New(
    i32 vertCount,
    const float4* pim_noalias positions,
    const float4* pim_noalias normals,
    const float4* pim_noalias uv01,
    const int4* pim_noalias texIndices)
{
    vkrMeshId id = { 0 };
    bool success = true;
    if (vertCount <= 0)
    {
        ASSERT(false);
        return id;
    }
    if (!positions || !normals || !uv01)
    {
        ASSERT(false);
        return id;
    }

    SASSERT(sizeof(positions[0]) == sizeof(float4));
    SASSERT(sizeof(normals[0]) == sizeof(float4));
    SASSERT(sizeof(uv01[0]) == sizeof(float4));
    SASSERT(sizeof(texIndices[0]) == sizeof(float4));

    id = ToMeshId(IdAlloc_Alloc(&ms_ids));
    ms_meshes = Perm_Realloc(ms_meshes, sizeof(ms_meshes[0]) * ms_ids.length);
    vkrMesh* mesh = &ms_meshes[id.index];
    mesh->vertCount = vertCount;

    const i32 streamSize = sizeof(positions[0]) * vertCount;
    const i32 totalSize = streamSize * vkrMeshStream_COUNT;

    if (!vkrBuffer_New(
        &mesh->buffer,
        totalSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        vkrMemUsage_GpuOnly))
    {
        ASSERT(false);
        success = false;
        goto cleanup;
    }

    if (!vkrMesh_Upload(id, vertCount, positions, normals, uv01, texIndices))
    {
        ASSERT(false);
        success = false;
        goto cleanup;
    }

cleanup:
    if (!success)
    {
        vkrMesh_Del(id);
    }
    return id;
}

bool vkrMesh_Del(vkrMeshId id)
{
    if (IdAlloc_Free(&ms_ids, ToGenId(id)))
    {
        i32 slot = id.index;
        vkrBuffer_Release(&ms_meshes[slot].buffer);
        ms_meshes[slot].vertCount = 0;
        return true;
    }
    return false;
}

bool vkrMesh_Upload(
    vkrMeshId id,
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
    if (!vkrMesh_Exists(id))
    {
        ASSERT(false);
        return false;
    }

    SASSERT(sizeof(positions[0]) == sizeof(float4));
    SASSERT(sizeof(normals[0]) == sizeof(float4));
    SASSERT(sizeof(uv01[0]) == sizeof(float4));
    SASSERT(sizeof(texIndices[0]) == sizeof(float4));

    vkrBuffer stagebuf = { 0 };
    if (!vkrBuffer_New(
        &stagebuf,
        sizeof(float4) * vertCount * vkrMeshStream_COUNT,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        vkrMemUsage_CpuOnly))
    {
        ASSERT(false);
        return false;
    }

    {
        float4* pim_noalias dst = vkrBuffer_Map(&stagebuf);
        ASSERT(dst);
        if (dst)
        {
            memcpy(dst, positions, sizeof(dst[0]) * vertCount);
            dst += vertCount;
            memcpy(dst, normals, sizeof(dst[0]) * vertCount);
            dst += vertCount;
            memcpy(dst, uv01, sizeof(dst[0]) * vertCount);
            dst += vertCount;
            memcpy(dst, texIndices, sizeof(dst[0]) * vertCount);
            dst += vertCount;
        }
        vkrBuffer_Unmap(&stagebuf);
        vkrBuffer_Flush(&stagebuf);
    }

    vkrMesh* mesh = &ms_meshes[id.index];
    ASSERT(mesh->buffer.handle);
    mesh->vertCount = vertCount;

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

void vkrCmdDrawMesh(VkCommandBuffer cmdbuf, vkrMeshId id)
{
    ASSERT(cmdbuf);
    if (vkrMesh_Exists(id))
    {
        const vkrMesh* mesh = &ms_meshes[id.index];
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
}
