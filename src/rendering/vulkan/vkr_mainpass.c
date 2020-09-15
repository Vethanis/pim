#include "rendering/vulkan/vkr_mainpass.h"
#include "rendering/vulkan/vkr_compile.h"
#include "rendering/vulkan/vkr_shader.h"
#include "rendering/vulkan/vkr_pipeline.h"
#include "rendering/vulkan/vkr_renderpass.h"
#include "rendering/vulkan/vkr_desc.h"
#include "rendering/vulkan/vkr_swapchain.h"
#include "rendering/vulkan/vkr_context.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_buffer.h"
#include "rendering/vulkan/vkr_imgui.h"
#include "rendering/drawable.h"
#include "rendering/render_system.h"
#include "rendering/framebuffer.h"
#include "rendering/texture.h"
#include "rendering/mesh.h"
#include "rendering/material.h"
#include "rendering/lightmap.h"
#include "rendering/camera.h"
#include "rendering/screenblit.h"
#include "containers/table.h"
#include "allocator/allocator.h"
#include "common/console.h"
#include "common/profiler.h"
#include "common/cvar.h"
#include "math/float4x4_funcs.h"
#include "math/box.h"
#include "math/frustum.h"
#include "math/sdf.h"
#include "threading/task.h"
#include <string.h>

// ----------------------------------------------------------------------------

typedef struct vkrTaskDraw
{
    task_t task;
    frus_t frustum;
    vkrMainPass* pass;
    const vkrSwapchain* chain;
    VkDescriptorSet descSet;
    VkFramebuffer framebuffer;
    VkFence primaryFence;
    const drawables_t* drawables;
    VkCommandBuffer* buffers;
    float* distances;
    i32 subpass;
} vkrTaskDraw;

// ----------------------------------------------------------------------------

static VkDescriptorSet vkrMainPass_UpdateDescriptors(vkrMainPass* pass, VkCommandBuffer cmd);
static VkCommandBuffer vkrDrawImGui(VkRenderPass renderPass, VkFence fence);
static void vkrTaskDrawFn(void* pbase, i32 begin, i32 end);

// ----------------------------------------------------------------------------

static cvar_t* cv_pt_trace;

// ----------------------------------------------------------------------------

bool vkrMainPass_New(vkrMainPass* pass)
{
    ASSERT(pass);

    cv_pt_trace = cvar_find("pt_trace");
    ASSERT(cv_pt_trace);

    bool success = true;

    const vkrVertexLayout vertLayout =
    {
        .streamCount = vkrMeshStream_COUNT,
        .types[vkrMeshStream_Position] = vkrVertType_float4,
        .types[vkrMeshStream_Normal] = vkrVertType_float4,
        .types[vkrMeshStream_Uv01] = vkrVertType_float4,
    };

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

    const VkAttachmentDescription attachments[] =
    {
        {
            .format = g_vkr.chain.colorFormat,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        },
        {
            .format = g_vkr.chain.lumAttachments[0].format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        },
        {
            .format = g_vkr.chain.depthAttachment.format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        },
    };
    const VkAttachmentReference colorAttachRefs[] =
    {
        {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        },
        {
            .attachment = 1,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        },
    };
    const VkAttachmentReference depthAttachRef =
    {
        .attachment = 2,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };
    const VkSubpassDescription subpasses[] =
    {
        {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = NELEM(colorAttachRefs),
            .pColorAttachments = colorAttachRefs,
            .pDepthStencilAttachment = &depthAttachRef,
        },
    };
    const VkSubpassDependency dependencies[] =
    {
        {
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = 0,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        },
    };

    VkRenderPass renderPass = vkrRenderPass_New(
        NELEM(attachments), attachments,
        NELEM(subpasses), subpasses,
        NELEM(dependencies), dependencies);
    pass->renderPass = renderPass;
    if (!renderPass)
    {
        success = false;
        goto cleanup;
    }
    const i32 subpass = 0;

    const vkrFixedFuncs ffuncs =
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
        .attachmentCount = NELEM(colorAttachRefs),
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
    };

    const VkDescriptorSetLayoutBinding bindings[] =
    {
        {
            // per draw  structured buffer
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        },
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
    };
    VkDescriptorSetLayout setLayout = vkrSetLayout_New(NELEM(bindings), bindings, 0x0);
    pass->setLayout = setLayout;
    if (!setLayout)
    {
        success = false;
        goto cleanup;
    }
    const VkPushConstantRange range =
    {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = sizeof(vkrMainPassConstants),
    };
    VkPipelineLayout layout = vkrPipelineLayout_New(1, &setLayout, 1, &range);
    pass->layout = layout;
    if (!layout)
    {
        success = false;
        goto cleanup;
    }

    VkPipeline pipeline = vkrPipeline_NewGfx(
        &ffuncs,
        &vertLayout,
        layout,
        renderPass,
        subpass,
        NELEM(shaders), shaders);
    pass->pipeline = pipeline;
    if (!pipeline)
    {
        success = false;
        goto cleanup;
    }

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

    {
        const VkDescriptorPoolSize sizes[] =
        {
            {
                .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = 1,
            },
            {
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
            },
            {
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = kTextureDescriptors,
            },
        };
        VkDescriptorPool descPool = vkrDescPool_New(kFramesInFlight, NELEM(sizes), sizes);
        pass->descPool = descPool;
        if (!descPool)
        {
            success = false;
            goto cleanup;
        }
    }

    for (i32 i = 0; i < kFramesInFlight; ++i)
    {
        pass->sets[i] = vkrDesc_New(pass->descPool, setLayout);
        if (!pass->sets[i])
        {
            success = false;
            goto cleanup;
        }
    }

