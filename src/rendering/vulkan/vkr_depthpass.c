#include "rendering/vulkan/vkr_depthpass.h"
#include "rendering/vulkan/vkr_pass.h"
#include "rendering/vulkan/vkr_shader.h"
#include "rendering/vulkan/vkr_desc.h"
#include "rendering/vulkan/vkr_context.h"
#include "rendering/vulkan/vkr_swapchain.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_buffer.h"
#include "rendering/vulkan/vkr_mesh.h"

#include "rendering/drawable.h"
#include "rendering/mesh.h"
#include "rendering/camera.h"
#include "rendering/material.h"
#include "rendering/texture.h"
#include "containers/table.h"

#include "allocator/allocator.h"
#include "common/profiler.h"
#include "math/float4x4_funcs.h"
#include "math/box.h"
#include "math/frustum.h"
#include "threading/task.h"
#include <string.h>

static vkrDepthPass_Execute(const vkrPassContext* ctx, vkrDepthPass* pass);

// ----------------------------------------------------------------------------

bool vkrDepthPass_New(vkrDepthPass* pass, VkRenderPass renderPass)
{
    ASSERT(pass);
    ASSERT(renderPass);
    memset(pass, 0, sizeof(*pass));
    bool success = true;

    VkPipelineShaderStageCreateInfo shaders[2] = { 0 };
    if (!vkrShader_New(&shaders[0], "DepthOnly.hlsl", "VSMain", vkrShaderType_Vert))
    {
        success = false;
        goto cleanup;
    }
    if (!vkrShader_New(&shaders[1], "DepthOnly.hlsl", "PSMain", vkrShaderType_Frag))
    {
        success = false;
        goto cleanup;
    }

    const VkPushConstantRange ranges[] =
    {
        {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .size = sizeof(float4x4),
        },
    };
    const VkVertexInputBindingDescription vertBindings[] =
    {
        // positionOS
        {
            .binding = 0,
            .stride = sizeof(float4),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        },
    };
    const VkVertexInputAttributeDescription vertAttributes[] =
    {
        // positionOS
        {
            .binding = 0,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .location = 0,
        },
    };

    const vkrPassDesc desc =
    {
        .bindpoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .renderPass = renderPass,
        .subpass = vkrPassId_Depth,
        .vertLayout =
        {
            .bindingCount = NELEM(vertBindings),
            .bindings = vertBindings,
            .attributeCount = NELEM(vertAttributes),
            .attributes = vertAttributes,
        },
        .fixedFuncs =
        {
            .viewport = vkrSwapchain_GetViewport(&g_vkr.chain),
            .scissor = vkrSwapchain_GetRect(&g_vkr.chain),
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .cullMode = VK_CULL_MODE_BACK_BIT,
            .depthCompareOp = VK_COMPARE_OP_LESS,
            .scissorOn = false,
            .depthClamp = false,
            .depthTestEnable = true,
            .depthWriteEnable = true,
            .attachmentCount = 1,
            .attachments[0] =
            {
                .colorWriteMask = 0x0,
                .blendEnable = false,
            },
        },
        .rangeCount = NELEM(ranges),
        .ranges = ranges,
        .shaderCount = NELEM(shaders),
        .shaders = shaders,
    };

    if (!vkrPass_New(&pass->pass, &desc))
    {
        success = false;
        goto cleanup;
    }

cleanup:
    for (i32 i = 0; i < NELEM(shaders); ++i)
    {
        vkrShader_Del(&shaders[i]);
    }
    if (!success)
    {
        vkrDepthPass_Del(pass);
    }
    return success;
}

void vkrDepthPass_Del(vkrDepthPass* pass)
{
    if (pass)
    {
        vkrPass_Del(&pass->pass);
        for (i32 i = 0; i < NELEM(pass->stagebufs); ++i)
        {
            vkrBuffer_Del(&pass->stagebufs[i]);
            vkrBuffer_Del(&pass->meshbufs[i]);
        }
        memset(pass, 0, sizeof(*pass));
    }
}

ProfileMark(pm_draw, vkrDepthPass_Draw)
void vkrDepthPass_Draw(const vkrPassContext* passCtx, vkrDepthPass* pass)
{
    ProfileBegin(pm_draw);
    vkrDepthPass_Execute(passCtx, pass);
    ProfileEnd(pm_draw);
}

// ----------------------------------------------------------------------------

