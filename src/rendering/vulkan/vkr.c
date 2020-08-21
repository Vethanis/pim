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
#include "allocator/allocator.h"
#include "common/console.h"
#include <string.h>

vkr_t g_vkr;

bool vkr_init(i32 width, i32 height)
{
    memset(&g_vkr, 0, sizeof(g_vkr));

    if (!vkrInstance_Init(&g_vkr))
    {
        return false;
    }

    g_vkr.display = vkrDisplay_New(width, height, "pimvk");
    if (!g_vkr.display)
    {
        return false;
    }

    if (!vkrDevice_Init(&g_vkr))
    {
        return false;
    }

    g_vkr.chain = vkrSwapchain_New(g_vkr.display, NULL);
    if (!g_vkr.chain)
    {
        return false;
    }

    char* shaderName = "first_tri.hlsl";
    char* firstTri = vkrLoadShader(shaderName);

    const vkrCompileInput vertInput =
    {
        .filename = shaderName,
        .entrypoint = "VSMain",
        .text = firstTri,
        .type = vkrShaderType_Vert,
        .compile = true,
        .disassemble = true,
    };
    vkrCompileOutput vertOutput = { 0 };
    if (vkrCompile(&vertInput, &vertOutput))
    {
        if (vertOutput.errors)
        {
            con_logf(LogSev_Error, "Vkc", "Errors while compiling %s %s", vertInput.filename, vertInput.entrypoint);
            con_logf(LogSev_Error, "Vkc", "%s", vertOutput.errors);
        }
        if (vertOutput.disassembly)
        {
            con_logf(LogSev_Info, "Vkc", "Dissassembly of %s %s", vertInput.filename, vertInput.entrypoint);
            con_logf(LogSev_Info, "Vkc", "%s", vertOutput.disassembly);
        }
    }

    const vkrCompileInput fragInput =
    {
        .filename = shaderName,
        .entrypoint = "PSMain",
        .text = firstTri,
        .type = vkrShaderType_Frag,
        .compile = true,
        .disassemble = true,
    };
    vkrCompileOutput fragOutput = { 0 };
    if (vkrCompile(&fragInput, &fragOutput))
    {
        if (fragOutput.errors)
        {
            con_logf(LogSev_Error, "Vkc", "Errors while compiling %s %s", fragInput.filename, fragInput.entrypoint);
            con_logf(LogSev_Error, "Vkc", "%s", fragOutput.errors);
        }
        if (fragOutput.disassembly)
        {
            con_logf(LogSev_Info, "Vkc", "Dissassembly of %s %s", fragInput.filename, fragInput.entrypoint);
            con_logf(LogSev_Info, "Vkc", "%s", fragOutput.disassembly);
        }
    }

    pim_free(firstTri);
    firstTri = NULL;

    const vkrFixedFuncs ffuncs =
    {
        .viewport =
        {
            .x = 0.0f,
            .y = 0.0f,
            .width = (float)g_vkr.chain->width,
            .height = (float)g_vkr.chain->height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        },
        .scissor =
        {
            .extent.width = g_vkr.chain->width,
            .extent.height = g_vkr.chain->height,
        },
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
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
        .streamCount = 0, // hardcoded triangle in shader for now
        .types[0] = vkrVertType_float4, // positionOS
        .types[1] = vkrVertType_float4, // normalOS
        .types[2] = vkrVertType_float2, // texture uv
        .types[3] = vkrVertType_float2, // lightmap uv
    };

    VkPipelineShaderStageCreateInfo shaders[] =
    {
        vkrCreateShader(&vertOutput),
        vkrCreateShader(&fragOutput),
    };

    const VkAttachmentDescription attachments[] =
    {
        {
            .format = g_vkr.chain->format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        },
    };
    const VkAttachmentReference attachmentRefs[] =
    {
        {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        },
    };
    const VkSubpassDescription subpasses[] =
    {
        {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = NELEM(attachmentRefs),
            .pColorAttachments = attachmentRefs,
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

    vkrRenderPass* renderPass = vkrRenderPass_New(
        NELEM(attachments), attachments,
        NELEM(subpasses), subpasses,
        NELEM(dependencies), dependencies);
    const i32 subpass = 0;

    vkrPipelineLayout* pipeLayout = vkrPipelineLayout_New();

    vkrPipeline* pipeline = vkrPipeline_NewGfx(
        &ffuncs,
        &vertLayout,
        pipeLayout,
        NELEM(shaders), shaders,
        renderPass,
        subpass);

    vkrSwapchain_SetupBuffers(g_vkr.chain, renderPass);

    vkrCompileOutput_Del(&vertOutput);
    vkrCompileOutput_Del(&fragOutput);
    for (i32 i = 0; i < NELEM(shaders); ++i)
    {
        vkrDestroyShader(shaders + i);
    }

    // TODO: use these
    vkrPipeline_Release(pipeline);
    vkrRenderPass_Release(renderPass);
    vkrPipelineLayout_Release(pipeLayout);

    return true;
}

void vkr_update(void)
{

}

void vkr_shutdown(void)
{
    vkrSwapchain_Release(g_vkr.chain);
    g_vkr.chain = NULL;
    vkrDevice_Shutdown(&g_vkr);
    vkrDisplay_Release(g_vkr.display);
    g_vkr.display = NULL;
    vkrInstance_Shutdown(&g_vkr);
}

