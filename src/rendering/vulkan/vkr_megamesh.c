#include "rendering/vulkan/vkr_megamesh.h"
#include "rendering/vulkan/vkr_buffer.h"
#include "rendering/vulkan/vkr_context.h"
#include "rendering/vulkan/vkr_cmd.h"

#include "rendering/mesh.h"
#include "allocator/allocator.h"
#include "containers/idalloc.h"
#include "common/profiler.h"

typedef struct SubMesh
{
    i32 offset;
    i32 length;
} SubMesh;

typedef struct MegaMesh
{
    i32 length;
    float4* pim_noalias positions;
    float4* pim_noalias normals;
    float4* pim_noalias uvs;
    int4* pim_noalias texIndices;
} MegaMesh;

i32 MegaMesh_Bytes(const MegaMesh* m)
{
    const i32 bytesPerVert =
        sizeof(m->positions[0]) +
        sizeof(m->normals[0]) +
        sizeof(m->uvs[0]) +
        sizeof(m->texIndices[0]);
    return m->length * bytesPerVert;
}

void MegaMesh_Resize(MegaMesh* m, i32 ct)
{
    ASSERT(ct >= 0);
    if (ct > m->length)
    {
        PermReserve(m->positions, ct);
        PermReserve(m->normals, ct);
        PermReserve(m->uvs, ct);
        PermReserve(m->texIndices, ct);
    }
    m->length = ct;
}

void MegaMesh_Clear(MegaMesh* m)
{
    m->length = 0;
}

void MegaMesh_Reset(MegaMesh* m)
{
    MegaMesh_Clear(m);
    FreePtr(m->positions);
    FreePtr(m->normals);
    FreePtr(m->uvs);
    FreePtr(m->texIndices);
}

bool MegaMesh_Fill(MegaMesh* m, vkrBuffer const *const buffer)
{
    ASSERT(buffer->size >= MegaMesh_Bytes(m));
    const i32 stride = m->length;
    float4* mem = vkrBuffer_Map(buffer);
    if (mem && stride > 0)
    {
        memcpy(mem, &m->positions[0], sizeof(m->positions[0]) * stride);
        mem += stride;
        memcpy(mem, &m->normals[0], sizeof(m->normals[0]) * stride);
        mem += stride;
        memcpy(mem, &m->uvs[0], sizeof(m->uvs[0]) * stride);
        mem += stride;
        memcpy(mem, &m->texIndices[0], sizeof(m->texIndices[0]) * stride);
        mem += stride;
    }
    vkrBuffer_Unmap(buffer);
    vkrBuffer_Flush(buffer);
    return mem != NULL;
}

i32 MegaMesh_Allocate(MegaMesh* m, i32 vertCount)
{
    i32 b = m->length;
    MegaMesh_Resize(m, b + vertCount);
    return b;
}

void MegaMesh_Free(MegaMesh* m, i32 offset, i32 length)
{
    i32 a = offset;
    i32 b = offset + length;
    i32 rem = m->length - b;
    rem = pim_max(0, rem);
    if (rem > 0)
    {
        memmove(m->positions + a, m->positions + b, sizeof(m->positions[0]) * rem);
        memmove(m->normals + a, m->normals + b, sizeof(m->normals[0]) * rem);
        memmove(m->uvs + a, m->uvs + b, sizeof(m->uvs[0]) * rem);
        memmove(m->texIndices + a, m->texIndices + b, sizeof(m->texIndices[0]) * rem);
    }
    MegaMesh_Resize(m, offset + rem);
}

static vkrBufferSet ms_buffer;
static vkrBufferSet ms_stage;
static idalloc_t ms_ids;
static SubMesh* ms_meshes;
static MegaMesh ms_mega;
static bool ms_dirty[kFramesInFlight];

PIM_C_BEGIN

static genid_t ToGenId(vkrMeshId id)
{
    genid_t gid = { 0 };
    gid.version = id.version;
    gid.index = id.index;
    return gid;
}

static vkrMeshId ToMeshId(genid_t gid)
{
    vkrMeshId id = { 0 };
    id.version = gid.version;
    id.index = gid.index;
    return id;
}

bool vkrMegaMesh_Init(void)
{
    return true;
}

ProfileMark(pm_update, vkrMegaMesh_Update)
void vkrMegaMesh_Update(void)
{
    ProfileBegin(pm_update);

    const u32 syncIndex = vkr_syncIndex();
    if (ms_dirty[syncIndex])
    {
        ms_dirty[syncIndex] = false;

        // TODO: partial uploads and stage buffer cache
        const i32 vertexBytes = MegaMesh_Bytes(&ms_mega);
        vkrBuffer *const buffer = vkrBufferSet_Current(&ms_buffer);
        vkrBuffer *const stage = vkrBufferSet_Current(&ms_stage);
        vkrBuffer_Reserve(
            stage,
            vertexBytes,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            vkrMemUsage_CpuOnly);
        vkrBuffer_Reserve(
            buffer,
            vertexBytes,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            vkrMemUsage_GpuOnly);

        MegaMesh_Fill(&ms_mega, stage);
        {
            VkFence fence = NULL;
            VkQueue queue = NULL;
            VkCommandBuffer cmd = vkrContext_GetTmpCmd(vkrQueueId_Gfx, &fence, &queue);
            vkrCmdBegin(cmd);
            vkrBuffer_Barrier(
                stage,
                cmd,
                VK_ACCESS_HOST_WRITE_BIT,
                VK_ACCESS_TRANSFER_READ_BIT,
                VK_ACCESS_HOST_WRITE_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT);
            vkrCmdCopyBuffer(cmd, *stage, *buffer);
            vkrBuffer_Barrier(
                buffer,
                cmd,
                VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_VERTEX_INPUT_BIT);
            vkrCmdEnd(cmd);
            vkrCmdSubmit(queue, cmd, fence, NULL, 0x0, NULL);
        }
    }

    ProfileEnd(pm_update);
}

