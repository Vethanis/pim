#include "rendering/vulkan/vkr_opaquepass.h"
#include "rendering/vulkan/vkr_pass.h"
#include "rendering/vulkan/vkr_swapchain.h"
#include "rendering/vulkan/vkr_buffer.h"
#include "rendering/vulkan/vkr_textable.h"
#include "rendering/vulkan/vkr_shader.h"
#include "rendering/vulkan/vkr_context.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_desc.h"

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

static vkrOpaquePass_Update(const vkrPassContext* passCtx, vkrOpaquePass* pass);
static vkrOpaquePass_Execute(const vkrPassContext* ctx, vkrOpaquePass* pass);

// ----------------------------------------------------------------------------

bool vkrOpaquePass_New(vkrOpaquePass* pass, VkRenderPass renderPass)
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
            vkrMemUsage_CpuToGpu,
            PIM_FILELINE))
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

    const VkDescriptorPoolSize poolSizes[] =
    {
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
        },
        {
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
        },
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = kTextureDescriptors,
        },
    };
    const VkDescriptorSetLayoutBinding bindings[] =
    {
        {
            // per camera cbuffer
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        },
        {
            // texture + sampler table
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = kTextureDescriptors,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        },
        {
            // exposure storage buffer
            .binding = 3,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        },
    };
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
        .bindpoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
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
        .poolSizeCount = NELEM(poolSizes),
        .poolSizes = poolSizes,
        .bindingCount = NELEM(bindings),
        .bindings = bindings,
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

void vkrOpaquePass_Del(vkrOpaquePass* pass)
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

void vkrOpaquePass_Draw(const vkrPassContext* passCtx, vkrOpaquePass* pass)
{
    vkrOpaquePass_Update(passCtx, pass);
    vkrOpaquePass_Execute(passCtx, pass);
}

// ----------------------------------------------------------------------------

typedef struct vkrTaskDraw
{
    task_t task;
    frus_t frustum;
    vkrOpaquePass* pass;
    VkRenderPass renderPass;
    vkrPassId subpass;
    const vkrSwapchain* chain;
    VkDescriptorSet descSet;
    VkFramebuffer framebuffer;
    VkFence primaryFence;
    const drawables_t* drawables;
    VkCommandBuffer* buffers;
    float* distances;
} vkrTaskDraw;

ProfileMark(pm_execute, vkrOpaquePass_Execute)
static vkrOpaquePass_Execute(const vkrPassContext* ctx, vkrOpaquePass* pass)
{
    ProfileBegin(pm_execute);

    const u32 syncIndex = ctx->syncIndex;
    VkCommandBuffer cmd = ctx->cmd;
    vkrCmdViewport(cmd, vkrSwapchain_GetViewport(&g_vkr.chain), vkrSwapchain_GetRect(&g_vkr.chain));
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pass->pass.pipeline);
    vkrCmdBindDescSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pass->pass.layout, 1, &pass->pass.sets[ctx->syncIndex]);

    VkBuffer mesh = g_vkr.mainPass.depth.meshbufs[syncIndex].handle;
    const i32 vertCount = g_vkr.mainPass.depth.vertCount;
    const i32 stride = vertCount * sizeof(float4);
    const VkBuffer buffers[] =
    {
        mesh, // positions
        mesh, // normals
        mesh, // uvs
        mesh, // texture indices
    };
    const VkDeviceSize offsets[] =
    {
        stride * 0,
        stride * 1,
        stride * 2,
        stride * 3,
    };
    vkCmdBindVertexBuffers(cmd, 0, NELEM(buffers), buffers, offsets);
    vkCmdDraw(cmd, vertCount, 1, 0, 0);

    ProfileEnd(pm_execute);
}

ProfileMark(pm_update, vkrOpaquePass_Update)
static vkrOpaquePass_Update(const vkrPassContext* passCtx, vkrOpaquePass* pass)
{
    ProfileBegin(pm_update);

    const u32 syncIndex = passCtx->syncIndex;
    VkDescriptorSet set = pass->pass.sets[syncIndex];
    vkrBuffer* camBuffer = &pass->perCameraBuffer[syncIndex];
    VkBuffer expBuffer = g_vkr.exposurePass.expBuffers[syncIndex].handle;

    // update per camera buffer
    {
        const vkrSwapchain* chain = &g_vkr.chain;
        float aspect = (float)chain->width / chain->height;
        camera_t camera;
        camera_get(&camera);
        float4 at = f4_add(camera.position, quat_fwd(camera.rotation));
        float4 up = quat_up(camera.rotation);
        float4x4 view = f4x4_lookat(camera.position, at, up);
        float4x4 proj = f4x4_vkperspective(f1_radians(camera.fovy), aspect, camera.zNear, camera.zFar);

        const lmpack_t* lmpack = lmpack_get();
        i32 lmBegin = kTextureDescriptors - kGiDirections;
        if (lmpack->lmCount > 0)
        {
            lmBegin = lmpack->lightmaps[0].slots[0].index;
        }

        vkrPerCamera* perCamera = vkrBuffer_Map(camBuffer);
        ASSERT(perCamera);
        perCamera->worldToClip = f4x4_mul(proj, view);
        perCamera->eye = camera.position;
        perCamera->exposure = g_vkr.exposurePass.params.exposure;
        perCamera->lmBegin = lmBegin;
        vkrBuffer_Unmap(camBuffer);
        vkrBuffer_Flush(camBuffer);
    }

    const vkrBinding bufferBindings[] =
    {
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .set = set,
            .binding = 1,
            .buffer =
            {
                .buffer = camBuffer->handle,
                .range = VK_WHOLE_SIZE,
            },
        },
        {
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .set = set,
            .binding = 3,
            .buffer =
            {
                .buffer = expBuffer,
                .range = VK_WHOLE_SIZE,
            },
        },
    };
    vkrDesc_WriteBindings(NELEM(bufferBindings), bufferBindings);

    vkrTexTable_Write(set);

    ProfileEnd(pm_update);
}