cleanup:
    vkrShader_Del(&shaders[0]);
    vkrShader_Del(&shaders[1]);
    if (!success)
    {
        vkrMainPass_Del(pass);
    }
    return success;
}

void vkrMainPass_Del(vkrMainPass* pass)
{
    if (pass)
    {
        vkrRenderPass_Del(pass->renderPass);
        vkrPipeline_Del(pass->pipeline);
        vkrPipelineLayout_Del(pass->layout);
        vkrSetLayout_Del(pass->setLayout);
        vkrDescPool_Del(pass->descPool);
        for (i32 i = 0; i < kFramesInFlight; ++i)
        {
            vkrBuffer_Del(&pass->perCameraBuffer[i]);
        }
        memset(pass, 0, sizeof(*pass));
    }
}

ProfileMark(pm_draw, vkrMainPass_Draw)
void vkrMainPass_Draw(
    vkrMainPass* pass,
    VkCommandBuffer cmd,
    VkFence fence)
{
    ProfileBegin(pm_draw);

    VkDescriptorSet descSet = vkrMainPass_UpdateDescriptors(pass, cmd);

    const bool r_sw = cvar_get_bool(cv_pt_trace);

    vkrSwapchain *const chain = &g_vkr.chain;
    VkRenderPass renderPass = pass->renderPass;
    VkPipeline pipeline = pass->pipeline;
    VkPipelineLayout layout = pass->layout;
    const VkPipelineBindPoint bindpoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    const drawables_t* drawables = drawables_get();
    const i32 drawcount = drawables->count;

    camera_t camera;
    camera_get(&camera);

    VkCommandBuffer* seccmds = tmp_calloc(sizeof(seccmds[0]) * drawcount);
    float* distances = tmp_calloc(sizeof(distances[0]) * drawcount);
    vkrTaskDraw* task = tmp_calloc(sizeof(*task));
    camera_frustum(&camera, &task->frustum);
    task->drawables = drawables;
    task->distances = distances;
    task->chain = chain;
    task->pass = pass;
    task->framebuffer = NULL;
    task->primaryFence = fence;
    task->descSet = descSet;
    task->buffers = seccmds;

    if (!r_sw)
    {
        task_submit(task, vkrTaskDrawFn, drawcount);
        task_sys_schedule();
    }

    VkCommandBuffer igcmd = vkrDrawImGui(renderPass, fence);

    VkRect2D rect = vkrSwapchain_GetRect(chain);
    VkViewport viewport = vkrSwapchain_GetViewport(chain);
    const VkClearValue clearValues[] =
    {
        {
            .color = { 0.0f, 0.0f, 0.0f, 1.0f },
        },
        {
            .color = { 0.0f, 0.0f, 0.0f, 1.0f },
        },
        {
            .depthStencil = { 1.0f, 0 },
        },
    };
    vkrCmdViewport(cmd, viewport, rect);
    vkCmdBindPipeline(cmd, bindpoint, pipeline);
    vkrCmdBindDescSets(cmd, bindpoint, layout, 1, &descSet);

    VkFramebuffer framebuffer = NULL;
    u32 imageIndex = vkrSwapchain_AcquireImage(chain, &framebuffer);
    if (r_sw)
    {
        const framebuf_t *const swfb = render_sys_frontbuf();
        vkrScreenBlit_Blit(
            &g_vkr.screenBlit,
            cmd,
            chain->images[imageIndex],
            swfb->color,
            swfb->width,
            swfb->height);
    }
    else
    {
        const VkImageMemoryBarrier barrier =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = 0x0,
            .dstAccessMask = 0x0,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = chain->images[imageIndex],
            .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1,
            },
        };
        vkrCmdImageBarrier(cmd,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            &barrier);
    }
    vkrCmdBeginRenderPass(
        cmd,
        renderPass,
        framebuffer,
        rect,
        NELEM(clearValues),
        clearValues,
        VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

    if (!r_sw)
    {
        task_await(task);
        VkCommandBuffer* pim_noalias buffers = task->buffers;
        float* pim_noalias distances = task->distances;
        i32 drawcount = drawables->count;
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
        // TODO: use something faster than bubble sort
        bool unsorted = true;
        while (unsorted)
        {
            unsorted = false;
            for (i32 i = 1; i < drawcount; ++i)
            {
                float a = distances[i - 1];
                float b = distances[i];
                if (a > b)
                {
                    distances[i - 1] = b;
                    distances[i] = a;
                    VkCommandBuffer t = buffers[i-1];
                    buffers[i-1] = buffers[i];
                    buffers[i] = t;
                    unsorted = true;
                }
            }
        }
        vkrCmdExecCmds(cmd, drawcount, seccmds);
    }

    vkrCmdExecCmds(cmd, 1, &igcmd);

    vkrCmdEndRenderPass(cmd);

    ProfileEnd(pm_draw);
}

