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
    const VkPushConstantRange ranges[] =
    {
        {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
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
        // normalOS and lightmap index
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
    };
    const VkVertexInputAttributeDescription vertAttributes[] =
    {
        // positionOS
        {
            .binding = 0,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .location = 0,
        },
        // normalOS and lightmap index
        {
            .binding = 1,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .location = 1,
        },
        // uv0 and uv1
        {
            .binding = 2,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .location = 2,
        },
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

static void vkrTaskDrawFn(void* pbase, i32 begin, i32 end)
{
    vkrTaskDraw* task = pbase;
    vkrOpaquePass* pass = task->pass;
    const vkrSwapchain* chain = task->chain;
    VkRenderPass renderPass = task->renderPass;
    VkPipeline pipeline = pass->pass.pipeline;
    VkPipelineLayout layout = pass->pass.layout;
    vkrPassId subpass = task->subpass;
    VkDescriptorSet descSet = task->descSet;
    VkFramebuffer framebuffer = task->framebuffer;
    VkFence primaryFence = task->primaryFence;
    const drawables_t* drawables = task->drawables;
    VkCommandBuffer* pim_noalias buffers = task->buffers;
    float* pim_noalias distances = task->distances;
    const VkPipelineBindPoint bindpoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    VkRect2D rect = vkrSwapchain_GetRect(chain);
    VkViewport viewport = vkrSwapchain_GetViewport(chain);
    const VkClearValue clearValues[] =
    {
        {
            .color = { 0.0f, 0.0f, 0.0f, 1.0f },
        },
        {
            .depthStencil = { 1.0f, 0 },
        },
    };

    const meshid_t* pim_noalias meshids = drawables->meshes;
    const material_t* pim_noalias materials = drawables->materials;
    const float4x4* pim_noalias matrices = drawables->matrices;
    const float3x3* pim_noalias invMatrices = drawables->invMatrices;
    const box_t* pim_noalias localBounds = drawables->bounds;
    const texture_t* pim_noalias textures = texture_table()->values;

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
        float4x4 matrix = matrices[i];
        box_t bounds = box_transform(matrix, localBounds[i]);
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
            vkrCmdBindDescSets(cmd, bindpoint, layout, 1, &descSet);

            i32 iAlbedo = materials[i].albedo.index;
            i32 iRome = materials[i].rome.index;
            i32 iNormal = materials[i].normal.index;
            iAlbedo = textures[iAlbedo].vkrtex.slot;
            iRome = textures[iRome].vkrtex.slot;
            iNormal = textures[iNormal].vkrtex.slot;
            const vkrOpaquePc pushConsts =
            {
                .localToWorld = matrix,
                .IMc0 = invMatrices[i].c0,
                .IMc1 = invMatrices[i].c1,
                .IMc2 = invMatrices[i].c2,
                .flatRome = materials[i].flatRome,
                .albedoIndex = iAlbedo,
                .romeIndex = iRome,
                .normalIndex = iNormal,
            };
            const u32 stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            vkrCmdPushConstants(cmd, layout, stages, &pushConsts, sizeof(pushConsts));
            vkrCmdDrawMesh(cmd, &mesh.vkrmesh);
            vkrCmdEnd(cmd);
        }
    }
}

ProfileMark(pm_execute, vkrOpaquePass_Execute)
static vkrOpaquePass_Execute(const vkrPassContext* ctx, vkrOpaquePass* pass)
{
    ProfileBegin(pm_execute);

	const vkrSwapchain* chain = &g_vkr.chain;
    const drawables_t* drawables = drawables_get();
	i32 drawcount = drawables->count;
	const i32 width = chain->width;
	const i32 height = chain->height;
	const float aspect = (float)width / (float)height;

    VkCommandBuffer* pim_noalias buffers = tmp_calloc(sizeof(buffers[0]) * drawcount);
    float* pim_noalias distances = tmp_calloc(sizeof(distances[0]) * drawcount);
    vkrTaskDraw* task = tmp_calloc(sizeof(*task));

    task->buffers = buffers;
    task->chain = chain;
    task->descSet = pass->pass.sets[ctx->syncIndex];
    task->distances = distances;
    task->drawables = drawables;
    task->framebuffer = ctx->framebuffer;
    camera_t camera;
    camera_get(&camera);
    camera_frustum(&camera, &task->frustum, aspect);
    task->pass = pass;
    task->primaryFence = ctx->fence;
    task->renderPass = ctx->renderPass;
    task->subpass = ctx->subpass;

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
            lmBegin = lmpack->lightmaps[0].vkrtex[0].slot;
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

    vkrTexTable_Write(&g_vkr.texTable, set);

    ProfileEnd(pm_update);
}
