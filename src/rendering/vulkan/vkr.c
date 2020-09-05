#include "rendering/vulkan/vkr.h"
#include "rendering/vulkan/vkr_compile.h"
#include "rendering/vulkan/vkr_instance.h"
#include "rendering/vulkan/vkr_debug.h"
#include "rendering/vulkan/vkr_device.h"
#include "rendering/vulkan/vkr_display.h"
#include "rendering/vulkan/vkr_swapchain.h"
#include "rendering/vulkan/vkr_shader.h"
#include "rendering/vulkan/vkr_pipeline.h"
#include "rendering/vulkan/vkr_renderpass.h"
#include "rendering/vulkan/vkr_cmd.h"
#include "rendering/vulkan/vkr_mem.h"
#include "rendering/vulkan/vkr_mesh.h"
#include "rendering/vulkan/vkr_context.h"
#include "rendering/vulkan/vkr_desc.h"
#include "rendering/vulkan/vkr_texture.h"
#include "rendering/drawable.h"
#include "rendering/mesh.h"
#include "rendering/texture.h"
#include "rendering/material.h"
#include "allocator/allocator.h"
#include "common/console.h"
#include "common/profiler.h"
#include "common/time.h"
#include "math/float4_funcs.h"
#include "math/float4x4_funcs.h"
#include "math/quat_funcs.h"
#include "threading/task.h"
#include "rendering/camera.h"
#include <string.h>

vkr_t g_vkr;
static vkrBinding ms_textureBindings[kTextureDescriptors];

bool vkr_init(i32 width, i32 height)
{
    bool success = true;
    char* shaderName = "brush.hlsl";
    char* shaderText = vkrLoadShader(shaderName);
    vkrCompileOutput vertOutput = { 0 };
    vkrCompileOutput fragOutput = { 0 };
    memset(&g_vkr, 0, sizeof(g_vkr));

    if (!vkrInstance_Init(&g_vkr))
    {
        success = false;
        goto cleanup;
    }

    if (!vkrDisplay_New(&g_vkr.display, width, height, "pimvk"))
    {
        success = false;
        goto cleanup;
    }

    if (!vkrDevice_Init(&g_vkr))
    {
        success = false;
        goto cleanup;
    }

    if (!vkrAllocator_New(&g_vkr.allocator))
    {
        success = false;
        goto cleanup;
    }

    if (!vkrSwapchain_New(&g_vkr.chain, &g_vkr.display, NULL))
    {
        success = false;
        goto cleanup;
    }

    if (!vkrContext_New(&g_vkr.context))
    {
        success = false;
        goto cleanup;
    }

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
            .format = g_vkr.chain.format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
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

    vkrSwapchain_SetupBuffers(&g_vkr.chain, renderPass);

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
            // per camera structured buffer
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        },
        {
            // albedo texture + sampler
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1024,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        },
    };
    if (!vkrPipelineLayout_AddSet(&pipeLayout, NELEM(bindings), bindings))
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
        &g_vkr.pipeline,
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

    u32 nullColor = 0x0;
    if (!vkrTexture2D_New(
        &g_vkr.nullTexture,
        1,
        1,
        VK_FORMAT_R8G8B8A8_SRGB,
        &nullColor,
        sizeof(nullColor)))
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
        vkr_shutdown();
    }
    return success;
}

