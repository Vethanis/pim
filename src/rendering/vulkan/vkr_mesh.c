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
    const float4* pim_noalias tangents,
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
    SASSERT(sizeof(tangents[0]) == sizeof(float4));
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

    if (!vkrMesh_Upload(id, vertCount, positions, normals, tangents, uv01, texIndices))
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
    const float4* pim_noalias tangents,
    const float4* pim_noalias uv01,
    const int4* pim_noalias texIndices)
{
    if (vertCount <= 0)
    {
        ASSERT(false);
        return false;
    }
    if (!positions || !normals || !tangents || !uv01)
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
    SASSERT(sizeof(tangents[0]) == sizeof(float4));
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
        float4* pim_noalias dst = vkrBuffer_MapWrite(&stagebuf);
        ASSERT(dst);
        if (dst)
        {
            memcpy(dst, positions, sizeof(dst[0]) * vertCount);
            dst += vertCount;
            memcpy(dst, normals, sizeof(dst[0]) * vertCount);
            dst += vertCount;
            memcpy(dst, tangents, sizeof(dst[0]) * vertCount);
            dst += vertCount;
            memcpy(dst, uv01, sizeof(dst[0]) * vertCount);
            dst += vertCount;
            memcpy(dst, texIndices, sizeof(dst[0]) * vertCount);
            dst += vertCount;
        }
        vkrBuffer_UnmapWrite(&stagebuf);
    }

    vkrMesh* mesh = &ms_meshes[id.index];
    ASSERT(mesh->buffer.handle);
    mesh->vertCount = vertCount;

    vkrCmdBuf* cmd = vkrCmdGet_G();
    vkrCmdCopyBuffer(cmd, &stagebuf, &mesh->buffer);
    vkrBufferState_VertexBuffer(cmd, &mesh->buffer);
    vkrBuffer_Release(&stagebuf);

    return true;
}

void vkrCmdDrawMesh(vkrCmdBuf* cmdbuf, vkrMeshId id)
{
    ASSERT(cmdbuf && cmdbuf->handle);
    ASSERT(cmdbuf->gfx);
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
            vkCmdBindVertexBuffers(cmdbuf->handle, 0, NELEM(buffers), buffers, offsets);
            vkCmdDraw(cmdbuf->handle, mesh->vertCount, 1, 0, 0);
        }
    }
}

bool vkrMeshDesc_Alloc(vkrMeshDesc* desc)
{
    ASSERT(!desc->buffer);
    ASSERT(desc->indexCount || desc->vertexCount);
    desc->positionMin = (float3){ FLT_MAX, FLT_MAX , FLT_MAX };
    desc->positionMax = (float3){ -FLT_MAX, -FLT_MAX , -FLT_MAX };
    const u32 headerSize = sizeof(uint2) * vkrMeshStreamCount + sizeof(float4) + sizeof(float4);
    u32 size = headerSize;
    for (u32 s = 0; s < vkrMeshStreamCount && desc->strides[s]; ++s)
    {
        u32 elemCount = (desc->formats[s] == vkrStreamFormat_Index) ? desc->indexCount : desc->vertexCount;
        ASSERT(elemCount);
        u32 stride = desc->strides[s];
        u32 streamSize = (stride * elemCount + 15) & (~(u32)15);
        ASSERT(size < (size + streamSize));
        ASSERT(!(size & 15));
        desc->offsets[s] = size;
        size += streamSize;
    }
    desc->buffer = Perm_Calloc(size);
    desc->bufferSize = desc->buffer ? size : 0;
    return desc->buffer != NULL;
}

void vkrMeshDesc_Free(vkrMeshDesc* desc)
{
    if (desc->buffer)
    {
        Mem_Free(desc->buffer);
        desc->buffer = NULL;
    }
    desc->bufferSize = 0;
}

void vkrMeshDesc_SetRaw(vkrMeshDesc* desc, u32 s, u32 i, const void* value, u32 ASSERT_ONLY(bytes))
{
    ASSERT(s < vkrMeshStreamCount);
    ASSERT(i < desc->vertexCount);
    const u32 stride = desc->strides[s];
    ASSERT(desc->offsets[s]);
    ASSERT(desc->formats[s] == vkrStreamFormat_Raw);
    ASSERT(stride == bytes);
    u32 offset = desc->offsets[s] + stride * i;
    memcpy((u8*)desc->buffer + offset, value, stride);
}

void vkrMeshDesc_SetIndex(vkrMeshDesc* desc, u32 s, u32 i, u16 value)
{
    ASSERT(s < vkrMeshStreamCount);
    ASSERT(i < desc->indexCount);
    const u32 stride = sizeof(u16);
    ASSERT(desc->offsets[s]);
    ASSERT(desc->formats[s] == vkrStreamFormat_Index);
    ASSERT(desc->strides[s] == stride);
    u32 offset = desc->offsets[s] + stride * i;
    memcpy((u8*)desc->buffer + offset, &value, stride);
}

void VEC_CALL vkrMeshDesc_SetPosition(vkrMeshDesc* desc, u32 s, u32 i, float4 value)
{
    ASSERT(s < vkrMeshStreamCount);
    ASSERT(i < desc->vertexCount);
    const u32 stride = sizeof(uint4);
    ASSERT(desc->offsets[s]);
    ASSERT(desc->formats[s] == vkrStreamFormat_Position);
    ASSERT(desc->strides[s] == stride);
    u32 offset = desc->offsets[s] + stride * i;
    uint4 packed;
    memcpy((u8*)desc->buffer + offset, &packed, stride);
}

void VEC_CALL vkrMeshDesc_SetBasis(vkrMeshDesc* desc, u32 s, u32 i, float4 normal, float4 tangent)
{
    ASSERT(s < vkrMeshStreamCount);
    ASSERT(i < desc->vertexCount);
    const u32 stride = sizeof(uint4);
    ASSERT(desc->offsets[s]);
    ASSERT(desc->formats[s] == vkrStreamFormat_Basis);
    ASSERT(desc->strides[s] == stride);
    u32 offset = desc->offsets[s] + stride * i;
    uint4 packed;
    memcpy((u8*)desc->buffer + offset, &packed, stride);
}

void VEC_CALL vkrMeshDesc_SetUv(vkrMeshDesc* desc, u32 s, u32 i, float2 value)
{
    ASSERT(s < vkrMeshStreamCount);
    ASSERT(i < desc->vertexCount);
    const u32 stride = sizeof(u32);
    ASSERT(desc->offsets[s]);
    ASSERT(desc->formats[s] == vkrStreamFormat_Uv);
    ASSERT(desc->strides[s] == stride);
    u32 offset = desc->offsets[s] + stride * i;
    u32 packed;
    memcpy((u8*)desc->buffer + offset, &packed, stride);
}

void VEC_CALL vkrMeshDesc_SetColor(vkrMeshDesc* desc, u32 s, u32 i, float4 value)
{
    ASSERT(s < vkrMeshStreamCount);
    ASSERT(i < desc->vertexCount);
    const u32 stride = sizeof(u32);
    ASSERT(desc->offsets[s]);
    ASSERT(desc->formats[s] == vkrStreamFormat_Color);
    ASSERT(desc->strides[s] == stride);
    u32 offset = desc->offsets[s] + stride * i;
    u32 packed;
    memcpy((u8*)desc->buffer + offset, &packed, stride);
}
