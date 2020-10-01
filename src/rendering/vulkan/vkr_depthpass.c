#include "rendering/vulkan/vkr_depthpass.h"
#include "rendering/vulkan/vkr_pass.h"
#include "rendering/vulkan/vkr_shader.h"
#include "rendering/vulkan/vkr_desc.h"
#include "rendering/vulkan/vkr_context.h"
#include "rendering/vulkan/vkr_swapchain.h"
#include "rendering/vulkan/vkr_cmd.h"

#include "rendering/drawable.h"
#include "rendering/mesh.h"
#include "rendering/camera.h"

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
            .size = sizeof(vkrOpaquePc),
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

typedef struct vkrTaskDraw
{
    task_t task;
    frus_t frustum;
    float4x4 worldToClip;
    vkrDepthPass* pass;
    VkRenderPass renderPass;
    vkrPassId subpass;
    const vkrSwapchain* chain;
    VkFramebuffer framebuffer;
    VkFence primaryFence;
    const drawables_t* drawables;
    VkCommandBuffer* buffers;
    float* distances;
} vkrTaskDraw;

static void vkrTaskDrawFn(void* pbase, i32 begin, i32 end)
{
    vkrTaskDraw* task = pbase;
    vkrDepthPass* pass = task->pass;
    const vkrSwapchain* chain = task->chain;
    VkRenderPass renderPass = task->renderPass;
    VkPipeline pipeline = pass->pass.pipeline;
    VkPipelineLayout layout = pass->pass.layout;
    vkrPassId subpass = task->subpass;
    VkFramebuffer framebuffer = task->framebuffer;
    VkFence primaryFence = task->primaryFence;
    const drawables_t* drawables = task->drawables;
    VkCommandBuffer* pim_noalias buffers = task->buffers;
    float* pim_noalias distances = task->distances;
    const VkPipelineBindPoint bindpoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    VkRect2D rect = vkrSwapchain_GetRect(chain);
    VkViewport viewport = vkrSwapchain_GetViewport(chain);

    const meshid_t* pim_noalias meshids = drawables->meshes;
    const float4x4* pim_noalias matrices = drawables->matrices;
    const box_t* pim_noalias localBounds = drawables->bounds;

    float4x4 worldToClip = task->worldToClip;
    frus_t frustum = task->frustum;
    plane_t fwd = frustum.z0;
    {
        // frustum has outward facing planes
        // we want forward plane for depth sorting draw calls
        float w = fwd.value.w;
        fwd.value = f4_neg(fwd.value);
        fwd.value.w = w;
    }

    vkrThreadContext* ctx = vkrContext_Get();
    for (i32 i = begin; i < end; ++i)
    {
        buffers[i] = NULL;
        distances[i] = 1 << 20;
        float4x4 localToWorld = matrices[i];
        box_t bounds = box_transform(localToWorld, localBounds[i]);
        if (sdFrusBox(frustum, bounds) > 0.0f)
        {
            continue;
        }
        mesh_t mesh;
        if (mesh_get(meshids[i], &mesh))
        {
            distances[i] = sdPlaneBox3D(fwd, bounds);
            VkCommandBuffer cmd = vkrContext_GetSecCmd(ctx, vkrQueueId_Gfx, primaryFence);
            buffers[i] = cmd;
            vkrCmdBeginSec(cmd, renderPass, subpass, framebuffer);
            vkrCmdViewport(cmd, viewport, rect);
            vkCmdBindPipeline(cmd, bindpoint, pipeline);
            const vkrDepthPc pushConsts =
            {
                .localToClip = f4x4_mul(worldToClip, localToWorld),
            };
            vkrCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT, &pushConsts, sizeof(pushConsts));
            const VkDeviceSize offset = {0};
            vkCmdBindVertexBuffers(cmd, 0, 1, &mesh.vkrmesh.buffer.handle, &offset);
            vkCmdDraw(cmd, mesh.length, 1, 0, 0);
            vkrCmdEnd(cmd);
        }
    }
}

ProfileMark(pm_execute, vkrDepthPass_Execute)
static vkrDepthPass_Execute(const vkrPassContext* ctx, vkrDepthPass* pass)
{
    ProfileBegin(pm_execute);

    const vkrSwapchain* chain = &g_vkr.chain;
    const drawables_t* drawables = drawables_get();
	i32 drawcount = drawables->count;
	const i32 width = chain->width;
	const i32 height = chain->height;
	const float aspect = (float)width / (float)height;

    camera_t camera;
    camera_get(&camera);
    float4 at = f4_add(camera.position, quat_fwd(camera.rotation));
    float4 up = quat_up(camera.rotation);
    float4x4 view = f4x4_lookat(camera.position, at, up);
    float4x4 proj = f4x4_vkperspective(f1_radians(camera.fovy), aspect, camera.zNear, camera.zFar);

    VkCommandBuffer* pim_noalias buffers = tmp_calloc(sizeof(buffers[0]) * drawcount);
    float* pim_noalias distances = tmp_calloc(sizeof(distances[0]) * drawcount);
    vkrTaskDraw* task = tmp_calloc(sizeof(*task));

    task->buffers = buffers;
    task->chain = chain;
    task->distances = distances;
    task->drawables = drawables;
    task->framebuffer = ctx->framebuffer;
    camera_frustum(&camera, &task->frustum, aspect);
    task->pass = pass;
    task->primaryFence = ctx->fence;
    task->renderPass = ctx->renderPass;
    task->subpass = ctx->subpass;
    task->worldToClip = f4x4_mul(proj, view);

    task_run(task, vkrTaskDrawFn, drawcount);

    for (i32 i = 0; i < drawcount; ++i)
    {
        if (!buffers[i])
        {
            buffers[i] = buffers[drawcount - 1];
            distances[i] = distances[drawcount - 1];
            --drawcount;
            --i;
        }
    }

    for (i32 i = 0; i < drawcount; ++i)
    {
        i32 c = i;
        for (i32 j = i + 1; j < drawcount; ++j)
        {
            if (distances[j] < distances[c])
            {
                c = j;
            }
        }
        if (c != i)
        {
            float t0 = distances[i];
            distances[i] = distances[c];
            distances[c] = t0;
            VkCommandBuffer t1 = buffers[i];
            buffers[i] = buffers[c];
            buffers[c] = t1;
        }
    }

    vkrCmdExecCmds(ctx->cmd, drawcount, buffers);

    ProfileEnd(pm_execute);
}