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
#include "common/atomics.h"
#include "common/cvar.h"
#include "math/float4x4_funcs.h"
#include "threading/task.h"
#include <string.h>

// ----------------------------------------------------------------------------

typedef struct vkrTaskDraw
{
    task_t task;
    const vkrPipeline* pipeline;
    const vkrSwapchain* chain;
    VkDescriptorSet descSet;
    VkFramebuffer framebuffer;
    VkFence primaryFence;
    const drawables_t* drawables;
    VkCommandBuffer* buffers;
    i32 buffercount;
} vkrTaskDraw;

// ----------------------------------------------------------------------------

static VkDescriptorSet vkrMainPass_UpdateDescriptors(VkCommandBuffer cmd);
static VkCommandBuffer vkrDrawImGui(VkRenderPass renderPass, VkFence fence);
static void vkrTaskDrawFn(void* pbase, i32 begin, i32 end);

// ----------------------------------------------------------------------------

static cvar_t* cv_pt_trace;

// ----------------------------------------------------------------------------

bool vkrMainPass_New(vkrPipeline* pipeline)
{
    ASSERT(pipeline);

    cv_pt_trace = cvar_find("pt_trace");
    ASSERT(cv_pt_trace);

    bool success = true;
    char* shaderName = "brush.hlsl";
    char* shaderText = vkrLoadShader(shaderName);
    vkrCompileOutput vertOutput = { 0 };
    vkrCompileOutput fragOutput = { 0 };

    const vkrCompileInput vertInput =
    {
        .filename = shaderName,
        .entrypoint = "VSMain",
        .text = shaderText,
        .type = vkrShaderType_Vert,
        .compile = true,
        .disassemble = true,
    };
    vkrCompile(&vertInput, &vertOutput);
    if (vertOutput.errors)
    {
        con_logf(LogSev_Error, "Vkc", "Errors while compiling %s %s", vertInput.filename, vertInput.entrypoint);
        con_logf(LogSev_Error, "Vkc", "%s", vertOutput.errors);
        ASSERT(false);
        success = false;
        goto cleanup;
    }
    if (vertOutput.disassembly)
    {
        con_logf(LogSev_Info, "Vkc", "Dissassembly of %s %s", vertInput.filename, vertInput.entrypoint);
        con_logf(LogSev_Info, "Vkc", "%s", vertOutput.disassembly);
    }

    const vkrCompileInput fragInput =
    {
        .filename = shaderName,
        .entrypoint = "PSMain",
        .text = shaderText,
        .type = vkrShaderType_Frag,
        .compile = true,
        .disassemble = true,
    };
    vkrCompile(&fragInput, &fragOutput);
    if (fragOutput.errors)
    {
        con_logf(LogSev_Error, "Vkc", "Errors while compiling %s %s", fragInput.filename, fragInput.entrypoint);
        con_logf(LogSev_Error, "Vkc", "%s", fragOutput.errors);
        ASSERT(false);
        success = false;
        goto cleanup;
    }
    if (fragOutput.disassembly)
    {
        con_logf(LogSev_Info, "Vkc", "Dissassembly of %s %s", fragInput.filename, fragInput.entrypoint);
        con_logf(LogSev_Info, "Vkc", "%s", fragOutput.disassembly);
    }

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
        .attachmentCount = 1,
        .attachments[0] =
        {
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
            .blendEnable = false,
        },
    };

    const vkrVertexLayout vertLayout =
    {
        .streamCount = vkrMeshStream_COUNT,
        .types[vkrMeshStream_Position] = vkrVertType_float4,
        .types[vkrMeshStream_Normal] = vkrVertType_float4,
        .types[vkrMeshStream_Uv01] = vkrVertType_float4,
    };

    VkPipelineShaderStageCreateInfo shaders[] =
    {
        vkrCreateShader(&vertOutput),
        vkrCreateShader(&fragOutput),
    };

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
            .format = g_vkr.chain.depthFormat,
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
    };
    const VkAttachmentReference depthAttachRef =
    {
        .attachment = 1,
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
    if (!renderPass)
    {
        success = false;
        goto cleanup;
    }
    const i32 subpass = 0;

    vkrPipelineLayout pipeLayout = { 0 };
    vkrPipelineLayout_New(&pipeLayout);
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
    if (!vkrPipelineLayout_AddSet(
        &pipeLayout,
        NELEM(bindings), bindings,
        0x0))
    {
        success = false;
        goto cleanup;
    }
    const VkPushConstantRange range =
    {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = sizeof(vkrPushConstants),
    };
    vkrPipelineLayout_AddRange(&pipeLayout, range);

    if (!vkrPipeline_NewGfx(
        pipeline,
        &ffuncs,
        &vertLayout,
        &pipeLayout,
        renderPass,
        subpass,
        NELEM(shaders), shaders))
    {
        success = false;
        goto cleanup;
    }

cleanup:
    vkrCompileOutput_Del(&vertOutput);
    vkrCompileOutput_Del(&fragOutput);
    for (i32 i = 0; i < NELEM(shaders); ++i)
    {
        vkrDestroyShader(shaders + i);
    }
    pim_free(shaderText);
    shaderText = NULL;
    if (!success)
    {
        vkrMainPass_Del(pipeline);
    }
    return success;
}

