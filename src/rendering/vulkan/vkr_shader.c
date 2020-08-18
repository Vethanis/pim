#include "rendering/vulkan/vkr_shader.h"
#include "allocator/allocator.h"
#include <string.h>

VkShaderModule vkrCreateShaderModule(const u32* dwords, i32 length)
{
    ASSERT(g_vkr.dev);
    VkShaderModule mod = NULL;
    if (dwords && length > 0)
    {
        const VkShaderModuleCreateInfo info =
        {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pCode = dwords,
            .codeSize = sizeof(dwords[0]) * length,
        };
        VkCheck(vkCreateShaderModule(g_vkr.dev, &info, NULL, &mod));
    }
    return mod;
}

void vkrDestroyShaderModule(VkShaderModule mod)
{
    ASSERT(g_vkr.dev);
    if (mod)
    {
        vkDestroyShaderModule(g_vkr.dev, mod, NULL);
    }
}

static vkrShaderTypeToStage(vkrShaderType type)
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
    }
}

VkPipelineShaderStageCreateInfo vkrCreateShader(vkrShaderType type, const u32* dwords, i32 length)
{
    VkPipelineShaderStageCreateInfo info =
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = vkrShaderTypeToStage(type),
        .module = vkrCreateShaderModule(dwords, length),
        .pName = "main",
    };
    return info;
}

void vkrDestroyShader(VkPipelineShaderStageCreateInfo* info)
{
    if (info)
    {
        vkrDestroyShaderModule(info->module);
        info->module = NULL;
    }
}
