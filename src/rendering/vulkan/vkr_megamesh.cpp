#include "rendering/vulkan/vkr_megamesh.h"
#include "rendering/vulkan/vkr_buffer.h"
#include "rendering/vulkan/vkr_context.h"
#include "rendering/vulkan/vkr_cmd.h"

#include "rendering/mesh.h"
#include "containers/array.hpp"
#include "containers/id_alloc.hpp"
#include "allocator/allocator.h"
#include "common/profiler.hpp"

struct SubMesh
{
    i32 offset;
    i32 length;
};

struct MegaMesh
{
    Array<float4> m_positions;
    Array<float4> m_normals;
    Array<float4> m_uvs;
    Array<int4> m_texIndices;

    i32 Size() const { return m_positions.Size(); }
    i32 GetBytes() const
    {
        constexpr i32 bytesPerVert =
            sizeof(m_positions[0]) +
            sizeof(m_normals[0]) +
            sizeof(m_uvs[0]) +
            sizeof(m_texIndices[0]);
        return Size() * bytesPerVert;
    }
    void Resize(i32 ct)
    {
        m_positions.Resize(ct);
        m_normals.Resize(ct);
        m_uvs.Resize(ct);
        m_texIndices.Resize(ct);
    }
    void Clear()
    {
        m_positions.Clear();
        m_normals.Clear();
        m_uvs.Clear();
        m_texIndices.Clear();
    }
    void Reset()
    {
        m_positions.Reset();
        m_normals.Reset();
        m_uvs.Reset();
        m_texIndices.Reset();
    }
    bool Fill(vkrBuffer const *const buffer)
    {
        ASSERT(buffer->size >= GetBytes());
        const i32 stride = Size();
        float4* mem = (float4*)vkrBuffer_Map(buffer);
        if (mem && stride > 0)
        {
            memcpy(mem, &m_positions[0], sizeof(m_positions[0]) * stride);
            mem += stride;
            memcpy(mem, &m_normals[0], sizeof(m_normals[0]) * stride);
            mem += stride;
            memcpy(mem, &m_uvs[0], sizeof(m_uvs[0]) * stride);
            mem += stride;
            memcpy(mem, &m_texIndices[0], sizeof(m_texIndices[0]) * stride);
            mem += stride;
        }
        vkrBuffer_Unmap(buffer);
        vkrBuffer_Flush(buffer);
        return mem != NULL;
    }
    i32 Allocate(i32 vertCount)
    {
        i32 b = Size();
        Resize(b + vertCount);
        return b;
    }
    void Free(i32 offset, i32 length)
    {
        i32 a = offset;
        i32 b = offset + length;
        i32 rem = Size() - b;
        rem = pim_max(0, rem);
        if (rem > 0)
        {
            float4* positions = m_positions.begin();
            float4* normals = m_normals.begin();
            float4* uvs = m_uvs.begin();
            int4* texIndices = m_texIndices.begin();
            memmove(positions + a, positions + b, sizeof(positions[0]) * rem);
            memmove(normals + a, normals + b, sizeof(normals[0]) * rem);
            memmove(uvs + a, uvs + b, sizeof(uvs[0]) * rem);
            memmove(texIndices + a, texIndices + b, sizeof(texIndices[0]) * rem);
        }
        Resize(offset + rem);
    }
};

static vkrBufferSet ms_buffer;
static vkrBufferSet ms_stage;
static IdAllocator ms_ids;
static Array<SubMesh> ms_meshes;
static MegaMesh ms_mega;
static bool ms_dirty[kFramesInFlight];

PIM_C_BEGIN

static GenId ToGenId(vkrMeshId id)
{
    GenId gid;
    gid.asint = id.asint;
    return gid;
}

static vkrMeshId ToMeshId(GenId gid)
{
    vkrMeshId id;
    id.asint = gid.asint;
    return id;
}

bool vkrMegaMesh_Init(void)
{
    return true;
}

ProfileMark(pm_update, vkrMegaMesh_Update)
void vkrMegaMesh_Update(void)
{
    ProfileScope(pm_update);

    const u32 syncIndex = vkr_syncIndex();
    if (ms_dirty[syncIndex])
    {
        ms_dirty[syncIndex] = false;

        // TODO: partial uploads and stage buffer cache
        const i32 vertexBytes = ms_mega.GetBytes();
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

        ms_mega.Fill(stage);
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

}

void vkrMegaMesh_Shutdown(void)
{
    vkrBufferSet_Del(&ms_buffer);
    vkrBufferSet_Del(&ms_stage);

    ms_ids.Reset();
    ms_meshes.Reset();
    ms_mega.Reset();
}

bool vkrMegaMesh_Exists(vkrMeshId id)
{
    return ms_ids.Exists(ToGenId(id));
}

vkrMeshId vkrMegaMesh_Alloc(i32 vertCount)
{
    GenId gid = ms_ids.Alloc();
    i32 index = gid.index;
    if (index >= ms_meshes.Size())
    {
        ms_meshes.Grow();
    }
    SubMesh& mesh = ms_meshes[index];
    mesh = {};
    mesh.length = vertCount;
    mesh.offset = ms_mega.Allocate(vertCount);

    memset(ms_dirty, 0xff, sizeof(ms_dirty));

    return ToMeshId(gid);
}

bool vkrMegaMesh_Free(vkrMeshId id)
{
    if (ms_ids.Free(ToGenId(id)))
    {
        SubMesh& mesh = ms_meshes[id.index];
        ms_mega.Free(mesh.offset, mesh.length);
        for (SubMesh& sm : ms_meshes)
        {
            if (sm.offset > mesh.offset)
            {
                sm.offset -= mesh.length;
            }
        }
        mesh = {};

        memset(ms_dirty, 0xff, sizeof(ms_dirty));
        return true;
    }
    return false;
}

bool vkrMegaMesh_Set(
    vkrMeshId id,
    const float4* positions,
    const float4* normals,
    const float4* uvs,
    const int4* texIndices,
    i32 vertCount)
{
    if (vkrMegaMesh_Exists(id))
    {
        const SubMesh& mesh = ms_meshes[id.index];
        i32 offset = mesh.offset;
        i32 len = mesh.length;
        ASSERT(len == vertCount);
        if (len == vertCount)
        {
            MegaMesh& mega = ms_mega;
            memcpy(&mega.m_positions[offset], positions, sizeof(positions[0]) * len);
            memcpy(&mega.m_normals[offset], normals, sizeof(normals[0]) * len);
            memcpy(&mega.m_uvs[offset], uvs, sizeof(uvs[0]) * len);
            memcpy(&mega.m_texIndices[offset], texIndices, sizeof(texIndices[0]) * len);
            memset(ms_dirty, 0xff, sizeof(ms_dirty));
            return true;
        }
    }
    return false;
}

void vkrMegaMesh_Draw(VkCommandBuffer cmd)
{
    const i32 vertCount = ms_mega.Size();
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
    const i32 vertCount = ms_mega.Size();
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
