#include "rendering/vulkan/vkr_depthpass.h"
#include "rendering/vulkan/vkr_pass.h"
#include "rendering/vulkan/vkr_shader.h"
#include "rendering/vulkan/vkr_desc.h"
#include "rendering/vulkan/vkr_context.h"
#include "rendering/vulkan/vkr_swapchain.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_buffer.h"
#include "rendering/vulkan/vkr_mesh.h"
#include "rendering/vulkan/vkr_megamesh.h"

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

// ----------------------------------------------------------------------------

bool vkrDepthPass_New(vkrDepthPass *const pass, VkRenderPass renderPass)
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
        .pushConstantBytes = sizeof(float4x4),
        .shaderCount = NELEM(shaders),
        .shaders = shaders,
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

void vkrDepthPass_Del(vkrDepthPass *const pass)
{
    if (pass)
    {
        vkrPass_Del(&pass->pass);
        memset(pass, 0, sizeof(*pass));
    }
}

ProfileMark(pm_setup, vkrDepthPass_Setup)
void vkrDepthPass_Setup(vkrDepthPass *const pass)
{
    ProfileBegin(pm_setup);

    const u32 syncIndex = vkr_syncIndex();
    VkViewport viewport = vkrSwapchain_GetViewport(&g_vkr.chain);
    const float aspect = viewport.width / viewport.height;

    camera_t camera;
    camera_get(&camera);
    float4 at = f4_add(camera.position, quat_fwd(camera.rotation));
    float4 up = quat_up(camera.rotation);
    float4x4 view = f4x4_lookat(camera.position, at, up);
    float4x4 proj = f4x4_vkperspective(f1_radians(camera.fovy), aspect, camera.zNear, camera.zFar);
    pass->worldToClip = f4x4_mul(proj, view);

    ProfileEnd(pm_setup);
}

// ----------------------------------------------------------------------------

ProfileMark(pm_execute, vkrDepthPass_Execute)
void vkrDepthPass_Execute(vkrPassContext const *const ctx, vkrDepthPass *const pass)
{
    ProfileBegin(pm_execute);

    const u32 syncIndex = vkr_syncIndex();
    vkrSwapchain const *const chain = &g_vkr.chain;
    VkRect2D rect = vkrSwapchain_GetRect(chain);
    VkViewport viewport = vkrSwapchain_GetViewport(chain);

    VkRenderPass renderPass = ctx->renderPass;
    VkPipeline pipeline = pass->pass.pipeline;
    VkPipelineLayout layout = pass->pass.layout;
    VkCommandBuffer cmd = ctx->cmd;

    vkrCmdViewport(cmd, viewport, rect);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdPushConstants(
        cmd,
        layout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(pass->worldToClip),
        &pass->worldToClip);

    vkrMegaMesh_DrawPosition(cmd);

    ProfileEnd(pm_execute);
}