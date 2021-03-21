#include "rendering/vulkan/vkr_opaquepass.h"
#include "rendering/vulkan/vkr_pass.h"
#include "rendering/vulkan/vkr_swapchain.h"
#include "rendering/vulkan/vkr_buffer.h"
#include "rendering/vulkan/vkr_textable.h"
#include "rendering/vulkan/vkr_shader.h"
#include "rendering/vulkan/vkr_context.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_desc.h"
#include "rendering/vulkan/vkr_bindings.h"
#include "rendering/vulkan/vkr_megamesh.h"

#include "rendering/drawable.h"
#include "rendering/mesh.h"
#include "rendering/texture.h"
#include "rendering/material.h"
#include "rendering/camera.h"
#include "rendering/lightmap.h"

#include "allocator/allocator.h"
#include "common/profiler.h"
#include "containers/table.h"
#include "math/box.h"
#include "math/frustum.h"
#include "threading/task.h"
#include <string.h>

// ----------------------------------------------------------------------------

bool vkrOpaquePass_New(vkrOpaquePass *const pass, VkRenderPass renderPass)
{
    ASSERT(pass);
    ASSERT(renderPass);
    memset(pass, 0, sizeof(*pass));
    bool success = true;

    for (i32 i = 0; i < kFramesInFlight; ++i)
    {
        if (!vkrBuffer_New(
            &pass->perCameraBuffer[i],
            sizeof(vkrPerCamera),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            vkrMemUsage_CpuToGpu))
        {
            success = false;
            goto cleanup;
        }
    }

    VkPipelineShaderStageCreateInfo shaders[2] = { 0 };
    if (!vkrShader_New(&shaders[0], "brush.hlsl", "VSMain", vkrShaderType_Vert))
    {
        success = false;
        goto cleanup;
    }
    if (!vkrShader_New(&shaders[1], "brush.hlsl", "PSMain", vkrShaderType_Frag))
    {
        success = false;
        goto cleanup;
    }

    const VkVertexInputBindingDescription vertBindings[] =
    {
        // positionWS
        {
            .binding = 0,
            .stride = sizeof(float4),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        },
        // normalWS and lightmap index
        {
            .binding = 1,
            .stride = sizeof(float4),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        },
        // uv0 and uv1
        {
            .binding = 2,
            .stride = sizeof(float4),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        },
        // texture indices
        {
            .binding = 3,
            .stride = sizeof(int4),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        },
    };
    const VkVertexInputAttributeDescription vertAttributes[] =
    {
        // positionOS
        {
            .binding = 0,
            .location = 0,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        },
        // normalOS and lightmap index
        {
            .binding = 1,
            .location = 1,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        },
        // uv0 and uv1
        {
            .binding = 2,
            .location = 2,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        },
        // texture indices
        {
            .binding = 3,
            .location = 3,
            .format = VK_FORMAT_R32G32B32A32_UINT,
        }
    };

    const vkrPassDesc desc =
    {
        .renderPass = renderPass,
        .subpass = vkrPassId_Opaque,
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
            .depthCompareOp = VK_COMPARE_OP_EQUAL,
            .scissorOn = false,
            .depthClamp = false,
            .depthTestEnable = true,
            .depthWriteEnable = false,
            .attachmentCount = 2,
            .attachments[0] =
            {
                .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
                .blendEnable = false,
            },
            .attachments[1] =
            {
                .colorWriteMask = VK_COLOR_COMPONENT_R_BIT,
                .blendEnable = false,
            },
        },
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
        vkrOpaquePass_Del(pass);
    }
    return success;
}

void vkrOpaquePass_Del(vkrOpaquePass *const pass)
{
    if (pass)
    {
        vkrPass_Del(&pass->pass);
        for (i32 i = 0; i < kFramesInFlight; ++i)
        {
            vkrBuffer_Del(&pass->perCameraBuffer[i]);
        }
        memset(pass, 0, sizeof(*pass));
    }
}

ProfileMark(pm_update, vkrOpaquePass_Setup)
void vkrOpaquePass_Setup(vkrOpaquePass *const pass)
{
    ProfileBegin(pm_update);

    const u32 syncIndex = vkrSys_SyncIndex();
    VkDescriptorSet set = vkrBindings_GetSet();
    vkrBuffer *const camBuffer = &pass->perCameraBuffer[syncIndex];
    vkrBuffer *const expBuffer = &g_vkr.exposurePass.expBuffers[syncIndex];

    // update per camera buffer
    {
        Camera camera;
        camera_get(&camera);
        vkrPerCamera *const perCamera = vkrBuffer_Map(camBuffer);
        ASSERT(perCamera);
        perCamera->worldToClip = g_vkr.mainPass.depth.worldToClip;
        perCamera->eye = camera.position;
        perCamera->exposure = g_vkr.exposurePass.params.exposure;
        vkrBuffer_Unmap(camBuffer);
        vkrBuffer_Flush(camBuffer);
    }

    vkrBindings_BindBuffer(
        vkrBindId_CameraData,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        camBuffer);
    vkrBindings_BindBuffer(
        vkrBindId_ExposureBuffer,
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        expBuffer);

    ProfileEnd(pm_update);
}

// ----------------------------------------------------------------------------

ProfileMark(pm_execute, vkrOpaquePass_Execute)
void vkrOpaquePass_Execute(
    vkrPassContext const *const ctx,
    vkrOpaquePass *const pass)
{
    ProfileBegin(pm_execute);

    VkDescriptorSet set = vkrBindings_GetSet();
    VkCommandBuffer cmd = ctx->cmd;
    vkrCmdViewport(cmd, vkrSwapchain_GetViewport(&g_vkr.chain), vkrSwapchain_GetRect(&g_vkr.chain));
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pass->pass.pipeline);
    vkrCmdBindDescSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pass->pass.layout, 1, &set);

    vkrMegaMesh_Draw(cmd);

    ProfileEnd(pm_execute);
}
