#include "rendering/vulkan/vkr.h"

#include "rendering/vulkan/vkr_im.h"
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
    int4* pim_noalias texInds;
} MacroMesh;

static i32 MacroMesh_Bytes(const MacroMesh* m);
static void MacroMesh_Resize(MacroMesh* m, i32 ct);
static void MacroMesh_Reset(MacroMesh* m);
static bool MacroMesh_Fill(MacroMesh* m, vkrBuffer* buffer);
static i32 MacroMesh_Alloc(MacroMesh* m, i32 vertCount);

static vkrBufferSet ms_buffer;
static vkrBufferSet ms_stage;
static i32 ms_gpuVertCount;
static MacroMesh ms_macro;
static bool ms_dirty;
static bool ms_capture;
static bool ms_begin;

bool vkrImSys_Init(void)
{
    return true;
}

void vkrImSys_Shutdown(void)
{
    vkrBufferSet_Release(&ms_buffer);
    vkrBufferSet_Release(&ms_stage);
    MacroMesh_Reset(&ms_macro);
}

void vkrImSys_Clear(void)
{
    ms_dirty = ms_macro.length != 0;
    ms_capture = true;
    ms_macro.length = 0;
    ms_gpuVertCount = 0;
}

ProfileMark(pm_flush, vkrImSys_Flush)
void vkrImSys_Flush(void)
{
    ProfileBegin(pm_flush);

    ms_capture = false;

    if (ms_dirty)
    {
        ms_dirty = false;
        ms_gpuVertCount = 0;

        const i32 vertexBytes = MacroMesh_Bytes(&ms_macro);
        ASSERT(vertexBytes >= 0);
        if (vertexBytes > 0)
        {
            vkrBuffer *const buffer = vkrBufferSet_Current(&ms_buffer);
            vkrBuffer *const stage = vkrBufferSet_Current(&ms_stage);
            bool gotStage = vkrBuffer_Reserve(
                stage,
                vertexBytes,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                vkrMemUsage_CpuOnly);
            bool gotBuffer = vkrBuffer_Reserve(
                buffer,
                vertexBytes,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                vkrMemUsage_GpuOnly);

            if (gotStage && gotBuffer)
            {
                MacroMesh_Fill(&ms_macro, stage);
                ms_gpuVertCount = ms_macro.length;

                vkrCmdBuf* cmd = vkrCmdGet_G();
                vkrCmdCopyBuffer(cmd, stage, buffer);
                vkrBufferState_VertexBuffer(cmd, buffer);
            }
        }
    }

    ProfileEnd(pm_flush);
}

void vkrIm_Begin(void)
{
    ASSERT(ms_capture);
    ASSERT(!ms_begin);
    ms_begin = true;
}

void vkrIm_End(void)
{
    ASSERT(ms_capture);
    ASSERT(ms_begin);
    ms_begin = false;
}

void vkrIm_Mesh(
    const float4* pim_noalias positions,
    const float4* pim_noalias normals,
    const float4* pim_noalias uvs,
    const int4* pim_noalias texInds,
    i32 vertCount)
{
    ASSERT(ms_begin);
    ASSERT(vertCount >= 0);
    if (vertCount > 0)
    {
        ms_dirty = true;
        i32 offset = MacroMesh_Alloc(&ms_macro, vertCount);
        for (i32 i = 0; i < vertCount; ++i)
        {
            ms_macro.positions[offset + i] = positions[i];
            ms_macro.normals[offset + i] = normals[i];
            ms_macro.uvs[offset + i] = uvs[i];
            ms_macro.texInds[offset + i] = texInds[i];
        }
    }
}

void VEC_CALL vkrIm_Vert(float4 pos, float4 nor, float4 uv, int4 itex)
{
    ASSERT(ms_begin);
    ms_dirty = true;
    i32 i = MacroMesh_Alloc(&ms_macro, 1);
    ms_macro.positions[i] = pos;
    ms_macro.normals[i] = nor;
    ms_macro.uvs[i] = uv;
    ms_macro.texInds[i] = itex;
}

void vkrImSys_Draw(void)
{
    // cannot flush during a VkRenderPass
    ASSERT(!ms_dirty);

    const i32 vertCount = ms_gpuVertCount;
    if (vertCount > 0)
    {
        vkrCmdBuf* cmd = vkrCmdGet_G();
        ASSERT(cmd->inRenderPass);
        vkrBuffer *const buffer = vkrBufferSet_Current(&ms_buffer);
        ASSERT(buffer->state.access & VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);

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
        ASSERT(cmd->handle);
        vkCmdBindVertexBuffers(cmd->handle, 0, NELEM(buffers), buffers, offsets);
        vkCmdDraw(cmd->handle, vertCount, 1, 0, 0);
    }
}

void vkrImSys_DrawDepth(void)
{
    // cannot flush during a VkRenderPass
    ASSERT(!ms_dirty);

    const i32 vertCount = ms_gpuVertCount;
    if (vertCount > 0)
    {
        vkrCmdBuf* cmd = vkrCmdGet_G();
        ASSERT(cmd->inRenderPass);
        vkrBuffer *const buffer = vkrBufferSet_Current(&ms_buffer);
        ASSERT(buffer->state.access & VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);

        const VkDeviceSize streamSize = sizeof(float4) * vertCount;
        const VkBuffer buffers[] =
        {
            buffer->handle,
        };
        const VkDeviceSize offsets[] =
        {
            streamSize * 0,
        };
        ASSERT(cmd->handle);
        vkCmdBindVertexBuffers(cmd->handle, 0, NELEM(buffers), buffers, offsets);
        vkCmdDraw(cmd->handle, vertCount, 1, 0, 0);
    }
}

static i32 MacroMesh_Bytes(const MacroMesh* m)
{
    const i32 bytesPerVert =
        sizeof(m->positions[0]) +
        sizeof(m->normals[0]) +
        sizeof(m->uvs[0]) +
        sizeof(m->texInds[0]);
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
        Perm_Reserve(m->texInds, ct);
    }
    m->length = ct;
}

static void MacroMesh_Reset(MacroMesh* m)
{
    Mem_Free(m->positions);
    Mem_Free(m->normals);
    Mem_Free(m->uvs);
    Mem_Free(m->texInds);
    memset(m, 0, sizeof(*m));
}

static bool MacroMesh_Fill(MacroMesh* m, vkrBuffer* buffer)
{
    ASSERT(buffer->size >= MacroMesh_Bytes(m));
    const i32 stride = m->length;
    float4* mem = vkrBuffer_MapWrite(buffer);
    if (mem && stride > 0)
    {
        memcpy(mem, &m->positions[0], sizeof(m->positions[0]) * stride);
        mem += stride;
        memcpy(mem, &m->normals[0], sizeof(m->normals[0]) * stride);
        mem += stride;
        memcpy(mem, &m->uvs[0], sizeof(m->uvs[0]) * stride);
        mem += stride;
        memcpy(mem, &m->texInds[0], sizeof(m->texInds[0]) * stride);
        mem += stride;
    }
    vkrBuffer_UnmapWrite(buffer);
    return mem != NULL;
}

static i32 MacroMesh_Alloc(MacroMesh* m, i32 vertCount)
{
    i32 b = m->length;
    MacroMesh_Resize(m, b + vertCount);
    return b;
}