ProfileMark(pm_updatedesc, vkrMainPass_UpdateDescriptors)
static VkDescriptorSet vkrMainPass_UpdateDescriptors(
    vkrMainPass* pass,
    VkCommandBuffer cmd)
{
    ProfileBegin(pm_updatedesc);

    vkrSwapchain* chain = &g_vkr.chain;
    const u32 syncIndex = g_vkr.chain.syncIndex;
    const drawables_t* drawables = drawables_get();
    const i32 drawcount = drawables->count;
    const float4x4* pim_noalias localToWorlds = drawables->matrices;
    const float3x3* pim_noalias worldToLocals = drawables->invMatrices;
    const meshid_t* pim_noalias meshids = drawables->meshes;
    const material_t* pim_noalias materials = drawables->materials;

    vkrThreadContext* ctx = vkrContext_Get();
    VkDescriptorSet set = pass->sets[syncIndex];
    ASSERT(set);
    {
        vkrBuffer* percambuf = &pass->perCameraBuffer[syncIndex];
        {
            float aspect = (float)chain->width / chain->height;
            camera_t camera;
            camera_get(&camera);
            float4 at = f4_add(camera.position, quat_fwd(camera.rotation));
            float4 up = quat_up(camera.rotation);
            float4x4 view = f4x4_lookat(camera.position, at, up);
            float4x4 proj = f4x4_perspective(f1_radians(camera.fovy), aspect, camera.zNear, camera.zFar);
            f4x4_11(proj) *= -1.0f;
            const lmpack_t* lmpack = lmpack_get();
            const table_t* textable = texture_table();
            const i32 tableWidth = textable->width;
            {
                vkrPerCamera* perCamera = vkrBuffer_Map(percambuf);
                ASSERT(perCamera);
                perCamera->worldToClip = f4x4_mul(proj, view);
                perCamera->eye = camera.position;
                perCamera->exposure = g_vkr.exposurePass.params.exposure;
                for (i32 i = 0; i < kGiDirections; ++i)
                {
                    perCamera->giAxii[i] = lmpack->axii[i];
                }
                perCamera->lmBegin = tableWidth;
                vkrBuffer_Unmap(percambuf);
                vkrBuffer_Flush(percambuf);
            }
        }

        const vkrBinding bufferBindings[] =
        {
            {
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .set = set,
                .binding = 1,
                .buffer =
                {
                    .buffer = percambuf->handle,
                    .range = VK_WHOLE_SIZE,
                },
            },
        };
        vkrDesc_WriteBindings(NELEM(bufferBindings), bufferBindings);

        const table_t* textable = texture_table();
        const i32 tableWidth = textable->width;
        const texture_t* textures = textable->values;
        const vkrTexture2D nullTexture = g_vkr.nullTexture;
        VkDescriptorImageInfo* texBindings = g_vkr.texTable;
        ASSERT(tableWidth <= kTextureDescriptors);
        for (i32 i = 0; i < kTextureDescriptors; ++i)
        {
            const vkrTexture2D* texture = &nullTexture;
            if ((i < tableWidth) && textures[i].texels)
            {
                texture = &textures[i].vkrtex;
            }
            texBindings[i].sampler = texture->sampler;
            texBindings[i].imageView = texture->view;
            texBindings[i].imageLayout = texture->layout;
        }
        {
            const lmpack_t* lmpack = lmpack_get();
            i32 iBinding = tableWidth;
            for (i32 ilm = 0; ilm < lmpack->lmCount; ++ilm)
            {
                const lightmap_t* lm = lmpack->lightmaps + ilm;
                for (i32 idir = 0; idir < kGiDirections; ++idir)
                {
                    if (iBinding < kTextureDescriptors)
                    {
                        const vkrTexture2D* texture = &lm->vkrtex[idir];
                        if (texture->layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
                        {
                            texBindings[iBinding].sampler = texture->sampler;
                            texBindings[iBinding].imageView = texture->view;
                            texBindings[iBinding].imageLayout = texture->layout;
                        }
                    }
                    else
                    {
                        ASSERT(false);
                    }
                    ++iBinding;
                }
            }
        }
        vkrDesc_WriteImageTable(
            set,
            2,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            kTextureDescriptors,
            texBindings);
    }

    ProfileEnd(pm_updatedesc);

    return set;
}

static void vkrTaskDrawFn(void* pbase, i32 begin, i32 end)
{
    vkrTaskDraw* task = pbase;
    vkrMainPass* pass = task->pass;
    const vkrSwapchain* chain = task->chain;
    VkRenderPass renderPass = pass->renderPass;
    VkPipeline pipeline = pass->pipeline;
    VkPipelineLayout layout = pass->layout;
    const i32 subpass = task->subpass;
    VkDescriptorSet descSet = task->descSet;
    VkFramebuffer framebuffer = task->framebuffer;
    VkFence primaryFence = task->primaryFence;
    const drawables_t* drawables = task->drawables;
    VkCommandBuffer* buffers = task->buffers;
    float* distances = task->distances;
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
        box_t bounds = box_transform(matrices[i], localBounds[i]);
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
            const vkrMainPassConstants pushConsts =
            {
                .localToWorld = matrix,
                .IMc0 = invMatrices[i].c0,
                .IMc1 = invMatrices[i].c1,
                .IMc2 = invMatrices[i].c2,
                .flatRome = materials[i].flatRome,
                .albedoIndex = materials[i].albedo.index,
                .romeIndex = materials[i].rome.index,
                .normalIndex = materials[i].normal.index,
            };
            const u32 stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            vkrCmdPushConstants(cmd, layout, stages, &pushConsts, sizeof(pushConsts));
            vkrCmdDrawMesh(cmd, &mesh.vkrmesh);
            vkrCmdEnd(cmd);
        }
    }
}

static VkCommandBuffer vkrDrawImGui(VkRenderPass renderPass, VkFence fence)
{
    VkCommandBuffer cmd = vkrContext_GetSecCmd(vkrContext_Get(), vkrQueueId_Gfx, fence);
    vkrCmdBeginSec(cmd, renderPass, 0, NULL);
    vkrImGuiPass_Draw(&g_vkr.imguiPass, cmd);
    vkrCmdEnd(cmd);
    return cmd;
}