ProfileMark(pm_execute, vkrDepthPass_Execute)
static vkrDepthPass_Execute(const vkrPassContext* ctx, vkrDepthPass* pass)
{
    ProfileBegin(pm_execute);

    const u32 syncIndex = ctx->syncIndex;
    const vkrSwapchain* chain = &g_vkr.chain;
    VkRect2D rect = vkrSwapchain_GetRect(chain);
    VkViewport viewport = vkrSwapchain_GetViewport(chain);
    const float aspect = viewport.width / viewport.height;

    camera_t camera;
    camera_get(&camera);
    float4 at = f4_add(camera.position, quat_fwd(camera.rotation));
    float4 up = quat_up(camera.rotation);
    float4x4 view = f4x4_lookat(camera.position, at, up);
    float4x4 proj = f4x4_vkperspective(f1_radians(camera.fovy), aspect, camera.zNear, camera.zFar);
    float4x4 worldToClip = f4x4_mul(proj, view);
    frus_t frustum;
    camera_frustum(&camera, &frustum, aspect);
    plane_t fwd = frustum.z0;
    {
        // frustum has outward facing planes
        // we want forward plane for depth sorting draw calls
        float w = fwd.value.w;
        fwd.value = f4_neg(fwd.value);
        fwd.value.w = w;
    }

    drawables_t const *const drawables = drawables_get();
    const i32 drawcount = drawables->count;
    meshid_t const *const pim_noalias meshids = drawables->meshes;
    float4x4 const *const pim_noalias matrices = drawables->matrices;
    float3x3 const *const pim_noalias invMatrices = drawables->invMatrices;
    box_t const *const pim_noalias localBounds = drawables->bounds;
    material_t const *const pim_noalias materials = drawables->materials;

    i32 visibleDrawCount = 0;
    i32 visibleVertCount = 0;
    i32* pim_noalias visibleIndices = NULL;
    int2* pim_noalias visibleRanges = NULL;
    for (i32 i = 0; i < drawcount; ++i)
    {
        float4x4 localToWorld = matrices[i];
        box_t bounds = box_transform(localToWorld, localBounds[i]);
        if (sdFrusBox(frustum, bounds) > 0.0f)
        {
            continue;
        }
        const mesh_t* pMesh = mesh_get(meshids[i]);
        if (!pMesh)
        {
            continue;
        }
        const i32 meshLen = pMesh->length;
        if (meshLen > 0)
        {
            ++visibleDrawCount;
            visibleIndices = tmp_realloc(visibleIndices, sizeof(visibleIndices[0]) * visibleDrawCount);
            visibleRanges = tmp_realloc(visibleRanges, sizeof(visibleRanges[0]) * visibleDrawCount);
            visibleIndices[visibleDrawCount - 1] = i;
            visibleRanges[visibleDrawCount - 1] = (int2) { visibleVertCount, meshLen };
            visibleVertCount += meshLen;
        }
    }

    pass->vertCount = visibleVertCount;
    const i32 meshBytes = (sizeof(float4) * vkrMeshStream_COUNT) * visibleVertCount;
    vkrBuffer *const stageBuf = &pass->stagebufs[syncIndex];
    vkrBuffer *const meshBuf = &pass->meshbufs[syncIndex];
    vkrBuffer_Reserve(
        stageBuf,
        meshBytes,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        vkrMemUsage_CpuOnly,
        NULL,
        PIM_FILELINE);
    vkrBuffer_Reserve(
        meshBuf,
        meshBytes,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        vkrMemUsage_GpuOnly,
        NULL,
        PIM_FILELINE);

    {
        float4 *const pim_noalias positions = vkrBuffer_Map(stageBuf);
        float4 *const pim_noalias normals = positions + visibleVertCount;
        float4 *const pim_noalias uv01s = normals + visibleVertCount;
        int4 *const pim_noalias texIndices = (int4*)(uv01s + visibleVertCount);
        texture_t const *const pim_noalias textures = texture_table()->values;

        i32 vertCount = 0;
        for (i32 iVis = 0; iVis < visibleDrawCount; ++iVis)
        {
            const i32 iMesh = visibleIndices[iVis];
            const mesh_t mesh = *mesh_get(meshids[iMesh]);
            const i32 vertBack = vertCount;
            const i32 meshLen = mesh.length;
            vertCount += meshLen;

            const float4x4 localToWorld = matrices[iMesh];
            const material_t mat = materials[iMesh];
            const i32 albedoIndex = textures[mat.albedo.index].slot.index;
            const i32 romeIndex = textures[mat.rome.index].slot.index;
            const i32 normalIndex = textures[mat.normal.index].slot.index;
            const float3x3 IM = invMatrices[iMesh];

            for (i32 j = 0; j < meshLen; ++j)
            {
                positions[vertBack + j] = f4x4_mul_pt(localToWorld, mesh.positions[j]);
                float4 normal = mesh.normals[j];
                normals[vertBack + j] = f3x3_mul_col(IM, normal);
                uv01s[vertBack + j] = mesh.uvs[j];
                i32 lmIndex = (i32)normal.w;
                int4 texIdx = { albedoIndex, romeIndex, normalIndex, lmIndex };
                texIndices[vertBack + j] = texIdx;
            }
        }
        ASSERT(vertCount == visibleVertCount);

        vkrBuffer_Unmap(stageBuf);
        vkrBuffer_Flush(stageBuf);
    }

    {
        VkFence fence = NULL;
        VkQueue queue = NULL;
        VkCommandBuffer cpyCmd = vkrContext_GetTmpCmd(vkrContext_Get(), vkrQueueId_Gfx, &fence, &queue);
        vkrCmdBegin(cpyCmd);
        vkrBuffer_Barrier(
            meshBuf,
            cpyCmd,
            VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_SHADER_READ_BIT,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT);
        vkrCmdCopyBuffer(cpyCmd, *stageBuf, *meshBuf);
        vkrBuffer_Barrier(
            meshBuf,
            cpyCmd,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
        vkrCmdEnd(cpyCmd);
        vkrCmdSubmit(queue, cpyCmd, fence, NULL, 0x0, NULL);
    }

    VkRenderPass renderPass = ctx->renderPass;
    VkPipeline pipeline = pass->pass.pipeline;
    VkPipelineLayout layout = pass->pass.layout;
    VkCommandBuffer cmd = ctx->cmd;

    vkrCmdViewport(cmd, viewport, rect);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    struct
    {
        float4x4 worldToClip;
    } pushConsts;
    pushConsts.worldToClip = worldToClip;
    vkCmdPushConstants(
        cmd,
        layout,
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(pushConsts),
        &pushConsts);

    // position only
    const VkBuffer buffers[] =
    {
        meshBuf->handle,
    };
    const VkDeviceSize offsets[] =
    {
        0,
    };
    vkCmdBindVertexBuffers(cmd, 0, NELEM(buffers), buffers, offsets);
    vkCmdDraw(cmd, visibleVertCount, 1, 0, 0);

    ProfileEnd(pm_execute);
}