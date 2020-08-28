#include "rendering/vulkan/vkr_mesh.h"
#include "rendering/vulkan/vkr_mem.h"
#include "allocator/allocator.h"
#include <string.h>

bool vkrMesh_New(
    vkrMesh* mesh,
    i32 length,
    const float4* pim_noalias positions,
    const float4* pim_noalias normals,
    const float4* pim_noalias uv01)
{
    ASSERT(mesh);
    ASSERT(length >= 0);
    ASSERT(positions || !length);
    ASSERT(normals || !length);
    ASSERT(uv01 || !length);

    memset(mesh, 0, sizeof(*mesh));
    if (length <= 0)
    {
        return false;
    }
    if (!positions || !normals || !uv01)
    {
        return false;
    }

    mesh->length = length;
    if (!vkrBuffer_New(
        &mesh->positions,
        sizeof(positions[0]) * length,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        vkrMemUsage_CpuToGpu))
    {
        ASSERT(false);
        goto cleanup;
    }
    if (!vkrBuffer_New(
        &mesh->normals,
        sizeof(normals[0]) * length,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        vkrMemUsage_CpuToGpu))
    {
        ASSERT(false);
        goto cleanup;
    }
    if (!vkrBuffer_New(
        &mesh->uv01,
        sizeof(uv01[0]) * length,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        vkrMemUsage_CpuToGpu))
    {
        ASSERT(false);
        goto cleanup;
    }

    {
        void* dst = vkrBuffer_Map(&mesh->positions);
        ASSERT(dst);
        if (dst)
        {
            memcpy(dst, positions, sizeof(positions[0]) * length);
        }
        vkrBuffer_Unmap(&mesh->positions);
    }
    {
        void* dst = vkrBuffer_Map(&mesh->normals);
        ASSERT(dst);
        if (dst)
        {
            memcpy(dst, normals, sizeof(normals[0]) * length);
        }
        vkrBuffer_Unmap(&mesh->normals);
    }
    {
        void* dst = vkrBuffer_Map(&mesh->uv01);
        ASSERT(dst);
        if (dst)
        {
            memcpy(dst, uv01, sizeof(uv01[0]) * length);
        }
        vkrBuffer_Unmap(&mesh->uv01);
    }
    vkrBuffer_Flush(&mesh->positions);
    vkrBuffer_Flush(&mesh->normals);
    vkrBuffer_Flush(&mesh->uv01);

    return true;
cleanup:
    vkrMesh_Del(mesh);
    return false;
}

void vkrMesh_Del(vkrMesh* mesh)
{
    if (mesh)
    {
        vkrBuffer_Del(&mesh->uv01);
        vkrBuffer_Del(&mesh->normals);
        vkrBuffer_Del(&mesh->positions);
    }
}