ProfileMark(pm_update, vkr_update)
void vkr_update(void)
{
    if (!g_vkr.inst)
    {
        return;
    }

    if (!vkrDisplay_IsOpen(&g_vkr.display))
    {
        vkr_shutdown();
        return;
    }

    if (vkrDisplay_UpdateSize(&g_vkr.display))
    {
        vkrSwapchain_Recreate(
            &g_vkr.chain,
            &g_vkr.display,
            g_vkr.pipeline.renderPass);
    }
    vkrSwapchain* chain = &g_vkr.chain;
    if (!chain->handle)
    {
        return;
    }

    ProfileBegin(pm_update);

    const u32 syncIndex = vkrSwapchain_AcquireSync(chain);
    vkrAllocator_Update(&g_vkr.allocator);
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

    vkrPipeline* pipeline = &g_vkr.pipeline;
    const drawables_t* drawables = drawables_get();
    const i32 drawcount = drawables->count;
    const float4x4* pim_noalias localToWorlds = drawables->matrices;
    const float3x3* pim_noalias worldToLocals = drawables->invMatrices;
    const meshid_t* pim_noalias meshids = drawables->meshes;
    const material_t* pim_noalias materials = drawables->materials;

    vkrFrameContext* ctx = vkrContext_Get();
    vkrDescPool_Reset(ctx->descpool);
    VkDescriptorSet descSet = vkrDesc_New(ctx, pipeline->layout.sets[0]);
    {
        vkrBuffer* perdrawbuf = &g_vkr.context.perdrawbuf;
        vkrBuffer* percambuf = &g_vkr.context.percambuf;
        vkrBuffer_Reserve(perdrawbuf, sizeof(vkrPerDraw) * drawcount, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, vkrMemUsage_CpuToGpu, NULL, PIM_FILELINE);
        vkrBuffer_Reserve(percambuf, sizeof(vkrPerCamera), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, vkrMemUsage_CpuToGpu, NULL, PIM_FILELINE);
        const vkrBinding bufferBindings[] =
        {
            {
                .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .set = descSet,
                .binding = 0,
                .buffer = *perdrawbuf,
            },
            {
                .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .set = descSet,
                .binding = 1,
                .buffer = *percambuf,
            },
        };
        vkrDesc_WriteBindings(ctx, NELEM(bufferBindings), bufferBindings);

        const vkrTexture2D nullTexture = g_vkr.nullTexture;
        vkrBinding* texBindings = ms_textureBindings;
        for (i32 i = 0; i < kTextureDescriptors; ++i)
        {
            texBindings[i].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            texBindings[i].set = descSet;
            texBindings[i].binding = 2;
            texBindings[i].arrayElem = i;
            texBindings[i].texture = nullTexture;
            if (i < drawcount)
            {
                texture_t texture = { 0 };
                if (texture_get(materials[i].albedo, &texture))
                {
                    texBindings[i].texture = texture.vkrtex;
                }
            }
        }
        vkrDesc_WriteBindings(ctx, kTextureDescriptors, texBindings);

        float aspect = (float)chain->width / chain->height;
        camera_t camera;
        camera_get(&camera);
        float4 at = f4_add(camera.position, quat_fwd(camera.rotation));
        float4 up = quat_up(camera.rotation);
        float4x4 view = f4x4_lookat(camera.position, at, up);
        float4x4 proj = f4x4_perspective(f1_radians(camera.fovy), aspect, camera.zNear, camera.zFar);
        f4x4_11(proj) *= -1.0f;
        {
            vkrPerCamera* perCamera = vkrBuffer_Map(percambuf);
            ASSERT(perCamera);
            perCamera->worldToCamera = view;
            perCamera->cameraToClip = proj;
            vkrBuffer_Unmap(percambuf);
        }

        {
            vkrPerDraw* perDraws = vkrBuffer_Map(perdrawbuf);
            ASSERT(perDraws);
            for (i32 i = 0; i < drawcount; ++i)
            {
                perDraws[i].localToWorld = localToWorlds[i];
                perDraws[i].worldToLocal.c0 = worldToLocals[i].c0;
                perDraws[i].worldToLocal.c1 = worldToLocals[i].c1;
                perDraws[i].worldToLocal.c2 = worldToLocals[i].c2;
                perDraws[i].worldToLocal.c3 = f4_v(0.0f, 0.0f, 0.0f, 1.0f);
                float4 st = materials[i].st;
                perDraws[i].textureScale = f4_v(st.x, st.y, 1.0f, 1.0f);
                perDraws[i].textureBias = f4_v(st.z, st.w, 0.0f, 0.0f);
            }
            vkrBuffer_Unmap(perdrawbuf);
        }

        vkrBuffer_Flush(percambuf);
        vkrBuffer_Flush(perdrawbuf);
    }
    ASSERT(descSet);

    VkCommandBuffer cmd = NULL;
    VkQueue queue = NULL;
    vkrContext_GetCmd(ctx, vkrQueueId_Gfx, &cmd, NULL, &queue);
    {
        vkrCmdBegin(cmd);
        {
            vkrCmdViewport(cmd, viewport, rect);

            const u32 imageIndex = vkrSwapchain_AcquireImage(chain);
            vkrCmdBeginRenderPass(
                cmd,
                pipeline->renderPass,
                chain->buffers[imageIndex],
                rect,
                NELEM(clearValues),
                clearValues);
            vkrCmdBindPipeline(cmd, pipeline);
            vkrCmdBindDescSets(cmd, pipeline, 1, &descSet);

            for (i32 i = 0; i < drawcount; ++i)
            {
                mesh_t mesh;
                if (mesh_get(meshids[i], &mesh))
                {
                    const vkrPushConstants pushConsts =
                    {
                        .drawIndex = i,
                        .cameraIndex = 0,
                    };
                    vkrCmdPushConstants(cmd, pipeline, &pushConsts, sizeof(pushConsts));
                    vkrCmdDrawMesh(cmd, &mesh.vkrmesh);
                }
            }

            vkrCmdEndRenderPass(cmd);
        }
        vkrCmdEnd(cmd);
    }
    vkrSwapchain_Present(chain, queue, cmd);

    ProfileEnd(pm_update);
}

void vkr_shutdown(void)
{
    if (g_vkr.inst)
    {
        vkrDevice_WaitIdle();

        vkrTexture2D_Del(&g_vkr.nullTexture);

        mesh_sys_vkfree();
        texture_sys_vkfree();
        vkrPipeline_Del(&g_vkr.pipeline);

        vkrContext_Del(&g_vkr.context);
        vkrSwapchain_Del(&g_vkr.chain);
        vkrAllocator_Del(&g_vkr.allocator);
        vkrDevice_Shutdown(&g_vkr);
        vkrDisplay_Del(&g_vkr.display);
        vkrInstance_Shutdown(&g_vkr);
    }
}