void vkrMainPass_Del(vkrPipeline* pipeline)
{
    vkrPipeline_Del(pipeline);
}

ProfileMark(pm_draw, vkrMainPass_Draw)
void vkrMainPass_Draw(
    vkrPipeline* pipeline,
    VkCommandBuffer cmd,
    VkFence fence)
{
    ProfileBegin(pm_draw);

    VkDescriptorSet descSet = vkrMainPass_UpdateDescriptors(cmd);

    const bool r_sw = cvar_get_bool(cv_pt_trace);

    vkrSwapchain *const chain = &g_vkr.chain;
    VkRenderPass renderPass = pipeline->renderPass;

    const drawables_t* drawables = drawables_get();
    const i32 drawcount = drawables->count;
    VkCommandBuffer* seccmds = tmp_calloc(sizeof(seccmds[0]) * drawcount);
    vkrTaskDraw* task = tmp_calloc(sizeof(*task));
    task->drawables = drawables;
    task->chain = chain;
    task->pipeline = pipeline;
    task->framebuffer = NULL;
    task->primaryFence = fence;
    task->descSet = descSet;
    task->buffers = seccmds;
    task->buffercount = 0;
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
            .depthStencil = { 1.0f, 0 },
        },
    };
    vkrCmdViewport(cmd, viewport, rect);
    vkrCmdBindPipeline(cmd, pipeline);
    vkrCmdBindDescSets(cmd, pipeline, 1, &descSet);

    VkFramebuffer framebuffer = NULL;
    u32 imageIndex = vkrSwapchain_AcquireImage(chain, &framebuffer);
    if (r_sw)
    {
        const framebuf_t *const swfb = render_sys_frontbuf();
        screenblit_blit(
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
        const i32 seccmdcount = task->buffercount;
        vkrCmdExecCmds(cmd, seccmdcount, seccmds);
    }

    vkrCmdExecCmds(cmd, 1, &igcmd);

    vkrCmdEndRenderPass(cmd);

    ProfileEnd(pm_draw);
}

