#include "rendering/vulkan/vkr_shader.h"
#include "rendering/vulkan/vkr_compile.h"
#include "allocator/allocator.h"
#include "common/console.h"
#include <string.h>

static VkShaderModule vkrShaderModule_New(const u32* dwords, i32 dwordCount)
{
    ASSERT(g_vkr.dev);
    VkShaderModule handle = NULL;
    if (dwords && dwordCount > 0)
    {
        const VkShaderModuleCreateInfo info =
        {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pCode = dwords,
            .codeSize = sizeof(dwords[0]) * dwordCount,
        };
        VkCheck(vkCreateShaderModule(g_vkr.dev, &info, NULL, &handle));
        ASSERT(handle);
    }
    return handle;
}

static void vkrShaderModule_Del(VkShaderModule handle)
{
    ASSERT(g_vkr.dev);
    if (handle)
    {
        vkDestroyShaderModule(g_vkr.dev, handle, NULL);
    }
}

VkShaderStageFlags vkrShaderTypeToStage(vkrShaderType type)
{
    switch (type)
    {
    default:
        ASSERT(false);
        return 0;
    case vkrShaderType_Vert:
        return VK_SHADER_STAGE_VERTEX_BIT;
    case vkrShaderType_Frag:
        return VK_SHADER_STAGE_FRAGMENT_BIT;
    case vkrShaderType_Comp:
        return VK_SHADER_STAGE_COMPUTE_BIT;
    case vkrShaderType_Raygen:
        return VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_RAYGEN_BIT_NV;
    case vkrShaderType_AnyHit:
        return VK_SHADER_STAGE_ANY_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_NV;
    case vkrShaderType_ClosestHit:
        return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;
    case vkrShaderType_Miss:
        return VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_NV;
    case vkrShaderType_Isect:
        return VK_SHADER_STAGE_INTERSECTION_BIT_KHR | VK_SHADER_STAGE_INTERSECTION_BIT_NV;
    case vkrShaderType_Call:
        return VK_SHADER_STAGE_CALLABLE_BIT_KHR | VK_SHADER_STAGE_CALLABLE_BIT_NV;
    case vkrShaderType_Task:
        return VK_SHADER_STAGE_TASK_BIT_NV;
    case vkrShaderType_Mesh:
        return VK_SHADER_STAGE_MESH_BIT_NV;
    }
}

bool vkrShader_New(
    VkPipelineShaderStageCreateInfo* shader,
    const char* filename,
    const char* entrypoint,
    vkrShaderType type)
{
    bool success = true;
    ASSERT(shader);
    ASSERT(filename);
    ASSERT(entrypoint);
    memset(shader, 0, sizeof(*shader));

    vkrCompileOutput output = { 0 };
    char* text = NULL;

    shader->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader->stage = vkrShaderTypeToStage(type);
    shader->pName = entrypoint;

    text = vkrLoadShader(filename);
    if (!text)
    {
        ASSERT(false);
        success = false;
        goto cleanup;
    }

    const vkrCompileInput input =
    {
        .filename = filename,
        .entrypoint = entrypoint,
        .type = type,
        .compile = true,
        .text = text,
    };
    if (!vkrCompile(&input, &output))
    {
        ASSERT(false);
        if (output.errors)
        {
            con_logf(LogSev_Error, "vkr", "Shader compile errors:\n%s", output.errors);
        }
        success = false;
        goto cleanup;
    }

    shader->module = vkrShaderModule_New(output.dwords, output.dwordCount);
    if (!shader->module)
    {
        success = false;
        goto cleanup;
    }

cleanup:
    vkrCompileOutput_Del(&output);
    pim_free(text);
    if (!success)
    {
        vkrShader_Del(shader);
    }
    return success;
}

void vkrShader_Del(VkPipelineShaderStageCreateInfo* info)
{
    if (info)
    {
        vkrShaderModule_Del(info->module);
        memset(info, 0, sizeof(*info));
    }
}
