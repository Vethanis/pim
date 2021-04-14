#include "rendering/vulkan/vkr_depthpass.h"
#include "rendering/vulkan/vkr_pass.h"
#include "rendering/vulkan/vkr_shader.h"
#include "rendering/vulkan/vkr_desc.h"
#include "rendering/vulkan/vkr_context.h"
#include "rendering/vulkan/vkr_swapchain.h"
#include "rendering/vulkan/vkr_renderpass.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_buffer.h"
#include "rendering/vulkan/vkr_mesh.h"
#include "rendering/vulkan/vkr_im.h"

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

typedef struct PushConstants_s
{
    float4x4 localToClip;
} PushConstants;

static VkRenderPass ms_renderPass;
static vkrPass ms_pass;

// ----------------------------------------------------------------------------

bool vkrDepthPass_New(void)
{
    bool success = true;

    const vkrRenderPassDesc renderPassDesc =
    {
        .srcStageMask =
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .srcAccessMask =
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,

        .dstStageMask =
            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .dstAccessMask =
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,

        .attachments[0] =
        {
            .format = g_vkr.chain.depthAttachments[0].format,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .load = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .store = VK_ATTACHMENT_STORE_OP_STORE,
        },
        .attachments[1] =
        {
            .format = g_vkr.chain.colorFormat,
            .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .load = VK_ATTACHMENT_LOAD_OP_LOAD,
            .store = VK_ATTACHMENT_STORE_OP_STORE,
        },
        .attachments[2] =
        {
            .format = g_vkr.chain.lumAttachments[0].format,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .load = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .store = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        },
    };
    ms_renderPass = vkrRenderPass_Get(&renderPassDesc);
    if (!ms_renderPass)
    {
        success = false;
        goto cleanup;
    }

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
        .pushConstantBytes = sizeof(PushConstants),
        .shaderCount = NELEM(shaders),
        .shaders = shaders,
        .renderPass = ms_renderPass,
        .subpass = 0,
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
            .attachmentCount = 2,
        },
    };

    if (!vkrPass_New(&ms_pass, &desc))
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
        vkrDepthPass_Del();
    }
    return success;
}

void vkrDepthPass_Del(void)
{
    vkrPass_Del(&ms_pass);
}

void vkrDepthPass_Setup(void)
{

}

ProfileMark(pm_execute, vkrDepthPass_Execute)
void vkrDepthPass_Execute(vkrPassContext const *const ctx)
{
    ProfileBegin(pm_execute);

    Camera camera;
    Camera_Get(&camera);
    float4x4 worldToClip = Camera_GetWorldToClip(&camera, vkrSwapchain_GetAspect(&g_vkr.chain));

    VkCommandBuffer cmd = ctx->cmd;
    vkrCmdDefaultViewport(cmd);
    vkrCmdBindPass(cmd, &ms_pass);
    const VkClearValue clearValues[] =
    {
        {
            .depthStencil = { 1.0f, 0 },
        },
        {
            .color = { 0.0f, 0.0f, 0.0f, 1.0f },
        },
        {
            .color = { 0.0f, 0.0f, 0.0f, 1.0f },
        },
    };
    vkrCmdBeginRenderPass(
        cmd,
        ms_renderPass,
        ctx->framebuffer,
        vkrSwapchain_GetRect(&g_vkr.chain),
        NELEM(clearValues), clearValues,
        VK_SUBPASS_CONTENTS_INLINE);

    const Entities* ents = Entities_Get();
    for (i32 iEnt = 0; iEnt < ents->count; ++iEnt)
    {
        const Mesh* mesh = Mesh_Get(ents->meshes[iEnt]);
        if (mesh)
        {
            PushConstants pc;
            pc.localToClip = f4x4_mul(worldToClip, ents->matrices[iEnt]);
            vkrCmdPushConstants(cmd, &ms_pass, &pc, sizeof(pc));
            vkrCmdDrawMesh(cmd, mesh->id);
        }
    }

    PushConstants pc;
    pc.localToClip = worldToClip;
    vkrCmdPushConstants(cmd, &ms_pass, &pc, sizeof(pc));
    vkrImSys_DrawDepth(cmd);

    vkrCmdEndRenderPass(cmd);

    ProfileEnd(pm_execute);
}