ProfileMark(pm_updatedesc, vkrMainPass_UpdateDescriptors)
static VkDescriptorSet vkrMainPass_UpdateDescriptors(VkCommandBuffer cmd)
{
    ProfileBegin(pm_updatedesc);

    vkrSwapchain* chain = &g_vkr.chain;
    vkrPipeline* pipeline = &g_vkr.mainPass;
    const u32 syncIndex = g_vkr.chain.syncIndex;
    const drawables_t* drawables = drawables_get();
    const i32 drawcount = drawables->count;
    const float4x4* pim_noalias localToWorlds = drawables->matrices;
    const float3x3* pim_noalias worldToLocals = drawables->invMatrices;
    const meshid_t* pim_noalias meshids = drawables->meshes;
    const material_t* pim_noalias materials = drawables->materials;

    vkrThreadContext* ctx = vkrContext_Get();
    vkrDescPool_Reset(ctx->descpools[syncIndex]);
    VkDescriptorSet descSet = vkrDesc_New(ctx, pipeline->layout.sets[0]);
    {
        vkrBuffer* percamstage = &g_vkr.context.percamstage;
        vkrBuffer* percambuf = &g_vkr.context.percambuf;

        {
            vkrBuffer_Reserve(percamstage, sizeof(vkrPerCamera), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, vkrMemUsage_CpuOnly, NULL, PIM_FILELINE);
            vkrBuffer_Reserve(percambuf, sizeof(vkrPerCamera), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, vkrMemUsage_GpuOnly, NULL, PIM_FILELINE);

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
                vkrPerCamera* perCamera = vkrBuffer_Map(percamstage);
                ASSERT(perCamera);
                perCamera->worldToClip = f4x4_mul(proj, view);
                perCamera->eye = camera.position;
                for (i32 i = 0; i < kGiDirections; ++i)
                {
                    perCamera->giAxii[i] = lmpack->axii[i];
                }
                perCamera->lmBegin = tableWidth;
                vkrBuffer_Unmap(percamstage);
            }

            vkrBuffer_Flush(percamstage);

            vkrCmdCopyBuffer(cmd, *percamstage, *percambuf);
            VkBufferMemoryBarrier barrier =
            {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .buffer = percambuf->handle,
                .offset = 0,
                .size = VK_WHOLE_SIZE,
            };
            vkrCmdBufferBarrier(
                cmd,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                &barrier);
        }

        const vkrBinding bufferBindings[] =
        {
            {
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .set = descSet,
                .binding = 1,
                .buffer =
                {
                    .buffer = percambuf->handle,
                    .range = VK_WHOLE_SIZE,
                },
            },
        };
        vkrDesc_WriteBindings(ctx, NELEM(bufferBindings), bufferBindings);

        const table_t* textable = texture_table();
        const i32 tableWidth = textable->width;
        const texture_t* textures = textable->values;
        const vkrTexture2D nullTexture = g_vkr.nullTexture;
        vkrBinding* texBindings = g_vkr.bindings;
        ASSERT(tableWidth <= kTextureDescriptors);
        for (i32 i = 0; i < kTextureDescriptors; ++i)
        {
            texBindings[i].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            texBindings[i].set = descSet;
            texBindings[i].binding = 2;
            texBindings[i].arrayElem = i;
            const vkrTexture2D* texture = &nullTexture;
            if ((i < tableWidth) && textures[i].texels)
            {
                texture = &textures[i].vkrtex;
            }
            texBindings[i].image.sampler = texture->sampler;
            texBindings[i].image.imageView = texture->view;
            texBindings[i].image.imageLayout = texture->layout;
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
                            texBindings[iBinding].image.sampler = texture->sampler;
                            texBindings[iBinding].image.imageView = texture->view;
                            texBindings[iBinding].image.imageLayout = texture->layout;
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
        vkrDesc_WriteBindings(ctx, kTextureDescriptors, texBindings);
    }

    ProfileEnd(pm_updatedesc);

    ASSERT(descSet);
    return descSet;
}

static void vkrTaskDrawFn(void* pbase, i32 begin, i32 end)
{
    vkrTaskDraw* task = pbase;
    const vkrPipeline* pipeline = task->pipeline;
    const vkrSwapchain* chain = task->chain;
    VkRenderPass renderPass = pipeline->renderPass;
    const i32 subpass = pipeline->subpass;
    VkDescriptorSet descSet = task->descSet;
    VkFramebuffer framebuffer = task->framebuffer;
    VkFence primaryFence = task->primaryFence;
    VkPipelineLayout layout = pipeline->layout.handle;
    const drawables_t* drawables = task->drawables;
    VkCommandBuffer* buffers = task->buffers;

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

    vkrThreadContext* ctx = vkrContext_Get();
    VkCommandBuffer cmd = vkrContext_GetSecCmd(ctx, vkrQueueId_Gfx, primaryFence);
    {
        i32 bufferindex = inc_i32(&task->buffercount, MO_AcqRel);
        ASSERT(bufferindex >= 0);
        ASSERT(bufferindex < drawables->count);
        buffers[bufferindex] = cmd;
    }
    vkrCmdBeginSec(cmd, renderPass, subpass, framebuffer);
    {
        vkrCmdViewport(cmd, viewport, rect);
        vkrCmdBindPipeline(cmd, pipeline);
        vkrCmdBindDescSets(cmd, pipeline, 1, &descSet);
        for (i32 i = begin; i < end; ++i)
        {
            mesh_t mesh;
            if (mesh_get(meshids[i], &mesh))
            {
                const vkrPushConstants pushConsts =
                {
                    .localToWorld = matrices[i],
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
            }
        }
    }
    vkrCmdEnd(cmd);
}

static VkCommandBuffer vkrDrawImGui(VkRenderPass renderPass, VkFence fence)
{
    VkCommandBuffer cmd = vkrContext_GetSecCmd(vkrContext_Get(), vkrQueueId_Gfx, fence);
    vkrCmdBeginSec(cmd, renderPass, 0, NULL);
    vkrImGuiPass_Draw(&g_vkr.imguiPass, cmd);
    vkrCmdEnd(cmd);
    return cmd;
}
