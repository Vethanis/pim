#include "rendering/vulkan/vkr_immesh.h"
#include "rendering/vulkan/vkr_buffer.h"
#include "rendering/vulkan/vkr_context.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_textable.h"

#include "allocator/allocator.h"
#include "rendering/drawable.h"
#include "rendering/material.h"
#include "rendering/mesh.h"
#include "rendering/texture.h"
#include "math/float4x4_funcs.h"
#include "common/profiler.h"
#include <string.h>

typedef struct MacroMesh_s
{
    i32 length;
    float4* pim_noalias positions;
    float4* pim_noalias normals;
    float4* pim_noalias uvs;
    int4* pim_noalias texIndices;
} MacroMesh;

static i32 MacroMesh_Bytes(const MacroMesh* m);
static void MacroMesh_Resize(MacroMesh* m, i32 ct);
static void MacroMesh_Clear(MacroMesh* m);
static void MacroMesh_Reset(MacroMesh* m);
static bool MacroMesh_Fill(MacroMesh* m, vkrBuffer const *const buffer);
static i32 MacroMesh_Alloc(MacroMesh* m, i32 vertCount);

static void vkrImMesh_Flush(VkCommandBuffer cmd);

static vkrBufferSet ms_buffer;
static vkrBufferSet ms_stage;
static MacroMesh ms_macro;
static bool ms_dirty;

bool vkrImMesh_Init(void)
{
    return true;
}

ProfileMark(pm_update, vkrImMesh_Update)
void vkrImMesh_Update(void)
{
    ProfileBegin(pm_update);

    vkrImMesh_Clear();

#if 0
    i32 allVertCount = 0;
    float4* pim_noalias allPos = ms_macro.positions;
    float4* pim_noalias allNor = ms_macro.normals;
    float4* pim_noalias allUvs = ms_macro.uvs;
    int4* pim_noalias allInds = ms_macro.texIndices;

    const Entities* ents = Entities_Get();
    const i32 entCount = ents->count;
    const MeshId* pim_noalias meshes = ents->meshes;
    const Material* pim_noalias materials = ents->materials;
    const float4x4* pim_noalias localToWorlds = ents->matrices;
    const float3x3* pim_noalias worldToLocals = ents->invMatrices;
    for (i32 iEnt = 0; iEnt < entCount; ++iEnt)
    {
        Mesh* mesh = Mesh_Get(meshes[iEnt]);
        if (mesh)
        {
            const i32 vertCount = mesh->length;
            const i32 offset = allVertCount;
            allVertCount += vertCount;
            allPos = Perm_Realloc(allPos, sizeof(allPos[0]) * allVertCount);
            allNor = Perm_Realloc(allNor, sizeof(allNor[0]) * allVertCount);
            allUvs = Perm_Realloc(allUvs, sizeof(allUvs[0]) * allVertCount);
            allInds = Perm_Realloc(allInds, sizeof(allInds[0]) * allVertCount);

            const float4* pim_noalias positions = mesh->positions;
            const float4* pim_noalias normals = mesh->normals;
            const float4* pim_noalias uvs = mesh->uvs;
            const int4* pim_noalias inds = mesh->texIndices;
            const float4x4 localToWorld = localToWorlds[iEnt];
            const float3x3 worldToLocal = worldToLocals[iEnt];
            const Material material = materials[iEnt];

            i32 albedoIndex = 0;
            i32 romeIndex = 0;
            i32 normalIndex = 0;
            const Texture* pim_noalias tex = Texture_Get(material.albedo);
            if (tex)
            {
                albedoIndex = tex->slot.index;
            }
            tex = Texture_Get(material.rome);
            if (tex)
            {
                romeIndex = tex->slot.index;
            }
            tex = Texture_Get(material.normal);
            if (tex)
            {
                normalIndex = tex->slot.index;
            }

            allPos += offset;
            allNor += offset;
            allUvs += offset;
            allInds += offset;
            ASSERT((vertCount % 3) == 0);
            for (i32 iVert = 0; iVert < vertCount; ++iVert)
            {
                allPos[iVert] = f4x4_mul_pt(localToWorld, positions[iVert]);
                allNor[iVert] = f3x3_mul_col(worldToLocal, normals[iVert]);
                allUvs[iVert] = uvs[iVert];
                int4 ind = inds[iVert];
                ind.x = albedoIndex;
                ind.y = romeIndex;
                ind.z = normalIndex;
                allInds[iVert] = ind;
            }
            allPos -= offset;
            allNor -= offset;
            allUvs -= offset;
            allInds -= offset;
        }
    }

    ms_macro.length = allVertCount;
    ms_macro.positions = allPos;
    ms_macro.normals = allNor;
    ms_macro.uvs = allUvs;
    ms_macro.texIndices = allInds;
    ms_dirty = true;

#endif // 0

    vkrImMesh_Flush(NULL);

    ProfileEnd(pm_update);
}