void vkrMegaMesh_Shutdown(void)
{
    vkrBufferSet_Del(&ms_buffer);
    vkrBufferSet_Del(&ms_stage);

    idalloc_del(&ms_ids);
    FreePtr(ms_meshes);
    MegaMesh_Reset(&ms_mega);
}

bool vkrMegaMesh_Exists(vkrMeshId id)
{
    return idalloc_exists(&ms_ids, ToGenId(id));
}

vkrMeshId vkrMegaMesh_Alloc(i32 vertCount)
{
    genid_t gid = idalloc_alloc(&ms_ids);
    PermReserve(ms_meshes, ms_ids.length);
    SubMesh* mesh = &ms_meshes[gid.index];
    mesh->length = vertCount;
    mesh->offset = MegaMesh_Allocate(&ms_mega, vertCount);

    memset(ms_dirty, 0xff, sizeof(ms_dirty));

    return ToMeshId(gid);
}

bool vkrMegaMesh_Free(vkrMeshId id)
{
    if (idalloc_free(&ms_ids, ToGenId(id)))
    {
        SubMesh *const pim_noalias meshes = ms_meshes;
        i32 slot = id.index;
        i32 meshOffset = meshes[slot].offset;
        i32 meshLen = meshes[slot].length;
        MegaMesh_Free(&ms_mega, meshOffset, meshLen);
        const i32 len = ms_ids.length;
        for (i32 i = 0; i < len; ++i)
        {
            if (meshes[i].offset > meshOffset)
            {
                meshes[i].offset -= meshLen;
            }
        }
        memset(&meshes[slot], 0, sizeof(meshes[0]));
        memset(ms_dirty, 0xff, sizeof(ms_dirty));
        return true;
    }
    return false;
}

bool vkrMegaMesh_Set(
    vkrMeshId id,
    const float4* pim_noalias positions,
    const float4* pim_noalias normals,
    const float4* pim_noalias uvs,
    const int4* pim_noalias texIndices,
    i32 vertCount)
{
    if (vkrMegaMesh_Exists(id))
    {
        i32 slot = id.index;
        i32 offset = ms_meshes[slot].offset;
        i32 len = ms_meshes[slot].length;
        ASSERT(len == vertCount);
        if (len == vertCount)
        {
            SASSERT(sizeof(ms_mega.positions[0]) == sizeof(positions[0]));
            SASSERT(sizeof(ms_mega.normals[0]) == sizeof(normals[0]));
            SASSERT(sizeof(ms_mega.uvs[0]) == sizeof(uvs[0]));
            SASSERT(sizeof(ms_mega.texIndices[0]) == sizeof(texIndices[0]));

            memcpy(&ms_mega.positions[offset], positions, sizeof(positions[0]) * len);
            memcpy(&ms_mega.normals[offset], normals, sizeof(normals[0]) * len);
            memcpy(&ms_mega.uvs[offset], uvs, sizeof(uvs[0]) * len);
            memcpy(&ms_mega.texIndices[offset], texIndices, sizeof(texIndices[0]) * len);
            memset(ms_dirty, 0xff, sizeof(ms_dirty));
            return true;
        }
    }
    return false;
}

void vkrMegaMesh_Draw(VkCommandBuffer cmd)
{
    const i32 vertCount = ms_mega.length;
    if (vertCount > 0)
    {
        vkrBuffer *const buffer = vkrBufferSet_Current(&ms_buffer);

        const VkDeviceSize streamSize = sizeof(float4) * vertCount;
        const VkBuffer buffers[] =
        {
            buffer->handle,
            buffer->handle,
            buffer->handle,
            buffer->handle,
        };
        const VkDeviceSize offsets[] =
        {
            streamSize * 0,
            streamSize * 1,
            streamSize * 2,
            streamSize * 3,
        };
        vkCmdBindVertexBuffers(cmd, 0, NELEM(buffers), buffers, offsets);
        vkCmdDraw(cmd, vertCount, 1, 0, 0);
    }
}

void vkrMegaMesh_DrawPosition(VkCommandBuffer cmd)
{
    const i32 vertCount = ms_mega.length;
    if (vertCount > 0)
    {
        vkrBuffer *const buffer = vkrBufferSet_Current(&ms_buffer);

        const VkDeviceSize streamSize = sizeof(float4) * vertCount;
        const VkBuffer buffers[] =
        {
            buffer->handle,
        };
        const VkDeviceSize offsets[] =
        {
            streamSize * 0,
        };
        vkCmdBindVertexBuffers(cmd, 0, NELEM(buffers), buffers, offsets);
        vkCmdDraw(cmd, vertCount, 1, 0, 0);
    }
}

PIM_C_END