void vkrImMesh_Shutdown(void)
{
    vkrBufferSet_Del(&ms_buffer);
    vkrBufferSet_Del(&ms_stage);
    MacroMesh_Reset(&ms_macro);
}

void vkrImMesh_Clear(void)
{
    MacroMesh_Clear(&ms_macro);
    ms_dirty = true;
}

bool vkrImMesh_Add(
    const float4* pim_noalias positions,
    const float4* pim_noalias normals,
    const float4* pim_noalias uvs,
    const int4* pim_noalias texIndices,
    i32 vertCount)
{
    if (vertCount > 0)
    {
        ms_dirty = true;
        i32 offset = MacroMesh_Alloc(&ms_macro, vertCount);
        for (i32 i = 0; i < vertCount; ++i)
        {
            ms_macro.positions[offset + i] = positions[i];
            ms_macro.normals[offset + i] = normals[i];
            ms_macro.uvs[offset + i] = uvs[i];
            ms_macro.texIndices[offset + i] = texIndices[i];
        }
        return true;
    }
    return false;
}

void VEC_CALL vkrImMesh_Vertex(float4 pos, float4 nor, float4 uv, int4 itex)
{
    ms_dirty = true;
    i32 i = MacroMesh_Alloc(&ms_macro, 1);
    ms_macro.positions[i] = pos;
    ms_macro.normals[i] = nor;
    ms_macro.uvs[i] = uv;
    ms_macro.texIndices[i] = itex;
}

void vkrImMesh_Draw(VkCommandBuffer cmd)
{
    if (ms_dirty)
    {
        // cannot flush during a VkRenderPass
        INTERRUPT();
        return;
    }

    const i32 vertCount = ms_macro.length;
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

void vkrImMesh_DrawPosition(VkCommandBuffer cmd)
{
    if (ms_dirty)
    {
        // cannot flush during a VkRenderPass
        INTERRUPT();
        return;
    }

    const i32 vertCount = ms_macro.length;
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

static i32 MacroMesh_Bytes(const MacroMesh* m)
{
    const i32 bytesPerVert =
        sizeof(m->positions[0]) +
        sizeof(m->normals[0]) +
        sizeof(m->uvs[0]) +
        sizeof(m->texIndices[0]);
    return m->length * bytesPerVert;
}

static void MacroMesh_Resize(MacroMesh* m, i32 ct)
{
    ASSERT(ct >= 0);
    if (ct > m->length)
    {
        Perm_Reserve(m->positions, ct);
        Perm_Reserve(m->normals, ct);
        Perm_Reserve(m->uvs, ct);
        Perm_Reserve(m->texIndices, ct);
    }
    m->length = ct;
}

static void MacroMesh_Clear(MacroMesh* m)
{
    m->length = 0;
}

static void MacroMesh_Reset(MacroMesh* m)
{
    Mem_Free(m->positions);
    Mem_Free(m->normals);
    Mem_Free(m->uvs);
    Mem_Free(m->texIndices);
    memset(m, 0, sizeof(*m));
}

static bool MacroMesh_Fill(MacroMesh* m, vkrBuffer const *const buffer)
{
    ASSERT(buffer->size >= MacroMesh_Bytes(m));
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

static i32 MacroMesh_Alloc(MacroMesh* m, i32 vertCount)
{
    i32 b = m->length;
    MacroMesh_Resize(m, b + vertCount);
    return b;
}

ProfileMark(pm_flush, vkrImMesh_Flush)
static void vkrImMesh_Flush(VkCommandBuffer cmd)
{
    ProfileBegin(pm_flush);

    if (ms_dirty)
    {
        ms_dirty = false;

        const i32 vertexBytes = MacroMesh_Bytes(&ms_macro);
        ASSERT(vertexBytes >= 0);
        if (vertexBytes > 0)
        {
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

            MacroMesh_Fill(&ms_macro, stage);

            bool tmpCmd = !cmd;
            VkFence fence = NULL;
            VkQueue queue = NULL;
            if (tmpCmd)
            {
                cmd = vkrContext_GetTmpCmd(vkrQueueId_Gfx, &fence, &queue);
                vkrCmdBegin(cmd);
            }
            vkrBuffer_Barrier(
                stage,
                cmd,
                VK_ACCESS_HOST_WRITE_BIT,
                VK_ACCESS_TRANSFER_READ_BIT,
                VK_ACCESS_HOST_WRITE_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT);
            vkrCmdCopyBuffer(cmd, stage, buffer);
            vkrBuffer_Barrier(
                buffer,
                cmd,
                VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_VERTEX_INPUT_BIT);
            if (tmpCmd)
            {
                vkrCmdEnd(cmd);
                vkrCmdSubmit(queue, cmd, fence, NULL, 0x0, NULL);
            }
        }
    }

    ProfileEnd(pm_flush);
}
